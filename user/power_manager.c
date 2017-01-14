#include "power_manager.h"
#include "config.h"
#include "main.h"

#include "string.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

int time;

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

void power_manager_init() {
  gpio_enable(RELAY_PIN, GPIO_OUTPUT);
  power_manager_set(false);
  
  timer_set_interrupts(FRC1, false);
  timer_set_run(FRC1, false);
  _xt_isr_attach(INUM_TIMER_FRC1, frc1_interrupt_handler);
  
}