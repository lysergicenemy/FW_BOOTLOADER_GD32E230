/**
 * STM32 USART bootloader protocol implementation for GD32E230
 * Compatible with STM32CubeProgrammer
 *
 * Minimal implementation for <= 1KB flash
 * Supports: Get, Get ID, Write Memory, Erase Memory, Go
 * Fixed baud rate: 57600
 * RS485 DE control via PA1 (hardware)
 */

#ifndef STM32_BOOTLOADER_H
#define STM32_BOOTLOADER_H


/* Bootloader version */
#define BOOTLOADER_VERSION 0x10 /* Version 1.0 */

/* Bootloader config if use gpio comment BL_USE_MAGICWORD */
#define BL_USE_MAGICWORD
#define BL_GPIO_PORT GPIOA
#define BL_GPIO_PIN GPIO_PIN_0

/* Memory configuration */
#define APP_START_ADDR 0x08000400UL /* Application starts after 1KB bootloader */
#define FLASH_PAGE_SIZE 1024        /* GD32E230 flash page size */
#define FLASH_TOTAL_PAGES 16        /* 16KB total flash = 16 pages */
#define MAX_WRITE_SIZE 256          /* Maximum data block size (protocol allows up to 256) */

/* Protocol constants */
#define SYNC_BYTE 0x7F
#define ACK_BYTE 0x79
#define NACK_BYTE 0x1F

/* Device PID */
#define PID_MSB 0x04
#define PID_LSB 0x44

/* Command codes */
#define CMD_GET 0x00
#define CMD_GET_VERSION 0x01
#define CMD_GET_ID 0x02
#define CMD_READ_MEMORY 0x11
#define CMD_WRITE_MEMORY 0x31
#define CMD_ERASE_MEMORY 0x43
#define CMD_GO 0x21

union addr_u
{
    uint8_t byte[4];
    uint32_t word;
};
typedef union addr_u addr_t;

/**
 * Initialize and run STM32 bootloader protocol
 * This function will not return until a Go command is received
 * or a system reset is triggered
 */
void stm32_bootloader_run(void);

void jump2app(void);

static inline void flash_lock(void)
{
    FMC_CTL |= FMC_CTL_LK;
}

static inline void flash_unlock(void)
{
    FMC_KEY = UNLOCK_KEY0;
    FMC_KEY = UNLOCK_KEY1;
}

#endif /* STM32_BOOTLOADER_H */