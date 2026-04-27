#include <libserialport.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEVICE_PATH "/dev/ttyUSB0"

struct sp_port *port;

int connect_to_device(const char *device_path) {
  enum sp_return result;

  result = sp_get_port_by_name(device_path, &port);
  if (result != SP_OK) {
    fprintf(stderr, "Error finding port: %s\n", sp_last_error_message());
    return 0;
  }

  result = sp_open(port, SP_MODE_READ_WRITE);
  if (result != SP_OK) {
    fprintf(stderr, "Error opening port: %s\n", sp_last_error_message());
    sp_free_port(port);
    return 0;
  }

  sp_set_baudrate(port, 115200);
  sp_set_bits(port, 8);
  sp_set_parity(port, SP_PARITY_NONE);
  sp_set_stopbits(port, 1);
  sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

  return 1;
}

int enter_bitbang_mode() {
  uint8_t cmd = 0x00;
  char response[6] = {0};

  for (int i = 0; i < 20; i++) {
    sp_blocking_write(port, &cmd, 1, 100);
  }

  sp_blocking_read(port, response, 5, 500);

  if (strncmp(response, "BBIO1", 5) == 0) {
    printf("Entered bitbang mode\n");
    return 1;
  }

  fprintf(stderr, "Failed to enter bitbang mode\n");
  return 0;
}

int main(void) {
  printf("Attempting connection to %s\n\n", DEVICE_PATH);

  if (!connect_to_device(DEVICE_PATH)) {
    printf("FAIL\n\n");
    return 1;
  }
  printf("SUCCESS\n\n");

  printf("Attempting to Enter BitBang mode: \n\n");

  if (!enter_bitbang_mode()) {
    printf("FAIL\n\n");
    return 1;
  }
  printf("SUCCESS\n\n");

  return 0;
}
