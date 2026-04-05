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
- **Fixed Baud Rate**: 57600 8N1
- **RS485 Support**: Hardware-driven DE pin (PA1) for RS485 transceiver control
- **Minimal Size**: Less than 1KB flash footprint

## Limitations

1. **Erase Command Behavior**:
   - Any erase command erases all application flash pages (except the bootloader page)
   - Does not verify XOR checksum for page numbers

2. **Go Command Behavior**:
   - Does not verify XOR checksum for the address
   - Always jumps to adress defined in `APP_START_ADDR` (0x08000400 by defualt)

## Advantages

- **Full STM32 Protocol Compatibility**: Can be used with standard STM32 programming software
- **Ultra-Compact Size**: Occupies only one flash page (< 1024 bytes)
- **RS485 Interface Support**: Hardware-controlled DE pin enables reliable RS485 communication
- **Minimal Resource Usage**: Optimized for small flash footprint

## Building

```bash
make
```

The build process generates:
- `build/bootloader_encoder_gd32.bin` (binary)
- `build/bootloader_encoder_gd32.hex` (Intel HEX)
- `build/bootloader_encoder_gd32.elf` (ELF)

## Usage

1. Connect the GD32E230 USART0 (PA9: TX, PA10: RX) to your RS485 transceiver
2. PA1 is used as DE (Driver Enable) control pin
3. Use STM32CubeProgrammer or compatible software with the following settings:
   - Interface: UART
   - Baud Rate: 57600
   - Parity: None
   - Data Bits: 8
   - Stop Bits: 1

## Project Structure

- `Src/stm32_bootloader_minimal.c` - Main bootloader implementation
- `Inc/stm32_bootloader.h` - Bootloader configuration and constants
- `GD32E230_FLASH_MINIMAL.ld` - Linker script for minimal bootloader placement
- `Makefile` - Build configuration

