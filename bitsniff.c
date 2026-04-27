#include <libserialport.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define NUM_BUCKETS 200
#define BUCKET_SIZE 25
#define SAMPLE_COUNT 10000
#define MIN_PULSE_US 200

// pin mapping in bitbang mode response byte:
// bit 7: POWER, 6: PULLUP, 5: AUX, 4: MOSI, 3: CLK, 2: MISO, 1: CS, 0: unused

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

  sp_flush(port, SP_BUF_BOTH);
  usleep(100000);

  for (int i = 0; i < 20; i++) {
    sp_blocking_write(port, &cmd, 1, 100);
  }

  sp_drain(port);
  usleep(200000);
  sp_blocking_read(port, response, 5, 1000);

  if (strncmp(response, "BBIO1", 5) == 0) {
    printf("Entered bitbang mode\n");
    usleep(100000);
    return 1;
  }

  fprintf(stderr, "Failed to enter bitbang mode\n");
  fprintf(stderr,
          "Ensure Bus Pirate is in HiZ mode. Power cycle if necessary.\n");
  return 0;
}

long detect_transition(int pin) {
  uint8_t cmd = 0x80;
  uint8_t response;
  int bucket_index = 0;
  long buckets[NUM_BUCKETS] = {0};
  struct timespec ts;
  int transition_count = 0;

  // seed initial sample
  sp_blocking_write(port, &cmd, 1, 10);
  sp_blocking_read(port, &response, 1, 10);
  uint8_t prev = (response >> pin) & 0x01;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  long prev_micros = ts.tv_sec * 1000000L + ts.tv_nsec / 1000;

  printf("Sampling signal...\n");

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sp_blocking_write(port, &cmd, 1, 10);
    sp_blocking_read(port, &response, 1, 10);
    uint8_t current = (response >> pin) & 0x01;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long current_micros = ts.tv_sec * 1000000L + ts.tv_nsec / 1000;

    if (prev != current) {
      long delta_us = current_micros - prev_micros;
      prev = current;
      prev_micros = current_micros;
      if (delta_us > MIN_PULSE_US) {
        bucket_index = delta_us / BUCKET_SIZE;
        if (bucket_index < NUM_BUCKETS) {
          buckets[bucket_index]++;
          transition_count++;
        }
      }
    }
  }

  if (transition_count == 0) {
    fprintf(stderr, "No transitions detected. Is the target transmitting?\n");
    return -1;
  }

  int max_count = 0;
  int winner = 0;
  for (int i = 0; i < NUM_BUCKETS; i++) {
    if (buckets[i] > max_count) {
      max_count = buckets[i];
      winner = i;
    }
  }

  printf("\nPulse width distribution:\n");
  for (int i = 0; i < NUM_BUCKETS; i++) {
    if (buckets[i] > 0) {
      printf("  %4ldus - %4ldus : %ld hits\n", (long)i * BUCKET_SIZE,
             (long)(i + 1) * BUCKET_SIZE, buckets[i]);
    }
  }

  long bit_period = winner * BUCKET_SIZE + BUCKET_SIZE / 2;
  printf("\nWinning bucket: %d, count: %d, bit period: %ldus\n", winner,
         max_count, bit_period);
  return bit_period;
}

long calculate_baud(long pulse_width) { return 1000000 / pulse_width; }

long match_standard_baud(long raw_baud) {
  const long standard_rates[] = {300,   1200,  2400,  4800,  9600,
                                 19200, 38400, 57600, 115200};
  const int num_rates = 9;

  long best = standard_rates[0];
  long best_err = labs(raw_baud - standard_rates[0]);

  for (int i = 1; i < num_rates; i++) {
    long err = labs(raw_baud - standard_rates[i]);
    if (err < best_err) {
      best_err = err;
      best = standard_rates[i];
    }
  }

  return best;
}

int confidence(long raw_baud, long matched_baud) {
  double err = (double)labs(raw_baud - matched_baud) / matched_baud;
  int conf = (int)((1.0 - err) * 100);
  return conf < 0 ? 0 : conf;
}

void exit_bitbang_mode() {
  uint8_t cmd = 0x0F;
  sp_blocking_write(port, &cmd, 1, 100);
  sp_drain(port);
  usleep(500000);
}

int main(int argc, char *argv[]) {
  const char *device_path = "/dev/ttyUSB0"; // default
  int pin = TARGET_PIN;
  int opt;

  while ((opt = getopt(argc, argv, "d:p:h")) != -1) {
    switch (opt) {
    case 'd':
      device_path = optarg;
      break;
    case 'p':
      pin = atoi(optarg);
      if (pin < 0 || pin > 7) {
        fprintf(stderr, "Pin must be between 0 and 7\n");
        return 1;
      }
      break;
    case 'h':
      printf("Usage: bitsniff [-d device] [-p pin]\n");
      printf("  -d  serial device path (default: /dev/ttyUSB0)\n");
      printf("  -p  pin bit position 0-7 (default: %d)\n", TARGET_PIN);
      printf("\nPin mapping (Bus Pirate bitbang response byte):\n");
      printf("  7: POWER  6: PULLUP  5: AUX  4: MOSI\n");
      printf("  3: CLK    2: MISO    1: CS   0: unused\n");
      return 0;
    default:
      fprintf(stderr, "Usage: bitsniff [-d device] [-p pin]\n");
      return 1;
    }
  }

  printf("bitsniff - UART baud rate detector\n");
  printf("Device: %s  Pin: %d\n\n", device_path, pin);

  if (!connect_to_device(device_path)) {
    return 1;
  }
  printf("Connected\n");

  if (!enter_bitbang_mode()) {
    sp_close(port);
    sp_free_port(port);
    return 1;
  }

  long pulse_width = detect_transition(pin);
  if (pulse_width <= 0) {
    exit_bitbang_mode();
    sp_close(port);
    sp_free_port(port);
    return 1;
  }

  long raw_baud = calculate_baud(pulse_width);
  long matched = match_standard_baud(raw_baud);
  int conf = confidence(raw_baud, matched);

  printf("\n--- Results ---\n");
  printf("Bit period:    %ld us\n", pulse_width);
  printf("Raw baud rate: %ld\n", raw_baud);
  printf("Detected baud: %ld (confidence: %d%%)\n", matched, conf);

  exit_bitbang_mode();
  sp_close(port);
  sp_free_port(port);

  return 0;
}
