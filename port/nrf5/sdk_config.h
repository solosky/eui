#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

/* ==========================================================
 * Minimal sdk_config.h for EUI nRF5 port
 * Enables: TWI/TWIM, SPI/SPIM, GPIO, APP_TIMER, Delay
 * ========================================================== */

/* ── Clock ───────────────────────────────────────────────── */
#ifndef NRFX_CLOCK_ENABLED
#define NRFX_CLOCK_ENABLED 1
#endif

#ifndef NRFX_CLOCK_CONFIG_LF_SRC
#define NRFX_CLOCK_CONFIG_LF_SRC 0
#endif

#ifndef NRFX_CLOCK_CONFIG_IRQ_PRIORITY
#define NRFX_CLOCK_CONFIG_IRQ_PRIORITY 6
#endif

#ifndef NRFX_CLOCK_CONFIG_LOG_ENABLED
#define NRFX_CLOCK_CONFIG_LOG_ENABLED 0
#endif

#ifndef NRFX_CLOCK_CONFIG_LOG_LEVEL
#define NRFX_CLOCK_CONFIG_LOG_LEVEL 3
#endif

#ifndef NRFX_CLOCK_CONFIG_INFO_COLOR
#define NRFX_CLOCK_CONFIG_INFO_COLOR 0
#endif

#ifndef NRFX_CLOCK_CONFIG_DEBUG_COLOR
#define NRFX_CLOCK_CONFIG_DEBUG_COLOR 0
#endif

/* ── Power management ────────────────────────────────────── */
#ifndef NRFX_POWER_ENABLED
#define NRFX_POWER_ENABLED 1
#endif

#ifndef NRFX_POWER_CONFIG_IRQ_PRIORITY
#define NRFX_POWER_CONFIG_IRQ_PRIORITY 6
#endif

#ifndef NRFX_POWER_CONFIG_DEFAULT_DCDCEN
#define NRFX_POWER_CONFIG_DEFAULT_DCDCEN 0
#endif

/* ── TWI / TWIM ──────────────────────────────────────────── */
#ifndef TWI_ENABLED
#define TWI_ENABLED 1
#endif

#ifndef TWI0_ENABLED
#define TWI0_ENABLED 1
#endif

#ifndef TWI1_ENABLED
#define TWI1_ENABLED 1
#endif

#ifndef TWI0_USE_EASY_DMA
#define TWI0_USE_EASY_DMA 0
#endif

#ifndef TWI1_USE_EASY_DMA
#define TWI1_USE_EASY_DMA 0
#endif

#ifndef NRFX_TWIM_ENABLED
#define NRFX_TWIM_ENABLED 1
#endif

#ifndef NRFX_TWIM0_ENABLED
#define NRFX_TWIM0_ENABLED 0
#endif

#ifndef NRFX_TWIM1_ENABLED
#define NRFX_TWIM1_ENABLED 0
#endif

#ifndef NRFX_TWIM_DEFAULT_CONFIG_FREQUENCY
#define NRFX_TWIM_DEFAULT_CONFIG_FREQUENCY 26738688
#endif

#ifndef NRFX_TWIM_DEFAULT_CONFIG_HOLD_BUS_UNINIT
#define NRFX_TWIM_DEFAULT_CONFIG_HOLD_BUS_UNINIT 0
#endif

#ifndef NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY 6
#endif

#ifndef NRFX_TWIM_CONFIG_LOG_ENABLED
#define NRFX_TWIM_CONFIG_LOG_ENABLED 0
#endif

#ifndef NRFX_TWIM_CONFIG_LOG_LEVEL
#define NRFX_TWIM_CONFIG_LOG_LEVEL 3
#endif

#ifndef NRFX_TWIM_CONFIG_INFO_COLOR
#define NRFX_TWIM_CONFIG_INFO_COLOR 0
#endif

#ifndef NRFX_TWIM_CONFIG_DEBUG_COLOR
#define NRFX_TWIM_CONFIG_DEBUG_COLOR 0
#endif

#ifndef NRFX_TWI_ENABLED
#define NRFX_TWI_ENABLED 1
#endif

#ifndef NRFX_TWI0_ENABLED
#define NRFX_TWI0_ENABLED 1
#endif

#ifndef NRFX_TWI1_ENABLED
#define NRFX_TWI1_ENABLED 1
#endif

#ifndef NRFX_TWI_DEFAULT_CONFIG_FREQUENCY
#define NRFX_TWI_DEFAULT_CONFIG_FREQUENCY 26738688
#endif

#ifndef NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT
#define NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT 0
#endif

#ifndef NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY 6
#endif

#ifndef NRFX_TWI_CONFIG_LOG_ENABLED
#define NRFX_TWI_CONFIG_LOG_ENABLED 0
#endif

#ifndef NRFX_TWI_CONFIG_LOG_LEVEL
#define NRFX_TWI_CONFIG_LOG_LEVEL 3
#endif

