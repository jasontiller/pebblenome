#include "pebble_os.h"
#include "pebble_app.h"

#ifndef SPINNER_H
#define SPINNER_H

////////////////////////////////////////////////////////////////////////
//
// spinner.h
//
// Implementation of an up-down spinner for Pebble OS using the
// up/down buttons on the Pebble smartwatch.
//
// The spinner is useful when the user is changing a value up or
// down.  Spinner implements single-button pushes for up/down, as well
// as long-push for repeated changes and speedup behavior if the
// button is held down for a certain length of time.
//
// It works via callbacks - the user provides function pointers to
// call when the value goes up or down.
//
// To use this:
//
// 1.  Call spinner_init_once() in your app init function.
//
// 2.  Ensure you've called timer_stack_init() already.
//
// 3.  Call spinner_init() and configure any specific timing-related
//     fields in your window's .appear function.  ONLY INIT THE
//     SPINNER WHEN THE WINDOW IS VISIBLE.
//
// 4.  Call spinner_deinit() when your window .disappears.  DON'T
//     LEAVE THE SPINNER ACTIVE WHEN YOUR WINDOW ISN'T VISIBLE.
//

// Maximum number of spinners that can be initialized simultaneously.
#define SPINNER_MAX_SPINNERS (6)

typedef struct {
   // You can change stuff here.
   int start_repeat_delay;
   int start_fast_repeat_count;

   int repeat_interval;
   int fast_repeat_interval;

   // Don't touch!!
   ClickHandler up_handler;
   ClickHandler down_handler;

   ClickConfigProvider additional_click_config;

   int num_fast_changes;
   AppTimerHandle fast_up_timer;
   AppTimerHandle fast_down_timer;

   AppContextRef ctx;
} spinner;

#define SPINNER_DEFAULT_REPEAT_DELAY (500)  // ms
#define SPINNER_DEFAULT_FAST_REPEAT_COUNT (10) // counts
#define SPINNER_DEFAULT_REPEAT_INTERVAL (100) // ms
#define SPINNER_DEFAULT_FAST_REPEAT_INTERVAL (50) // ms

#define SPIN_NO_ADDITIONAL_CLICK_CONFIG (0)

void spinner_init_once( void );

void spinner_init( spinner* spin,
                   Window* win,
                   ClickHandler up_handler,
                   ClickHandler down_handler,
                   ClickConfigProvider additional_click_config,
                   AppContextRef ctx );

bool spinner_activate( void );

void spinner_deactivate( spinner* spin );

#endif

