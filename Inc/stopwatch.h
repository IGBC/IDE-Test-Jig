#include <stdint.h>
#include "stm32f1xx.h"

/**
 * All Macro Library for nano second order blocking delays using the STM32 debug registers,
 * adapted from https://stackoverflow.com/a/19124472 
 */

#define CLK_SPEED         48000000 // EXAMPLE for CortexM4, EDIT as needed

typedef struct {
    uint32_t m_nStart;               //DEBUG Stopwatch start cycle counter value
    uint32_t m_nStop;                //DEBUG Stopwatch stop cycle counter value
} Stopwatch_t;

#define DEMCR_TRCENA    0x01000000

/* Core Debug registers */
#define DEMCR           (*((volatile uint32_t *)0xE000EDFC))
#define DWT_CTRL        (*(volatile uint32_t *)0xe0001000)
#define CYCCNTENA       (1<<0)
#define DWT_CYCCNT      ((volatile uint32_t *)0xE0001004)

#define STOPWATCH_START(STOPWATCH) { STOPWATCH.m_nStart = *DWT_CYCCNT;}
#define STOPWATCH_STOP(STOPWATCH)  { STOPWATCH.m_nStop = *DWT_CYCCNT;}
#define STOPWATCH_CLEAR(STOPWATCH) { STOPWATCH.m_nStop = 0; STOPWATCH.m_nStop = 0; }
#define STOPWATCH_READ_TICKS(STOPWATCH) (STOPWATCH.m_nStop - STOPWATCH.m_nStart)

#define STOPWATCH_GET_TICKS() (*DWT_CYCCNT)
#define STOPWATCH_DELAY(TICKS) { uint32_t end_ticks = (uint32_t)(TICKS) + STOPWATCH_GET_TICKS(); while(1) { if (STOPWATCH_GET_TICKS() >= end_ticks) break; }}
#define STOPWATCH_RESET() { DEMCR |= DEMCR_TRCENA; *DWT_CYCCNT = 0; DWT_CTRL |= CYCCNTENA; }

#define STOPWATCH_NS_TO_TICKS(NS) ((uint32_t)(((unsigned long long)(NS) * (unsigned long long)(CLK_SPEED)) / 1000000000ULL))
#define STOPWATCH_TICKS_TO_NS(TICKS) ((uint32_t)(1000 * (uint32_t)(TICKS) / ((unsigned long long)(CLK_SPEED) / 1000000ULL)))