#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include <stdint.h>
#include <stdio.h>

#define MY_UUID { 0xA6, 0x42, 0xF2, 0xC8, 0x2D, 0x04, 0x42, 0x97, 0xA0, 0x31, 0xEB, 0xD6, 0x61, 0x76, 0x16, 0x2B }
PBL_APP_INFO(MY_UUID,
             "Metronome", "CalMan Corp",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

uint8_t tempo;
uint32_t beat_interval;

char tempo_str[4];
char size_str[9];

Window window;
TextLayer tempo_layer;
TextLayer up_layer;
TextLayer down_layer;
TextLayer run_layer;
Layer visual_beat_layer;

Window menu_win;
SimpleMenuLayer menu_lay;
void vibe_active_selected( int index, void* context );
void fast_increment_selected( int index, void* context );
SimpleMenuItem menu_items[] = {
   {
      .title = "Vibration",
      .subtitle = "Enabled",
      .callback = (SimpleMenuLayerSelectCallback) &vibe_active_selected,
      .icon = NULL
   },
   {
      .title = "Fast Increment",
      .subtitle = "Enabled",
      .callback = (SimpleMenuLayerSelectCallback) &fast_increment_selected,
      .icon = NULL
   }
};
SimpleMenuSection menu_sect[] = {
   {
      .items = menu_items,
      .num_items = ARRAY_LENGTH(menu_items),
      .title = "Metronome"
   }
};


AppTimerHandle fast_up_timer;
AppTimerHandle fast_down_timer;
AppTimerHandle beat_timer;
AppTimerHandle clear_beat_timer;
AppContextRef my_ctx;

int num_fast_changes;

uint8_t running;
uint8_t draw_beat;

uint8_t vibe_enabled;
uint8_t fast_roll_enabled;

void fast_increment_selected( int index, void* context )
{
   fast_roll_enabled = ! fast_roll_enabled;
   menu_items[index].subtitle =
      fast_roll_enabled ? "Enabled" : "Disabled";
   layer_mark_dirty( (Layer*) &menu_lay );
}

void switch_to_menu( ClickRecognizerRef recognizer,
                     Window* win )
{
   window_stack_push( &menu_win, true );
}


void vibe_active_selected( int index, void* context )
{
   vibe_enabled = ! vibe_enabled;
   menu_items[index].subtitle =
      vibe_enabled ? "Enabled" : "Disabled";
   layer_mark_dirty( (Layer*) &menu_lay );
}

void start_long_up_click_response( ClickRecognizerRef recognizer,
                                   Window* win )
{
   fast_up_timer = app_timer_send_event( my_ctx, 100, 0 );
   num_fast_changes = 0;
}

void stop_long_up_click_response( ClickRecognizerRef recognizer,
                                  Window* win )
{
   app_timer_cancel_event( my_ctx, fast_up_timer );
}

void start_long_down_click_response( ClickRecognizerRef recognizer,
                                     Window* win )
{
   fast_down_timer = app_timer_send_event( my_ctx, 100, 0 );
   num_fast_changes = 0;
}

void stop_long_down_click_response( ClickRecognizerRef recognizer,
                                    Window* win )
{
   app_timer_cancel_event( my_ctx, fast_down_timer );
}

void update_tempo_layer( uint8_t old_tempo,
                         uint8_t new_tempo )
{
   if( old_tempo != new_tempo ) {
      // NOTE: sprintf() isn't supported - get '_sbrk' errors.
      // NOTE: sprintf() isn't supported - get '_sbrk' errors.
      snprintf( tempo_str, 4, "%d", new_tempo );
      text_layer_set_text( &tempo_layer, tempo_str );
      layer_mark_dirty( &tempo_layer.layer );
   }
}

void handle_up_click( ClickRecognizerRef recognizer,
                      Window* win )
{
   uint8_t old_tempo = tempo;
   if( tempo < 255 ) {
      tempo += 1;
   }
   update_tempo_layer( old_tempo, tempo );
}

void handle_down_click( ClickRecognizerRef recognizer,
                        Window* win )
{
   uint8_t old_tempo = tempo;
   if( tempo > 0 ) {
      tempo -= 1;
   }
   update_tempo_layer( old_tempo, tempo );
}

void draw_visual_beat( Layer* lay, GContext* ctx )
{
   graphics_context_set_fill_color( ctx,
                                    draw_beat ? GColorBlack : GColorClear );   
   graphics_fill_circle( ctx, GPoint( 20, 20 ), 19 );
}


static const uint32_t const vibe_segs[] = { 50 };
VibePattern vibe_pat = {
   .durations = vibe_segs,
   .num_segments = ARRAY_LENGTH(vibe_segs)
};

void handle_run_click( ClickRecognizerRef recognizer,
                       Window* win )
{
   running = ! running;
   layer_mark_dirty( &visual_beat_layer );
   text_layer_set_text( &run_layer, ( running ? "stop" : "start" ) );
   
   // tempo = beats per min
   // beat_interval = ms per beat
   // 60,000 ms per min
   //
   // ms_per_min / tempo = ms/min / beats/min 
   //                    = ms/beat
   // 
   if( running ) {
      draw_beat = 1;
      // This integer division will be off up to 1ms.
      beat_interval = 60000 / tempo;
      beat_timer = app_timer_send_event( my_ctx, beat_interval, 0 );
      clear_beat_timer = app_timer_send_event( my_ctx, 100, 0 );
      if( vibe_enabled ) {
         vibes_enqueue_custom_pattern( vibe_pat );
      }
   } else {
      app_timer_cancel_event( my_ctx, beat_timer );
   }
}

void handle_timeout( AppContextRef app_ctx,
                     AppTimerHandle handle,
                     uint32_t cookie )
{
   int fast_timeout = 100;

   if( handle == fast_up_timer || handle == fast_down_timer ) {
      if( fast_roll_enabled ) {
         if( ++num_fast_changes >= 15 ) {
            fast_timeout = 50;
         }
      }
      if( handle == fast_up_timer ) {
         handle_up_click( (ClickRecognizerRef) NULL,
                          (Window*) NULL );
         fast_up_timer = app_timer_send_event( my_ctx, fast_timeout, 0 );
      } else {
         handle_down_click( (ClickRecognizerRef) NULL,
                            (Window*) NULL );
         fast_down_timer = app_timer_send_event( my_ctx, fast_timeout, 0 );
      }
   } else if( handle == beat_timer ) {
      beat_timer = app_timer_send_event( my_ctx, beat_interval, 0 );
      draw_beat = 1;
      layer_mark_dirty( &visual_beat_layer );
      clear_beat_timer = app_timer_send_event( my_ctx, 100, 0 );
      if( vibe_enabled ) {
         vibes_enqueue_custom_pattern( vibe_pat );
      }
   } else if( handle == clear_beat_timer ) {
      draw_beat = 0;
      layer_mark_dirty( &visual_beat_layer );
   } else {
      // Error.
   }
}


void config_click_provider( ClickConfig** config,
                            Window* window )
{
   config[BUTTON_ID_UP]->click.handler =
      (ClickHandler) &handle_up_click;
   config[BUTTON_ID_DOWN]->click.handler =
      (ClickHandler) &handle_down_click;
   config[BUTTON_ID_SELECT]->click.handler =
      (ClickHandler) &handle_run_click;

   config[BUTTON_ID_UP]->long_click.handler =
      (ClickHandler) &start_long_up_click_response;
   config[BUTTON_ID_UP]->long_click.release_handler =
      (ClickHandler) &stop_long_up_click_response;
   config[BUTTON_ID_UP]->long_click.delay_ms = 500;

   config[BUTTON_ID_DOWN]->long_click.handler =
      (ClickHandler) &start_long_down_click_response;
   config[BUTTON_ID_DOWN]->long_click.release_handler =
      (ClickHandler) &stop_long_down_click_response;
   config[BUTTON_ID_DOWN]->long_click.delay_ms = 500;

   config[BUTTON_ID_SELECT]->long_click.handler =
      (ClickHandler) &switch_to_menu;
   config[BUTTON_ID_SELECT]->long_click.delay_ms = 500;
}

void metronome_win_appear( Window* win )
{
   num_fast_changes = 0;
}

void metronome_win_load( Window* win )
{
  window_set_click_config_provider(
     &window,
     (ClickConfigProvider) &config_click_provider );

  // So, text_align(..., GRect) works lke this:
  //
  // GRect( xorg, yorg, width, height )
  //    xorg is measured from left
  //    yorg is measured from top
  //    the box is width x hight, with its top-left corner at
  //    (xorg,yorg).

  // // Pebble is 144 x 168 (width x height).  In a normal app, with
  // // the bar on top (time, battery status), you have 144 x 131
  // // available for usage.
  // Font is displayed on top of the rect.

  // 144 x 131 available.
  // font is 21-pix high

  // For a 21-pix font.
  int font_height = 21;
  int font_descender_height = 4;
  int screen_width = 144;
  int screen_height = 141;

  // Tempo layer.

  font_height = 42;
  // This number was determined empirically.  There are no font
  // metrics available, so I have to just trial and error to get the
  // right widths for proportional fonts.
  int box_w = 80;
  int box_h = font_height;

  // We want it centered in height but a bit over to the left.
  int xorg = 10;
  int yorg = ( screen_height - box_h ) / 2;

  // This displays X centered in a 30x30 box in the top-left corner.
  // This X is 12 pix wide and 21 pix high.
  text_layer_init( &tempo_layer, GRect( xorg, yorg, box_w, box_h ) );
  text_layer_set_text_alignment( &tempo_layer,
                                 GTextAlignmentCenter );
  snprintf( tempo_str, 4, "%d", 96 );
  text_layer_set_text( &tempo_layer, tempo_str );
  // text_layer_set_font(
  //    &tempo_layer,
  //    fonts_get_system_font( FONT_KEY_ROBOTO_CONDENSED_21 ) );
  text_layer_set_font(
     &tempo_layer,
     fonts_get_system_font( FONT_KEY_BITHAM_42_BOLD ) );

  // Up button layer.

  font_height = 21;
  box_w = 20;
  box_h = font_height + font_descender_height;
  // extra space for lowercase descenders.

  // To put up against far right on top:
  xorg = screen_width - box_w;
  yorg = 0;

  text_layer_init( &up_layer, GRect( xorg, yorg, box_w, box_h ) );
  text_layer_set_text_alignment( &up_layer,
                                 GTextAlignmentRight );
  text_layer_set_text( &up_layer, "up" );
  text_layer_set_font(
     &up_layer,
     fonts_get_system_font( FONT_KEY_ROBOTO_CONDENSED_21 ) );

  // Down button layer.

  box_w = 46;
  box_h = font_height;

  // To put up against far right on bottom:
  xorg = screen_width - box_w;
  yorg = screen_height - box_h;

  text_layer_init( &down_layer, GRect( xorg, yorg, box_w, box_h ) );
  text_layer_set_text_alignment( &down_layer,
                                 GTextAlignmentRight );
  text_layer_set_text( &down_layer, "down" );
  text_layer_set_font(
     &down_layer,
     fonts_get_system_font( FONT_KEY_ROBOTO_CONDENSED_21 ) );

  // Run button layer.

  box_w = 40;
  box_h = font_height + font_descender_height;

  // To put up against far right in middle.
  xorg = screen_width - box_w;
  yorg = ( screen_height - box_h ) / 2;

  text_layer_init( &run_layer, GRect( xorg, yorg, box_w, box_h ) );
  text_layer_set_text_alignment( &run_layer,
                                 GTextAlignmentRight );
  text_layer_set_text( &run_layer, "start" );
  text_layer_set_font(
     &run_layer,
     fonts_get_system_font( FONT_KEY_ROBOTO_CONDENSED_21 ) );

  layer_init( &visual_beat_layer, GRect( 0, 0, 40, 40 ) );
  visual_beat_layer.update_proc = (LayerUpdateProc) &draw_visual_beat;

  layer_add_child( &window.layer, &tempo_layer.layer );
  layer_add_child( &window.layer, &up_layer.layer );
  layer_add_child( &window.layer, &down_layer.layer );
  layer_add_child( &window.layer, &run_layer.layer );
  layer_add_child( &window.layer, &visual_beat_layer );
}

void handle_init(AppContextRef ctx)
{
   my_ctx = ctx;

   // Metronome window.

  window_init(&window, "Metronome Win");
  window.window_handlers.load = (WindowHandler) &metronome_win_load;
  window.window_handlers.appear = (WindowHandler) &metronome_win_appear;

  window_stack_push(&window, true /* Animated */);

  // Menu configuration.

  window_init(&menu_win, "Menu Win");
  vibe_enabled = 1;
  fast_roll_enabled = 1;
  
  simple_menu_layer_init( &menu_lay,
                          GRect( 0, 0, 144, 141 ),
                          &menu_win,
                          menu_sect,
                          ARRAY_LENGTH(menu_sect),
                          NULL );
  layer_add_child( &menu_win.layer, (Layer*) &menu_lay );
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
     .init_handler = &handle_init,
     .timer_handler = &handle_timeout,
  };
  tempo = 96;
  running = 0;
  app_event_loop(params, &handlers);
}