#ifndef NRFX_TWI_CONFIG_INFO_COLOR
#define NRFX_TWI_CONFIG_INFO_COLOR 0
#endif

#ifndef NRFX_TWI_CONFIG_DEBUG_COLOR
#define NRFX_TWI_CONFIG_DEBUG_COLOR 0
#endif

/* ── SPI / SPIM ──────────────────────────────────────────── */
#ifndef SPI_ENABLED
#define SPI_ENABLED 1
#endif

#ifndef SPI0_ENABLED
#define SPI0_ENABLED 1
#endif

#ifndef SPI1_ENABLED
#define SPI1_ENABLED 1
#endif

#ifndef SPI2_ENABLED
#define SPI2_ENABLED 1
#endif

#ifndef SPI0_USE_EASY_DMA
#define SPI0_USE_EASY_DMA 0
#endif

#ifndef SPI1_USE_EASY_DMA
#define SPI1_USE_EASY_DMA 0
#endif

#ifndef SPI2_USE_EASY_DMA
#define SPI2_USE_EASY_DMA 0
#endif

#ifndef NRFX_SPIM_ENABLED
#define NRFX_SPIM_ENABLED 1
#endif

#ifndef NRFX_SPIM0_ENABLED
#define NRFX_SPIM0_ENABLED 0
#endif

#ifndef NRFX_SPIM1_ENABLED
#define NRFX_SPIM1_ENABLED 0
#endif

#ifndef NRFX_SPIM2_ENABLED
#define NRFX_SPIM2_ENABLED 0
#endif

#ifndef NRFX_SPIM_MISO_PULL_CFG
#define NRFX_SPIM_MISO_PULL_CFG 1
#endif

#ifndef NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY 6
#endif

#ifndef NRFX_SPIM_CONFIG_LOG_ENABLED
#define NRFX_SPIM_CONFIG_LOG_ENABLED 0
#endif

#ifndef NRFX_SPIM_CONFIG_LOG_LEVEL
#define NRFX_SPIM_CONFIG_LOG_LEVEL 3
#endif

#ifndef NRFX_SPIM_CONFIG_INFO_COLOR
#define NRFX_SPIM_CONFIG_INFO_COLOR 0
#endif

#ifndef NRFX_SPIM_CONFIG_DEBUG_COLOR
#define NRFX_SPIM_CONFIG_DEBUG_COLOR 0
#endif

#ifndef NRFX_SPIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED
#define NRFX_SPIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED 0
#endif

#ifndef NRFX_SPI_ENABLED
#define NRFX_SPI_ENABLED 1
#endif

#ifndef NRFX_SPI0_ENABLED
#define NRFX_SPI0_ENABLED 1
#endif

#ifndef NRFX_SPI1_ENABLED
#define NRFX_SPI1_ENABLED 1
#endif

#ifndef NRFX_SPI2_ENABLED
#define NRFX_SPI2_ENABLED 1
#endif

#ifndef NRFX_SPI_MISO_PULL_CFG
#define NRFX_SPI_MISO_PULL_CFG 1
#endif

#ifndef NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY 6
#endif

#ifndef NRFX_SPI_CONFIG_LOG_ENABLED
#define NRFX_SPI_CONFIG_LOG_ENABLED 0
#endif

#ifndef NRFX_SPI_CONFIG_LOG_LEVEL
#define NRFX_SPI_CONFIG_LOG_LEVEL 3
#endif

#ifndef NRFX_SPI_CONFIG_INFO_COLOR
#define NRFX_SPI_CONFIG_INFO_COLOR 0
#endif

#ifndef NRFX_SPI_CONFIG_DEBUG_COLOR
#define NRFX_SPI_CONFIG_DEBUG_COLOR 0
#endif

#ifndef SPI_DEFAULT_CONFIG_IRQ_PRIORITY
#define SPI_DEFAULT_CONFIG_IRQ_PRIORITY 6
#endif

#ifndef TWI_DEFAULT_CONFIG_IRQ_PRIORITY
#define TWI_DEFAULT_CONFIG_IRQ_PRIORITY 6
#endif

/* Use EasyDMA (SPIM/TWIM) to avoid shared IRQ conflicts */
#ifndef NRF_DRV_TWI_USE_TWIM
#define NRF_DRV_TWI_USE_TWIM 1
#endif

#ifndef NRF_DRV_SPI_USE_SPIM
#define NRF_DRV_SPI_USE_SPIM 1
#endif

/* ── GPIO ────────────────────────────────────────────────── */
#ifndef NRFX_GPIOTE_ENABLED
#define NRFX_GPIOTE_ENABLED 1
#endif

#ifndef GPIOTE_ENABLED
#define GPIOTE_ENABLED 1
#endif

#ifndef GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS
#define GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 1
#endif

/* ── APP_TIMER ───────────────────────────────────────────── */
#ifndef APP_TIMER_ENABLED
#define APP_TIMER_ENABLED 1
#endif

