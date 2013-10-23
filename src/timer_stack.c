////////////////////////////////////////////////////////////////////////
//
// timer_stack.c
//
// Simple timeout handler stack.
//
// See timer_stack.h for more information.
//

#include "timer_stack.h"

static timer_stack_timeout_handler
timer_stack_handler_stack[TIMER_STACK_MAX_DEPTH];

static int curr_stack_depth;

void timer_stack_init_once( void )
{
   curr_stack_depth = 0;
   memset( timer_stack_handler_stack,
           0,
           ARRAY_LENGTH(timer_stack_handler_stack)
              * sizeof(timer_stack_timeout_handler) );
}

bool timer_stack_push( timer_stack_timeout_handler handler )
{
   if( curr_stack_depth < ( TIMER_STACK_MAX_DEPTH - 1 ) ) {
      timer_stack_handler_stack[curr_stack_depth++] = handler;
      return true;
   }

   return false;
}

bool timer_stack_pop( void )
{
   if( curr_stack_depth > 0 ) {
      timer_stack_handler_stack[curr_stack_depth--] = NULL;
      return true;
   }

   return false;
}

void timer_stack_handle_timeout( AppContextRef app_ctx,
                                 AppTimerHandle handle,
                                 uint32_t cookie )
{
   bool handler_ret;
   timer_stack_timeout_handler handler;

   // Bail if the stack is empty.
   if( curr_stack_depth == 0 ) {
      return;
   }

   for( int sp = curr_stack_depth - 1; sp >= 0; sp-- ) {
      handler = timer_stack_handler_stack[sp];
      // Something bad has happened - don't cause a crash.
      if( handler == NULL ) {
         continue;
      }

      // Call this handler.
      handler_ret = (*handler)( app_ctx, handle, cookie );

      // Handler consumed timeout - we're done.
      if( handler_ret ) {
         return;
      }
   }
}
