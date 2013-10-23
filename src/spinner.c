#include "spinner.h"
#include "timer_stack.h"

typedef struct {
   Window* win;
   spinner* spin;
} spinner_to_win;

static spinner_to_win spinners[SPINNER_MAX_SPINNERS];

void spinner_config_click_provider( ClickConfig** config,
                                    void* context );

void spinner_up_handler( ClickRecognizerRef recognizer,
                         void* ctx );
void spinner_down_handler( ClickRecognizerRef recognizer,
                           void* ctx );

void spinner_long_up_handler( ClickRecognizerRef recognizer,
                              void* ctx );
void spinner_long_down_handler( ClickRecognizerRef recognizer,
                                void* ctx );

void spinner_long_up_release_handler( ClickRecognizerRef recognizer,
                                      void* ctx );
void spinner_long_down_release_handler( ClickRecognizerRef recognizer,
                                        void* ctx );

bool spinner_handle_timeout( AppContextRef app_ctx,
                             AppTimerHandle handle,
                             uint32_t cookie );

// bool add_spinner( Window* win,
//                   spinner* spin );
// 
// spinner* get_spin_for_win( Window* win );

void spinner_init_once( void )
{
   memset( spinners,
           0, 
           ARRAY_LENGTH(spinners) * sizeof(spinner_to_win) );
}

void spinner_init( spinner* spin,
                   Window* win,
                   ClickHandler up_handler,
                   ClickHandler down_handler,
                   ClickConfigProvider additional_click_config,
                   AppContextRef ctx )
{
   spin->up_handler = up_handler;
   spin->down_handler = down_handler;
   spin->ctx = ctx;

   spin->start_repeat_delay = SPINNER_DEFAULT_REPEAT_DELAY;
   spin->start_fast_repeat_count = SPINNER_DEFAULT_FAST_REPEAT_COUNT;
   spin->repeat_interval = SPINNER_DEFAULT_REPEAT_INTERVAL;
   spin->fast_repeat_interval = SPINNER_DEFAULT_FAST_REPEAT_INTERVAL;

   spin->fast_up_timer = 0;
   spin->fast_down_timer = 0;
   spin->num_fast_changes = 0;
   spin->additional_click_config = additional_click_config;
   
   window_set_click_config_provider_with_context(
      win,
      (ClickConfigProvider) &spinner_config_click_provider,
      (void*) spin );
}

bool spinner_activate( void )
{
   return timer_stack_push( &spinner_handle_timeout );
}

void spinner_deactivate( spinner* spin )
{
   if( spin == 0 ) {
      return;
   }

   if( spin->fast_up_timer != 0 ) {
      app_timer_cancel_event( spin->ctx, spin->fast_up_timer );
   }
   if( spin->fast_down_timer != 0 ) {
      app_timer_cancel_event( spin->ctx, spin->fast_down_timer );
   }

   timer_stack_pop();
}

void spinner_config_click_provider( ClickConfig** config,
                                    void* context )
{
   spinner* spin = (spinner*) context;
   uint32_t repeat_delay;

   if( spin == 0 ) {
      // Something bad has has happened.
      return;
   }

   config[BUTTON_ID_UP]->click.handler =
      (ClickHandler) &spinner_up_handler;
   config[BUTTON_ID_DOWN]->click.handler =
      (ClickHandler) &spinner_down_handler;

   config[BUTTON_ID_UP]->long_click.handler =
      (ClickHandler) &spinner_long_up_handler;
   config[BUTTON_ID_UP]->long_click.release_handler =
      (ClickHandler) &spinner_long_up_release_handler;
   
   config[BUTTON_ID_DOWN]->long_click.handler =
      (ClickHandler) &spinner_long_down_handler;
   config[BUTTON_ID_DOWN]->long_click.release_handler =
      (ClickHandler) &spinner_long_down_release_handler;

   repeat_delay = spin ? spin->start_repeat_delay
                       : SPINNER_DEFAULT_REPEAT_DELAY;

   config[BUTTON_ID_DOWN]->long_click.delay_ms = repeat_delay;
   config[BUTTON_ID_UP]->long_click.delay_ms = repeat_delay;

   if( spin->additional_click_config ) {
      (*spin->additional_click_config)( config, context );
   }
}

bool spinner_handle_timeout( AppContextRef app_ctx,
                             AppTimerHandle handle,
                             uint32_t cookie )
{
   spinner* spin = (spinner*) cookie;
   uint32_t timeout;

   if( spin == 0 ) {
      return false;
   }

   timeout = spin->repeat_interval;

   if(    handle == spin->fast_up_timer
       || handle == spin->fast_down_timer ) {
      if( ++spin->num_fast_changes > spin->start_fast_repeat_count ) {
         timeout = spin->fast_repeat_interval;
      }

      if( handle == spin->fast_up_timer ) {
         spinner_up_handler( (ClickRecognizerRef) NULL,
                             (void*) spin );
         spin->fast_up_timer = app_timer_send_event( app_ctx,
                                                     timeout,
                                                     cookie );
      } else if( handle == spin->fast_down_timer ) {
         spinner_down_handler( (ClickRecognizerRef) NULL,
                               (void*) spin );
         spin->fast_down_timer = app_timer_send_event( app_ctx,
                                                       timeout,
                                                       cookie );
      }
      return true;
   }

   return false;
}

void spinner_up_handler( ClickRecognizerRef recognizer,
                         void* ctx )
{
   spinner* spin = (spinner*) ctx;
   (*spin->up_handler)( recognizer, ctx );
}

void spinner_down_handler( ClickRecognizerRef recognizer,
                           void* ctx )
{
   spinner* spin = (spinner*) ctx;
   (*spin->down_handler)( recognizer, ctx );
}

void spinner_long_up_handler( ClickRecognizerRef recognizer,
                              void* ctx )
{
   spinner* spin = (spinner*) ctx;
   spin->fast_up_timer = app_timer_send_event( spin->ctx,
                                               spin->repeat_interval,
                                               (uint32_t) spin );
   spin->num_fast_changes = 0;
   (*spin->up_handler)( recognizer, ctx );
}

void spinner_long_up_release_handler( ClickRecognizerRef recognizer,
                                      void* ctx )
{
   spinner* spin = (spinner*) ctx;
   app_timer_cancel_event( spin->ctx, spin->fast_up_timer );
}

void spinner_long_down_handler( ClickRecognizerRef recognizer,
                                void* ctx )
{
   spinner* spin = (spinner*) ctx;
   spin->fast_down_timer = app_timer_send_event( spin->ctx,
                                                 spin->repeat_interval,
                                                 (uint32_t) spin );
   spin->num_fast_changes = 0;
   (*spin->down_handler)( recognizer, ctx );
}

void spinner_long_down_release_handler( ClickRecognizerRef recognizer,
                                        void* ctx )
{
   spinner* spin = (spinner*) ctx;
   app_timer_cancel_event( spin->ctx, spin->fast_down_timer );
}