#ifndef APP_TIMER_CONFIG_RTC_FREQUENCY
#define APP_TIMER_CONFIG_RTC_FREQUENCY 0
#endif

#ifndef APP_TIMER_CONFIG_IRQ_PRIORITY
#define APP_TIMER_CONFIG_IRQ_PRIORITY 6
#endif

#ifndef APP_TIMER_CONFIG_OP_QUEUE_SIZE
#define APP_TIMER_CONFIG_OP_QUEUE_SIZE 2
#endif

#ifndef APP_TIMER_CONFIG_SWI_NUMBER
#define APP_TIMER_CONFIG_SWI_NUMBER 0
#endif

#ifndef APP_TIMER_CONFIG_USE_SCHEDULER
#define APP_TIMER_CONFIG_USE_SCHEDULER 0
#endif

/* ── TIMER ───────────────────────────────────────────────── */
#ifndef TIMER_ENABLED
#define TIMER_ENABLED 1
#endif

#ifndef TIMER0_ENABLED
#define TIMER0_ENABLED 1
#endif

#ifndef TIMER1_ENABLED
#define TIMER1_ENABLED 1
#endif

#ifndef TIMER2_ENABLED
#define TIMER2_ENABLED 1
#endif

#ifndef TIMER3_ENABLED
#define TIMER3_ENABLED 1
#endif

#ifndef TIMER4_ENABLED
#define TIMER4_ENABLED 1
#endif

#ifndef NRFX_TIMER_ENABLED
#define NRFX_TIMER_ENABLED 1
#endif

#ifndef NRFX_TIMER0_ENABLED
#define NRFX_TIMER0_ENABLED 1
#endif

#ifndef NRFX_TIMER1_ENABLED
#define NRFX_TIMER1_ENABLED 1
#endif

#ifndef NRFX_TIMER2_ENABLED
#define NRFX_TIMER2_ENABLED 1
#endif

#ifndef NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY
#define NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY NRF_TIMER_FREQ_1MHz
#endif

#ifndef NRFX_TIMER_DEFAULT_CONFIG_MODE
#define NRFX_TIMER_DEFAULT_CONFIG_MODE NRF_TIMER_MODE_TIMER
#endif

#ifndef NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH
#define NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH NRF_TIMER_BIT_WIDTH_16
#endif

#ifndef NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY 6
#endif

/* ── RTC ─────────────────────────────────────────────────── */
#ifndef RTC_ENABLED
#define RTC_ENABLED 1
#endif

#ifndef RTC0_ENABLED
#define RTC0_ENABLED 1
#endif

#ifndef RTC1_ENABLED
#define RTC1_ENABLED 1
#endif

#ifndef NRFX_RTC_ENABLED
#define NRFX_RTC_ENABLED 1
#endif

#ifndef NRFX_RTC0_ENABLED
#define NRFX_RTC0_ENABLED 1
#endif

#ifndef NRFX_RTC1_ENABLED
#define NRFX_RTC1_ENABLED 1
#endif

#ifndef NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY 6
#endif

/* ── PPI ─────────────────────────────────────────────────── */
#ifndef PPI_ENABLED
#define PPI_ENABLED 1
#endif

#ifndef NRFX_PPI_ENABLED
#define NRFX_PPI_ENABLED 1
#endif

/* ── PRS (Peripheral Resource Sharing) ───────────────────── */
#ifndef NRFX_PRS_ENABLED
#define NRFX_PRS_ENABLED 1
#endif

#ifndef NRFX_PRS_BOX_0_ENABLED
#define NRFX_PRS_BOX_0_ENABLED 0
#endif

#ifndef NRFX_PRS_BOX_1_ENABLED
#define NRFX_PRS_BOX_1_ENABLED 0
#endif

#ifndef NRFX_PRS_BOX_2_ENABLED
#define NRFX_PRS_BOX_2_ENABLED 0
#endif

#ifndef NRFX_PRS_BOX_3_ENABLED
#define NRFX_PRS_BOX_3_ENABLED 0
#endif

#ifndef NRFX_PRS_BOX_4_ENABLED
#define NRFX_PRS_BOX_4_ENABLED 1
#endif

/* ── NVMC ────────────────────────────────────────────────── */
#ifndef NRFX_NVMC_ENABLED
#define NRFX_NVMC_ENABLED 1
#endif

/* ── Logging (disabled) ──────────────────────────────────── */
#ifndef NRF_LOG_ENABLED
#define NRF_LOG_ENABLED 0
#endif

#ifndef NRF_LOG_BACKEND_UART_ENABLED
#define NRF_LOG_BACKEND_UART_ENABLED 0
#endif

#ifndef NRFX_TWIM_CONFIG_LOG_ENABLED
#define NRFX_TWIM_CONFIG_LOG_ENABLED 0
#endif

#ifndef NRFX_TWIM_CONFIG_LOG_LEVEL
#define NRFX_TWIM_CONFIG_LOG_LEVEL 3
#endif

#endif /* SDK_CONFIG_H */
