#include "hw_timer.h"

#define TIM5_CR1 (*(volatile uint32_t*) 0x40000C00)
#define TIM5_CR2 (*(volatile uint32_t*) 0x40000C04)
#define TIM5_SR  (*(volatile uint32_t*) 0x40000C10)
#define TIM5_EGR (*(volatile uint32_t*) 0x40000C14)
#define TIM5_CNT (*(volatile uint32_t*) 0x40000C24)
#define TIM5_PSC (*(volatile uint32_t*) 0x40000C28)
#define TIM5_ARR (*(volatile uint32_t*) 0x40000C2C)

#define RCC_APB1ENR (*(volatile uint32_t*) 0x40023840)
#define RCC_APB1LPENR (*(volatile uint32_t*) 0x40023860)

// This is 32MHz, which I believe is the Pebble Watch system clock
// frequency.
const uint32_t SYSCLK_FREQ = 0x02000000;

void hw_timer_init( uint32_t ticks_per_s )
{
   // Set bit 3 of RCC_APB1ENR high.  This enables TIM5 to receive a
   // sysclock input.
   uint32_t val = RCC_APB1ENR;
   RCC_APB1ENR = val | ( 0b1 << 3 );
   // We also enable it in low-power (sleep) mode.
   val = RCC_APB1LPENR;
   RCC_APB1LPENR = val | ( 0b1 << 3 );
   // Enable the counter and count up.
   TIM5_CR1 = 0x00000005;
   // sysclk / prescale = tick_freq
   // PSC = sysclk / ticks_per_s
   TIM5_PSC = SYSCLK_FREQ / ticks_per_s;
   // Reload at 0xFFFFFFFF - essentially, don't overflow until you
   // count up to full 32-bit value.
   TIM5_ARR = 0xFFFFFFFF;
   // Just to be safe.
   TIM5_CR2 = 0;
   hw_timer_start();
}

void hw_timer_start( void )
{
   // Force an update.
   uint32_t val = TIM5_EGR;
   TIM5_EGR = val | ( 0b1 << 0 );
}

uint32_t hw_timer_get_time( void )
{
   return TIM5_CNT;
}

void hw_timer_deinit( void )
{
   // Disable the hardware counter and power it down.
   TIM5_CR1 = 0x00000000;
   uint32_t val = RCC_APB1ENR;
   RCC_APB1ENR = val & ( ~ (0b1 << 3 ) );
   val = RCC_APB1LPENR;
   RCC_APB1LPENR = val & ( ~ ( 0b1 << 3 ) );
}
