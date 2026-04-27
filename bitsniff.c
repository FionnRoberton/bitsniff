#include <libserialport.h>
#include <stdio.h>

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

int main(void) {
  printf("Attempting connection to %s\n\n", DEVICE_PATH);

  if (!connect_to_device(DEVICE_PATH)) {
    printf("FAIL\n\n");
    return 1;
  }
  printf("SUCCESS\n\n");
  return 0;
}
