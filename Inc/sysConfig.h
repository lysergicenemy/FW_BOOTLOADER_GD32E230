/*!
    \file  sysConfig.h
    \brief System and peripherial configuration
*/

#ifndef _SYSCONFIG_H_
#define _SYSCONFIG_H_

#include "gd32e23x.h"

#define EEPROM_ADDR ((uint32_t)0x08003C00U)


//
// System data struct
//
struct system_s
{
    volatile uint8_t uart_tx_buff[2];
    volatile uint8_t uart_rx_buff[1];
    uint32_t uart_baud;
};
typedef struct system_s system_t;

/*!
    \brief      configure the different system clocks
    \param[in]  none
    \param[out] none
    \retval     none
*/
static inline void rcu_config(void)
{
    /* enable GPIO clocks */
    rcu_periph_clock_enable(RCU_GPIOA);
    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART0);
}

/*!
    \brief      configure the NVIC
    \param[in]  none
    \param[out] none
    \retval     none
*/
static inline void nvic_config(void)
{
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 2U);
    /* USART interrupt configuration */
    nvic_irq_enable(USART0_IRQn, 0);
}

/*!
    \brief      configure the GPIO
    \param[in]  none
    \param[out] none
    \retval     none
*/
static inline void gpio_config(void)
{
    /* connect port to USARTx_Tx */
    gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_9);
    /* connect port to USARTx_Rx */
    gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_10);
    /* connect port to USARTx_DE */
    gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_1);

    /* configure USART Tx as alternate function push-pull */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_9);

    /* configure USART Rx as alternate function push-pull */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_10);

    /* configure USART DE as alternate function push-pull */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO_PIN_1);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_1);
}

/*!
    \brief      configure the DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
static inline void uart_dma_config(system_t *p)
{
    dma_parameter_struct dma_init_struct;
    /* initialize USART */

    /* USART configure */
    usart_deinit(USART0);
    usart_oversample_config(USART0, USART_OVSMOD_16);
    usart_baudrate_set(USART0, p->uart_baud);
    usart_overrun_disable(USART0);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    /* ~30ns - 1unit, driver en to out delay - 150ns */
    usart_driver_assertime_config(USART0, 8U);
    usart_driver_deassertime_config(USART0, 2U);
    usart_depolarity_config(USART0, USART_DEP_HIGH);
    usart_rs485_driver_enable(USART0);
    usart_enable(USART0);

    /* deinitialize DMA channel3(USART0 tx) */
    dma_deinit(DMA_CH1);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_addr = (uint32_t)p->uart_tx_buff;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = sizeof(p->uart_tx_buff);
    dma_init_struct.periph_addr = ((uint32_t)&USART_TDATA(USART0));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(DMA_CH1, &dma_init_struct);
    /* deinitialize DMA channel4 (USART0 rx) */
    usart_interrupt_enable(USART0, USART_INT_RBNE);
    /* configure DMA mode */
    dma_circulation_disable(DMA_CH1);
    dma_memory_to_memory_disable(DMA_CH1);
    /* USART DMA enable for transmission and reception */
    usart_dma_transmit_config(USART0, USART_DENT_ENABLE);
}


/*!
    \brief      erase fmc pages from FMC_WRITE_START_ADDR to FMC_WRITE_END_ADDR
    \param[in]  none
    \param[out] none
    \retval     none
*/
static inline void fmc_erase_page(uint32_t page_address)
{

    /* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGERR);

    /* erase the flash pages */
    fmc_page_erase(page_address);
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGERR);

    /* lock the main FMC after the erase operation */
    fmc_lock();
}

/*!
    \brief      program fmc word by word from FMC_WRITE_START_ADDR to FMC_WRITE_END_ADDR
    \param[in]  none
    \param[out] none
    \retval     none
*/
static inline void fmc_program(uint32_t address, void *data, uint32_t data_size)
{
    uint32_t *ptr = (uint32_t *)data;
    /* unlock the flash program/erase controller */
    fmc_unlock();
    for (uint32_t i = 0; i < data_size; i += 4)
    {
        /* program flash */
        fmc_word_program((address + i), *(ptr++));
        fmc_flag_clear(FMC_FLAG_END);
        fmc_flag_clear(FMC_FLAG_WPERR);
        fmc_flag_clear(FMC_FLAG_PGERR);
    }

    /* lock the main FMC after the program operation */
    fmc_lock();
}

/*!
    \brief      program fmc word by word from FMC_WRITE_START_ADDR to FMC_WRITE_END_ADDR
    \param[in]  none
    \param[out] none
    \retval     none
*/
static inline void fmc_read(uint32_t address, void *data, uint32_t data_size)
{
    uint32_t *ptr = (uint32_t *)data;

    for (uint32_t i = 0; i < data_size; i += 4)
    {
        *(ptr++) = *(uint32_t *)(address + i);
    }
}

/*!
    \brief      send new packet via UART using DMA
    \param[in]  buff: data buffer
    \param[out] none
    \retval     none
*/
static inline void usart_dma_startTx(uint32_t buff, uint32_t size)
{
    dma_flag_clear(DMA_CH1, DMA_INTF_GIF);
    usart_interrupt_flag_clear(USART0, USART_INT_FLAG_TC);
    dma_channel_disable(DMA_CH1);
    /* re-enable transmition */
    dma_memory_address_config(DMA_CH1, buff);
    dma_transfer_number_config(DMA_CH1, size);
    dma_channel_enable(DMA_CH1);
}

/*!
    \brief      receive new packet via UART using DMA
    \param[in]  buff: data buffer
    \param[out] none
    \retval     none
*/
static inline void usart_dma_startRx(uint32_t buff, uint32_t size)
{
    /* re-enable transmition */
    usart_receive_config(USART0, USART_RECEIVE_DISABLE);
    dma_memory_address_config(DMA_CH2, buff);
    dma_transfer_number_config(DMA_CH2, size);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    dma_channel_enable(DMA_CH2);
}

/*!
    \brief      config all system modules
    \param[in]  p: pointer to system data struct
    \param[out] none
    \retval     none
*/
static inline void system_start(system_t *p)
{
    /* init system data */
    p->uart_baud = 1778000U;
    /* enable peripherial */
    rcu_config();
    nvic_config();
    gpio_config();
    uart_dma_config(p);
}

#endif // _SYSCONFIG_H_