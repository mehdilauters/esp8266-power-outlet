#include "power_manager.h"
#include "config.h"
#include "main.h"

#include "string.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

void power_manager_set(bool _status) {
  printf("Setting power %d\n", _status);
  gpio_write(RELAY_PIN, _status);
}

void power_manager_init() {
  gpio_enable(RELAY_PIN, GPIO_OUTPUT);
  power_manager_set(false);
}