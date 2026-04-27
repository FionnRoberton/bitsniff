# bitsniff
Raw bitbang UART baud rate detector for Linux

## Overview

bitsniff is a tool designed to detect the baud rate of a device connected via SparkFun "Bus Pirate". I developed this for my own use when I was attempting to find the baud rate of an unkown UART device without access to a logic analyser. bitsniff works by putting the buspirate into a raw bitbang mode, and polling pin state at a high frequency, and then measuring pulse widths in order to identify the shortest stable sigal, which corresponds to a bit period. Inverting that result will return the baud rate, which is then matched against standard rates with a confidence score.

## Hardware requirements
- Bus Pirate v3/v4
- Target device with UART output
- Correctly wired pins

## Building 
  
Install the Dependencey: 

`sudo apt install libserialport-dev`

Build:
```bash
clang -o bitsniff bitsniff.c -lserialport -Wall -Wextra
```

-o bitsniff — output binary name

-lserialport — link against libserialport

-Wall -Wextra — enable all warnings

Run:

`./bitsniff -d /dev/ttyUSB0`

## Hardware constraints and known limitations

**Sampling ceiling: ~2000 baud maximum**

bitsniff works by polling the Bus Pirate pin state over USB. Each 
poll-and-response cycle takes approximately 1ms due to USB round-trip 
latency. This means the tool can take roughly 1000 samples per second.

To reliably detect a signal, you need to sample at least several times 
per bit period (Nyquist). At 1000 samples/second, the shortest bit 
period you can resolve is around 500-1000µs, which corresponds to a 
baud rate ceiling of approximately 1000-2000 baud.

Signals above this rate will not be detected correctly. This is a 
hardware constraint of the Bus Pirate USB interface, not a software 
limitation. A logic analyser or FTDI chip in bitbang mode would remove 
this ceiling entirely.

Bus pirate may sometimes need to be power cycled in between uses, due to occasionally dropping to SPI mode.

##  License

MIT
