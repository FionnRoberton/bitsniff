# bitsniff
Raw bitbang UART baud rate detector for Linux via Bus Pirate.

## Overview
bitsniff detects the baud rate of an unknown UART device using a SparkFun Bus Pirate, without requiring a logic analyser. It puts the Bus Pirate into raw bitbang mode, polls pin state at high frequency, and measures pulse widths to identify the shortest stable signal — which corresponds to one bit period. Inverting that period returns the baud rate, matched against standard rates with a confidence score.

Built for hardware security assessments and firmware extraction workflows where baud rate is unknown and conventional tooling isn't available.

## Hardware Requirements
- Bus Pirate v3 or v4
- Target device with UART output
- Correctly wired MISO/GND pins

## Building

Install dependency:
```
sudo apt install libserialport-dev
```

Build:
```
clang -o bitsniff bitsniff.c -lserialport -Wall -Wextra
```

Run:
```
./bitsniff -d /dev/ttyUSB0
```

## Hardware Constraints and Known Limitations

**Sampling ceiling: ~2000 baud maximum.**

bitsniff polls Bus Pirate pin state over USB. Each poll-response cycle takes approximately 1ms due to USB round-trip latency, yielding roughly 1000 samples per second. Per Nyquist, reliable signal detection requires several samples per bit period — at 1000 samples/second this limits detection to approximately 1000–2000 baud.

This is a hardware constraint of the Bus Pirate USB interface, not a software limitation. A logic analyser or FTDI chip in bitbang mode would remove this ceiling entirely.

The Bus Pirate may occasionally need power cycling between uses if it drops into SPI mode.

## Potential Extensions
- FTDI bitbang backend to raise the sampling ceiling significantly
- Auto-detection across multiple `/dev/ttyUSB*` interfaces
- Passive sniffing mode without active Bus Pirate control

## License
MIT
