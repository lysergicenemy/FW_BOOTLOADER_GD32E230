# Bootloader for GD32E230 MCU

A compact bootloader implementation for GD32E230 microcontrollers compatible with the STM32 USART bootloader protocol. The bootloader fits within a 1 flash page (< 1024 bytes) and supports RS485 communication with hardware-driven DE pin control.

## Features

- **STM32 Protocol Compatibility**: Works with standard STM32 programming tools (STM32CubeProgrammer)
- **Supported Commands**:
  - Get (0x00)
  - Get ID (0x02)
  - Read Memory (0x11)
  - Write Memory (0x31)
  - Erase Memory (0x43)
  - Go (0x21)
- **Fixed Baud Rate**: 57600 8N1 (USART0 - PA9: TX, PA10: RX)
- **RS485 Support**: Hardware-driven DE pin (PA1) for RS485 transceiver control
- **Minimal Size**: Less than 1KB flash footprint
- **Boot Modes**: Magic word using RTC backup registers or GPIO state

## Limitations

1. **Erase Command Behavior**:
   - Any erase command erases all application flash pages (except the bootloader page)
   - Does not verify XOR checksum for page numbers

2. **Go Command Behavior**:
   - Does not verify XOR checksum for the address
   - Always jumps to adress defined in `APP_START_ADDR` (0x08000400 by defualt)

## Building

```bash
make
```


