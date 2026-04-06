/**
 * Ultra-minimal STM32 USART bootloader protocol for GD32E230
 * Target: < 1024 bytes total
 * Compatible with STM32CubeProgrammer
 *
 * Supports: Get, Get Version, Get ID, Read Memory, Write Memory, Erase Memory, Go
 * Fixed baud rate: 57600, RS485 DE on PA1
 */

#include "gd32e23x.h"
#include "stm32_bootloader.h"

/* Simple inline functions */
static inline void send_byte(uint8_t data)
{
    while ((USART_STAT(USART0) & USART_STAT_TBE) == 0)
        ;
    USART_TDATA(USART0) = data;
}

static inline uint8_t receive_byte(void)
{
    while ((USART_STAT(USART0) & USART_STAT_RBNE) == 0)
        ;
    return (uint8_t)USART_RDATA(USART0);
}

static inline uint8_t xor_checksum(const uint8_t *data, uint32_t len)
{
    uint8_t sum = 0;
    while (len--)
        sum ^= *data++;
    return sum;
}

static void flash_erase_page(uint32_t page_addr)
{
    /* Align to page boundary */
    page_addr &= ~(FLASH_PAGE_SIZE - 1);
    FMC_STAT = (FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);

    while (FMC_STAT & FMC_STAT_BUSY)
        ;

    FMC_CTL |= FMC_CTL_PER;
    FMC_ADDR = page_addr;
    FMC_CTL |= FMC_CTL_START;

    while (FMC_STAT & FMC_STAT_BUSY)
        ;
    /* reset the PER bit */
    FMC_CTL &= ~FMC_CTL_PER;
}

/**
 * Initialize USART0 for 57600 baud with RS485 DE (PA1)
 */
static void usart0_init(void)
{
    /* Enable GPIOA and USART0 clocks */
    RCU_REG_VAL(RCU_GPIOA) |= BIT(RCU_BIT_POS(RCU_GPIOA));
    RCU_REG_VAL(RCU_USART0) |= BIT(RCU_BIT_POS(RCU_USART0));

    /* GPIO PA9 PA10 PA1 config */
    GPIO_AFSEL0(GPIOA) = 0x00000010;
    GPIO_AFSEL1(GPIOA) = 0x00000110;
    GPIO_CTL(GPIOA) = 0x28280008;
    GPIO_PUD(GPIOA) = 0x24140008;
    GPIO_OSPD(GPIOA) = 0x0c140004;

    /* USART0 config: 57600, 8N1, RS485 DE enabled */
    USART_BAUD(USART0) = (uint32_t)(8000000U / 57600U);
    USART_CTL0(USART0) = 0x0042000c;
    USART_CTL2(USART0) = 0x00005000;
    USART_CTL0(USART0) |= USART_CTL0_UEN;
}

/**
 * Handle Get command (0x00) - minimal response
 */
static void handle_get(void)
{
    // send_byte(ACK_BYTE);
    send_byte(6); /* 7 bytes follow (version + 6 commands, N = bytes - 1) */
    send_byte(BOOTLOADER_VERSION);
    send_byte(CMD_GET);
    send_byte(CMD_GET_ID);
    send_byte(CMD_READ_MEMORY);
    send_byte(CMD_WRITE_MEMORY);
    send_byte(CMD_ERASE_MEMORY);
    send_byte(CMD_GO);
    send_byte(ACK_BYTE);
}

/**
 * Handle Get ID command (0x02) - returns GD32E230 chip ID (0x0440)
 */
static void handle_get_id(void)
{
    // send_byte(ACK_BYTE);
    send_byte(1); /* N = 1 (2 bytes of ID follow) */
    send_byte(PID_MSB);
    send_byte(PID_LSB);
    send_byte(ACK_BYTE);
}

/**
 * Handle Read Memory command (0x11) - reads memory content
 */
static void handle_read_memory(void)
{
    uint16_t n, i;
    uint8_t *ptr;
    addr_t addr;

    /* Get address */
    for (i = 0; i < 4; i++)
        addr.byte[3U - i] = receive_byte();
    if (xor_checksum(addr.byte, 4) != receive_byte())
    {
        send_byte(NACK_BYTE);
        return;
    }

    send_byte(ACK_BYTE);

    /* Get number of bytes to read */
    n = receive_byte();
    if ((n ^ receive_byte()) != 0xFF)
    {
        send_byte(NACK_BYTE);
        return;
    }

    send_byte(ACK_BYTE);

    /* Read and send data */
    ptr = (uint8_t *)addr.word;
    for (i = 0; i <= n; i++) // n+1 bytes, i is uint16_t, no overflow
    {
        send_byte(ptr[i]);
    }
}

/**
 * Handle Write Memory command (0x31) - minimal
 */
