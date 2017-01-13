#include "power_manager.h"
#include "config.h"
#include "main.h"

#include "string.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

static int m_port;
static char m_server[256];

void power_set(bool _status) {
  printf("Setting power %d\n", _status);
  gpio_write(RELAY_PIN, _status);
}

static char recv_buf[1024];
bool power_manager_get(bool *_status) {
  const struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo *res;
  
  printf("Running DNS lookup for %s...\r\n", m_server);
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
  printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));
  
  int s = socket(res->ai_family, res->ai_socktype, 0);
  if(s < 0) {
    printf("... Failed to allocate socket.\r\n");
    freeaddrinfo(res);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return false;
  }
  
  printf("... allocated socket\r\n");
  
  if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
    close(s);
    freeaddrinfo(res);
    printf("... socket connect failed.\r\n");
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    return false;
  }
  
  printf("... connected\r\n");
  freeaddrinfo(res);
  
  const char *req =
  "GET "STATUS_URL"\r\n"
  "User-Agent: esp-open-rtos/0.1 esp8266\r\n"
  "\r\n";
  if (write(s, req, strlen(req)) < 0) {
    printf("... socket send failed\r\n");
    close(s);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    return false;
  }
  printf("... socket send success\r\n");
  
  bool ret = false;
  
  int r;
  bzero(recv_buf, sizeof(recv_buf));
  r = read(s, recv_buf, sizeof(recv_buf));
  if(r > 0) {
    printf("%s", recv_buf);
    char *data = strstr(recv_buf, STATUS_KEY);
    if(data != NULL) {
      data += strlen(STATUS_KEY);
      int status = strtol(data, NULL, 10);
      *_status = status == 1;
      ret = true;
    } else {
      printf("Invalid answer\n");
    }
  }
  
  close(s);
  printf("... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
  return ret;
}

static void power_manager_task(void *pvParameters)
{
  while(true) {
    if(is_connected()) {
      bool status;
      if(power_manager_get(&status)) {
        power_set(status);
      } else {
        printf("Failed to fetch data\n");
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void power_manager_init() {
  gpio_enable(RELAY_PIN, GPIO_OUTPUT);
  power_set(false);
  
  memset(m_server, 0, 256);
  m_port = 0;
  load_server(m_server, &m_port);
  
  xTaskCreate(power_manager_task, (const char *)"power_manager_task", 512, NULL, 3, NULL);
}