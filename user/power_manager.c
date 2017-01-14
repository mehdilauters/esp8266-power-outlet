#include "power_manager.h"
#include "config.h"
#include "main.h"

#include "string.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

static int time;
static int m_port;
static char m_server[256];

void power_manager_set(bool _status) {
  printf("Setting power %d\n", _status);
  gpio_write(RELAY_PIN, _status);
  
#if TIMEOUT_S != 0
  if(_status) {
    timer_set_run(FRC1, false);
    time = TIMEOUT_S;
    timer_set_frequency(FRC1, 1);
    timer_set_interrupts(FRC1, true);
    timer_set_run(FRC1, true);
  }
#endif
}

void frc1_interrupt_handler(void)
{
  if(time <= 0) {
    power_manager_set(false);
    timer_set_run(FRC1, false);
  }
  time--;
}

char recv_buf[256];
bool power_manager_get() {
  const struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo *res = NULL;
  
  char port[10];
  sprintf(port, "%d", m_port);
  int err = getaddrinfo(m_server, port, &hints, &res);
  
  if(err != 0 || res == NULL) {
    printf("DNS lookup failed err=%d res=%p\r\n", err, res);
    if(res)
      freeaddrinfo(res);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return false;
  }
  /* Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
  struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
  
  int s = socket(res->ai_family, res->ai_socktype, 0);
  if(s < 0) {
    printf("... Failed to allocate socket.\r\n");
    freeaddrinfo(res);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return false;
  }
  
  
  if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
    close(s);
    freeaddrinfo(res);
    printf("... socket connect failed.\r\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return false;
  }
  
  freeaddrinfo(res);
  
  const char *req =
  "GET "POWERON_URL"\r\n"
  "User-Agent: esp-open-rtos/0.1 esp8266\r\n"
  "\r\n";
  if (write(s, req, strlen(req)) < 0) {
    printf("... socket send failed\r\n");
    close(s);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    return false;
  }
  
  bool ret = false;
  
  int r;
  bzero(recv_buf, sizeof(recv_buf));
  r = read(s, recv_buf, sizeof(recv_buf));
  if(r > 0) {
    printf("%s", recv_buf);
  }
  
  close(s);
  return ret;
}

static void ping_task(void *pvParameters)
{
  printf("Starting ping task to http://%s:%d\n", m_server, m_port);
  while(true) {
    if(is_connected()) {
      power_manager_get();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(200 / portTICK_PERIOD_MS); 
    }
  }
}

void power_manager_init() {
  gpio_enable(RELAY_PIN, GPIO_OUTPUT);
  power_manager_set(false);
  
  timer_set_interrupts(FRC1, false);
  timer_set_run(FRC1, false);
  _xt_isr_attach(INUM_TIMER_FRC1, frc1_interrupt_handler);
  
  memset(m_server, 0, 256);
  m_port = 0;
  if(load_server(m_server, &m_port)) {
    xTaskCreate(ping_task, (const char *)"ping_task", 512, NULL, 3, NULL);
  }
}