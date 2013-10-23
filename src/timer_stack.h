#ifndef TIMER_STACK_H
#define TIMER_STACK_H

#include "pebble_os.h"
#include "pebble_app.h"

////////////////////////////////////////////////////////////////////////
//
// timer_stack.h
//
// This file implements the concept of hierarchical processing of
// Pebble OS timer events via a timer stack.  A stack allows
// independent management of timers by multiple timeout sinks without
// having a single function that undestands all timers in the system.
//
// I wanted to make this because the Pebble app framework only
// supports a single timeout handler on a per-application basis.
// Timer Stack is very similar in concept to the click provider, which
// allows per-window configuration of how button clicks are handled.
//
// The stack contains function pointers, each of which is an
// independent timeout handler.  The function pointer has the
// signature timer_stack_timeout_handler, which means it has the same
// arguments as the app-wide timeout handler, but returns a boolean
// instead of void.
//
// The return value is important - it tells the stack manager whether
// or not to propagate the timeout.  If your handler is called for a
// timer handle that you didn't create, then be sure and return
// 'false'. from your handler.  This tells the stack manager that you
// didn't handle the timeout and that the manager should continue
// asking handlers further down the stack if they want to handle it.
//
// If your handler *does* consume the timeout event, then be sure and
// returh 'false' so that the stack manager halts timeout processing
// and doesn't propagate handling to a handler further down the stack.
//
// If you choose to use timer stack, it will become the root handler
// for all timeouts.  Here's how you use it:
//
// 1.  Call timer_stack_init() once during your app init.
//
// 2.  Set the PebbleAppHandlers.timer_handler to
//     &timer_stack_handle_timeout.
//
// 3.  When your window that has a timer handler associated with it is
//     loaded, call timer_stack_push( <handler> ) with the argument
//     <handler> as the function pointer of your timeout handler.
//     Depending on the behavior of your timers, you may want to
//     push/pop your timeout handler in appear/disappear rather than
//     load/unload.
//
// 4.  In your handler that you've now pushed on the stack, if the
//     timer handle is unknown to you, simply return 'false' so that
//     the manager will propagate the timeout down to other handlers
//     on the stack.  If you *do* recognize the timer handle and
//     respond with some action, be sure and return 'true'.
//
// 5.  When your window is unloaded, call timer_stack_pop().  As per
//     #3 above, you may want to perform these actions on
//     appear()/disappear() rather than load()/unload().  Depends if
//     you want your timers to continue to be active while your window
//     is not on the top of the stack.
//
// Here is the behavior of the timer_stack when a timeout occurs:
//
// 1.  FOR each handler on the stack, starting from the top:
//
//     a.  If handler is non-NULL, call handler, passing in same
//         arguments we got.  (If handler is NULL, just skip.)
//
//     b.  Is handler return value TRUE?
//
//         i.  Yes: we're done, simply 'return'.
//
//         ii. No: This handler didn't consume the timeout.
//
// 2.  return
//
// That's it.

// Set this value to increase or decrease the depth of the timer
// handler stack.  Each entry is only the size of a pointer.
#define TIMER_STACK_MAX_DEPTH (4)

typedef bool (* timer_stack_timeout_handler)( AppContextRef app_ctx,
                                              AppTimerHandle handle,
                                              uint32_t cookie );

// Call this once in your application.
void timer_stack_init_once( void );

// Call this when you want to add a new handler at the current top of
// the stack.  It returns 'false' if the timer stack is full and the
// handler wasn't pushed.
bool timer_stack_push( timer_stack_timeout_handler handler );

// Call this when you want to forget the handler on the top of the
// stack.  It returns 'false' when the stack is already empty, but
// this isn't actually an error.
bool timer_stack_pop();

// This function becomes the app-level timeout handler.  You must set
// the PebbleAppHandlers .timer_handler to this function in pbl_main()
// during app initialization.
void timer_stack_handle_timeout( AppContextRef app_ctx,
                                 AppTimerHandle handle,
                                 uint32_t cookie );


#endif
