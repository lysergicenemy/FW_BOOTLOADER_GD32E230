/**
 * Ultra-minimal bootloader for GD32E230F4
 * Size target: 1 flash page (1024 bytes)
 * Uses RTC backup registers for magic word
 * USART0 with RS485 DE control
 */

#include "gd32e23x.h"
#include "bootloader.h"

/* Memory layout */
#define APP_START_ADDR 0x08000400UL /* Application starts after 1KB bootloader */
#define MAGIC_WORD 0x424C454EUL     /* ASCII: 'BLEN' */

/* Minimal clock initialization */
static inline void clock_init(void)
{
    /* Enable IRC8M */
    RCU_CTL0 |= RCU_CTL0_IRC8MEN;
    /* Wait for stabilization */
    while ((RCU_CTL0 & RCU_CTL0_IRC8MSTB) == 0)
    {
    }

    /* Enable backup domain */
    RCU_REG_VAL(RCU_PMU) |= BIT(RCU_BIT_POS(RCU_PMU));
    RCU_REG_VAL(RCU_RTC) |= BIT(RCU_BIT_POS(RCU_RTC));
    PMU_CTL |= PMU_CTL_BKPWEN;
}

/* Write magic word to RTC backup register */
static inline void write_magic_word(uint32_t value)
{
    /* Unlock RTC registers */
    RTC_WPK = 0xCA;
    RTC_WPK = 0x53;

    /* Write to backup register */
    RTC_BKP0 = value;

    /* Lock RTC */
    RTC_WPK = 0xFF;
}

/* Main bootloader logic */
int main(void)
{
    /* Initialize clocks */
    clock_init();

#ifdef BL_USE_MAGICWORD
    if (RTC_BKP0 != MAGIC_WORD)
    /* Magic word is not correct - jump to application */
#else
    if ((!GPIO_ISTAT(BL_GPIO_PORT) & (BL_GPIO_PIN)))
    /* GPIO is LOW state - jump to application */
#endif
    {
        jump2app();
    }

    /* Magic word is set - stay in bootloader */
    /* Clear magic word to indicate we're in bootloader */
    write_magic_word(0U);

    /* Run STM32-compatible bootloader protocol */
    stm32_bootloader_run();

    /* Should never reach here */
    while (1)
        ;
}