static void handle_write_memory(void)
{
    uint32_t i, data_len;
    uint32_t xor_sum;
    addr_t addr;
    uint8_t data[MAX_WRITE_SIZE];

    /* Get address (big-endian) */
    for (i = 0; i < 4; i++)
        addr.byte[3U - i] = receive_byte();

    if (xor_checksum(addr.byte, 4) != receive_byte())
    {
        send_byte(NACK_BYTE);
        return;
    }

    /* if xor ok - ack */
    send_byte(ACK_BYTE);

    /* Get N (number of bytes - 1) */
    xor_sum = receive_byte();

    data_len = xor_sum + 1; // actual number of bytes

    /* Receive data and compute checksum */
    for (i = 0; i < data_len; i++)
    {
        data[i] = receive_byte();
        xor_sum ^= data[i];
    }

    if (xor_sum != receive_byte())
    {
        send_byte(NACK_BYTE);
        return;
    }

    /* Program flash */
    flash_unlock();

    uint32_t *word_ptr = (uint32_t *)data;
    uint32_t word_count = data_len / 4;

    /* Wait BSY flag is reset and set PG */
    while (FMC_STAT & FMC_STAT_BUSY)
        ;
    FMC_CTL |= FMC_CTL_PG;

    /* Write pages */
    for (i = 0; i < word_count; i++)
    {
        REG32(addr.word + i * 4) = word_ptr[i];
        while (FMC_STAT & FMC_STAT_BUSY)
            ;
    }

    /* Clear PG */
    FMC_CTL &= ~FMC_CTL_PG;
    // FMC_STAT = (FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);

    flash_lock();

    send_byte(ACK_BYTE);
}

/**
 * Handle Erase Memory command (0x43) - global erase only
 */
static void handle_erase_memory(void)
{
    uint8_t n_pages_minus1, i;

    /* Receive N (number of pages - 1) or 0xFF for global erase */
    n_pages_minus1 = receive_byte();

    for (i = 0; i <= n_pages_minus1; i++)
    {
        receive_byte();
    }

    /* xor */
    receive_byte();

    /* Unlock flash and erase all application pages */
    flash_unlock();

    
    //for (uint32_t page = 1; page <= n_pages_minus1 + 1; page++) /* Skip page 0 (bootloader) */
    for (uint32_t page = 1; page < FLASH_TOTAL_PAGES; page++) /* Skip page 0 (bootloader) */
    {
        uint32_t page_address = FLASH_BASE + (page * FLASH_PAGE_SIZE);
        flash_erase_page(page_address);
    }

    flash_lock();

    send_byte(ACK_BYTE);
}

/*
  jump from the bootloader to the application code
 */
#if 0
void jump2app(void)
{
    __disable_irq();

    const uint32_t app_address = APP_START_ADDR;
    const uint32_t *app_data = (const uint32_t *)app_address;
    const uint32_t stack_top = app_data[0];
    const uint32_t JumpAddress = app_data[1];

    // setup vector table
    SCB->VTOR = app_address;

    // setup sp, msp and jump
    asm volatile(
        "mov sp, %0	\n"
        "msr msp, %0	\n"
        "bx	%1	\n"
        : : "r"(stack_top), "r"(JumpAddress) :);
}
#endif

void jump2app(void)
{
    __disable_irq();
    SCB->VTOR = APP_START_ADDR;
    uint32_t sp = *(volatile uint32_t *)APP_START_ADDR;
    uint32_t pc = *(volatile uint32_t *)(APP_START_ADDR + 4);
    __set_MSP(sp);
    ((void (*)(void))pc)();
}

/**
 * Handle Go command (0x21) - SIMPLE VERSION. always jumps to APP_START_ADDR
 */
static void handle_go(void)
{
    uint32_t i;
    /* 4x address + xor */
    for (i = 0; i < 5; i++)
    {
        receive_byte();
    }
    /* send ACK */
    send_byte(ACK_BYTE);
    /* delay for jump2app */
    for (i = 255U; i != 0U; i--)
    {
        __NOP();
    }
    /* Jump to application */
    jump2app();
}

/**
 * Main bootloader protocol
 */
void stm32_bootloader_run(void)
{
    uint8_t cmd, comp;

    usart0_init();

    /* Wait for sync */
    while (receive_byte() != SYNC_BYTE)
        ;
    send_byte(ACK_BYTE);

    /* Command loop */
    while (1)
    {
        cmd = receive_byte();
        comp = receive_byte();

        if ((cmd ^ comp) != 0xFF)
        {
            send_byte(NACK_BYTE);
            continue;
        }

        /* ack for command */
        send_byte(ACK_BYTE);

        switch (cmd)
        {
        case CMD_GET:
            handle_get();
            break;
        case CMD_GET_ID:
            handle_get_id();
            break;
        case CMD_READ_MEMORY:
            handle_read_memory();
            break;
        case CMD_WRITE_MEMORY:
            handle_write_memory();
            break;
        case CMD_ERASE_MEMORY:
            handle_erase_memory();
            break;
        case CMD_GO:
            handle_go();
            break;
        default:
            send_byte(NACK_BYTE);
            break;
        }
    }
}