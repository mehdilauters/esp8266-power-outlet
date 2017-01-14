#ifndef CONFIG_HPP
#define CONFIG_HPP

#define DEFAULT_SSID "esp-power-outlet"

// should be firmwar_1.bin on the server
#define TFTP_IMAGE_FILENAME "firmware.bin"
#define PROGRAM_PUSH_PIN 0
#define POWERON_URL "/set/on"

#define RELAY_PIN 16
// 0 = infite timeout
#define TIMEOUT_S 0

#endif