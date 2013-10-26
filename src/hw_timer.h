#ifndef HW_TIMER_H
#define HW_TIMER_H

#include <stdint.h>

////////////////////////////////////////////////////////////////////////
//
// hw_timer.h
//
// This file contains the declarations for a facility for Pebble
// applications to make use of a high-resolution HW timer.

void hw_timer_init( uint32_t ticks_per_s );

void hw_timer_start( void );

uint32_t hw_timer_get_time( void );

void hw_timer_deinit( void );

#endif
