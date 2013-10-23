#include "timer_stack.h"
#include "spinner.h"
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include <stdint.h>
#include <stdio.h>

#define MY_UUID { 0xA6, 0x42, 0xF2, 0xC8, 0x2D, 0x04, 0x42, 0x97, 0xA0, 0x31, 0xEB, 0xD6, 0x61, 0x76, 0x16, 0x2B }
PBL_APP_INFO(MY_UUID,
             "Metronome", "Jason Tiller",
             0, 2, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

const uint8_t SCREEN_WIDTH = 144; // px
const uint8_t SCREEN_HEIGHT = 141; // px

uint8_t tempo;
uint8_t min_tempo;
uint8_t max_tempo;
uint32_t beat_interval;

char tempo_str[4];
char size_str[9];

Window window;
TextLayer tempo_layer;
TextLayer up_layer;
TextLayer down_layer;
TextLayer run_layer;
Layer visual_beat_layer;

spinner tempo_spin;

TextLayer bpm_layer;
const char bpm_str[] = "bpm";

uint8_t num_beats;

Window menu_win;
SimpleMenuLayer menu_lay;
void vibe_active_selected( int index, void* context );
void stop_after_selected( int index, void* context );
void fast_increment_selected( int index, void* context );
void vibe_dur_selected( int index, void* context );
const uint8_t VIBE_DUR_INDEX = 1;
const uint8_t STOP_AFTER_INDEX = 2;
SimpleMenuItem menu_items[] = {
   {
      .title = "Vibration",
      .subtitle = "Enabled",
      .callback = (SimpleMenuLayerSelectCallback) &vibe_active_selected,
      .icon = NULL
   },
   {
      .title = "Vibe Duration",
      .subtitle = "50",
      .callback = (SimpleMenuLayerSelectCallback) &vibe_dur_selected,
      .icon = NULL
   },
   {
      .title = "Stop After",
      .subtitle = "Never",
      .callback = (SimpleMenuLayerSelectCallback) &stop_after_selected,
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


AppTimerHandle beat_timer;
AppTimerHandle clear_beat_timer;
AppContextRef my_ctx;

uint8_t running;
uint8_t draw_beat;

uint8_t vibe_enabled;

static uint32_t vibe_segs[] = { 50 };
VibePattern vibe_pat = {
   .durations = vibe_segs,
   .num_segments = ARRAY_LENGTH(vibe_segs)
};

////////////////////////////////////////////////////////////////////////
// Stop after window

Window stop_after_win;
TextLayer stop_after_lay;

char stop_after_str[6];
uint8_t stop_after;
char never_str[] = "Never";
char beats_str[] = "beats";

spinner stop_after_spin;
uint8_t min_stop_after = 0;
uint8_t max_stop_after = 64;
uint8_t init_stop_after = 0;

spinner stop_after_spin;

TextLayer stop_after_title_lay;
InverterLayer stop_after_title_inverter_lay;
const char stop_after_title_str[] = "Stop After";

TextLayer stop_after_bpm_lay;

char* get_str_for_stop_after( void )
{
   if( stop_after == 0 ) {
      return never_str;
   } else {
      snprintf( stop_after_str, 6, "%d", stop_after );
      return stop_after_str;
   }
}

bool should_stop_beating( uint8_t beats_so_far )
{
   if( stop_after > 0 && beats_so_far >= stop_after ) {
      return true;
   }

   return false;
}

void update_stop_after( void )
{
   text_layer_set_text( &stop_after_lay, get_str_for_stop_after() );
   layer_mark_dirty( &stop_after_lay.layer );
}

void stop_after_up( ClickRecognizerRef recognizer,
                    void* context )
{
   if( stop_after < max_stop_after ) {
      stop_after++;
      update_stop_after();
   }
}

void stop_after_down( ClickRecognizerRef recognizer,
                    void* context )
{
   if( stop_after > min_stop_after ) {
      stop_after--;
      update_stop_after();
   }
}

void stop_after_win_appear( Window* win )
{
   spinner_activate();
}

void stop_after_win_disappear( Window* win )
{
   spinner_deactivate( &stop_after_spin );
}

void stop_after_win_init( void )
{
   stop_after = init_stop_after;

   window_init( &stop_after_win, "Stop After" );
   stop_after_win.window_handlers.appear =
      (WindowHandler) &stop_after_win_appear;
   stop_after_win.window_handlers.disappear =
      (WindowHandler) &stop_after_win_disappear;

   spinner_init( &stop_after_spin,
                 &stop_after_win,
                 stop_after_up,
                 stop_after_down,
                 SPIN_NO_ADDITIONAL_CLICK_CONFIG,
                 my_ctx );

   text_layer_init( &stop_after_title_lay,
                    GRect( 0, 0, SCREEN_WIDTH, 28 ) );
   text_layer_set_font( &stop_after_title_lay,
                        fonts_get_system_font( FONT_KEY_ROBOTO_CONDENSED_21 ) );
   text_layer_set_text_alignment( &stop_after_title_lay,
                                  GTextAlignmentCenter );
   text_layer_set_text( &stop_after_title_lay, stop_after_title_str );
   layer_add_child( &stop_after_win.layer, &stop_after_title_lay.layer );

   inverter_layer_init( &stop_after_title_inverter_lay,
                        GRect( 0, 0, SCREEN_WIDTH, 30 ) );
   layer_add_child( &stop_after_win.layer,
                    (Layer*) &stop_after_title_inverter_lay );

   text_layer_init( &stop_after_lay, GRect( 0, 55, SCREEN_WIDTH, 30 ) );
   text_layer_set_font( &stop_after_lay,
                        fonts_get_system_font( FONT_KEY_BITHAM_30_BLACK ) );
   text_layer_set_text_alignment( &stop_after_lay,
                                  GTextAlignmentCenter );
   text_layer_set_text( &stop_after_lay, get_str_for_stop_after() );
   layer_add_child( &stop_after_win.layer, &stop_after_lay.layer );
   
   text_layer_init( &stop_after_bpm_lay, GRect( 0, 85, SCREEN_WIDTH, 25 ) );
   text_layer_set_font( &stop_after_bpm_lay,
                        fonts_get_system_font( FONT_KEY_ROBOTO_CONDENSED_21 ) );
   text_layer_set_text_alignment( &stop_after_bpm_lay,
                                  GTextAlignmentCenter );
   text_layer_set_text( &stop_after_bpm_lay, beats_str );
   layer_add_child( &stop_after_win.layer, &stop_after_bpm_lay.layer );
 }

////////////////////////////////////////////////////////////////////////
// Vibe duration window

Window vibe_dur_win;
TextLayer vibe_dur_lay;

char vibe_dur_str[4];
uint8_t vibe_dur;

spinner vibe_dur_spin;
uint8_t min_vibe_dur = 25;
uint8_t max_vibe_dur = 200;
uint8_t init_vibe_dur = 50;

void vibe_dur_up( ClickRecognizerRef recognizer,
                  void* context )
{
   if( vibe_dur < max_vibe_dur ) {
      vibe_dur++;
      snprintf( vibe_dur_str, 4, "%d", vibe_dur );
      text_layer_set_text( &vibe_dur_lay, vibe_dur_str );
      layer_mark_dirty( &vibe_dur_lay.layer );
   }
}

void vibe_dur_down( ClickRecognizerRef recognizer,
                    void* context )
{
   if( vibe_dur > min_vibe_dur ) {
      vibe_dur--;
      snprintf( vibe_dur_str, 4, "%d", vibe_dur );
      text_layer_set_text( &vibe_dur_lay, vibe_dur_str );
      layer_mark_dirty( &vibe_dur_lay.layer );
   }
}

void vibe_dur_win_appear( Window* win )
{
   spinner_activate();
}

void vibe_dur_win_disappear( Window* win )
{
   spinner_deactivate( &vibe_dur_spin );
}

void update_menu( Window* win )
{
   snprintf( vibe_dur_str, 4, "%d", vibe_dur );
   menu_items[VIBE_DUR_INDEX].subtitle = vibe_dur_str;
   menu_items[STOP_AFTER_INDEX].subtitle = get_str_for_stop_after();
   vibe_segs[0] = vibe_dur;
   layer_mark_dirty( (Layer*) &menu_lay );
}

TextLayer vibe_dur_title_lay;
InverterLayer vibe_dur_title_inverter_lay;
const char vibe_dur_title_str[] = "Vibe Length";

void vibe_dur_win_init( void )
{
   vibe_dur = init_vibe_dur;

   window_init( &vibe_dur_win, "Vibe Dur" );
   vibe_dur_win.window_handlers.appear =
      (WindowHandler) &vibe_dur_win_appear;
   vibe_dur_win.window_handlers.disappear =
      (WindowHandler) &vibe_dur_win_disappear;

   spinner_init( &vibe_dur_spin,
                 &vibe_dur_win,
                 vibe_dur_up,
                 vibe_dur_down,
                 SPIN_NO_ADDITIONAL_CLICK_CONFIG,
                 my_ctx );

   text_layer_init( &vibe_dur_title_lay,
                    GRect( 0, 0, SCREEN_WIDTH, 28 ) );
   text_layer_set_font( &vibe_dur_title_lay,
                        fonts_get_system_font( FONT_KEY_ROBOTO_CONDENSED_21 ) );
   text_layer_set_text_alignment( &vibe_dur_title_lay,
                                  GTextAlignmentCenter );
   text_layer_set_text( &vibe_dur_title_lay, vibe_dur_title_str );
   layer_add_child( &vibe_dur_win.layer, &vibe_dur_title_lay.layer );

   inverter_layer_init( &vibe_dur_title_inverter_lay,
                        GRect( 0, 0, SCREEN_WIDTH, 30 ) );
   layer_add_child( &vibe_dur_win.layer,
                    (Layer*) &vibe_dur_title_inverter_lay );

   text_layer_init( &vibe_dur_lay, GRect( 0, 55, SCREEN_WIDTH, 30 ) );
   text_layer_set_font( &vibe_dur_lay,
                        fonts_get_system_font( FONT_KEY_BITHAM_30_BLACK ) );
   text_layer_set_text_alignment( &vibe_dur_lay,
                                  GTextAlignmentCenter );
   snprintf( vibe_dur_str, 4, "%d", vibe_dur );
   text_layer_set_text( &vibe_dur_lay, vibe_dur_str );
   
   layer_add_child( &vibe_dur_win.layer, &vibe_dur_lay.layer );
 }

////////////////////////////////////////////////////////////////////////

void vibe_dur_selected( int index, void* context )
{
   window_stack_push( &vibe_dur_win, true );
}

void stop_after_selected( int index, void* context )
{
   window_stack_push( &stop_after_win, true );
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

////////////////////////////////////////////////////////////////////////

void update_tempo_layer( uint8_t old_tempo,
                         uint8_t new_tempo )
{
   if( old_tempo != new_tempo ) {
      // NOTE: sprintf() isn't supported - get '_sbrk' errors.
      snprintf( tempo_str, 4, "%d", new_tempo );
      text_layer_set_text( &tempo_layer, tempo_str );
   }
}

void tempo_up( ClickRecognizerRef recognizer,
               Window* win )
{
   uint8_t old_tempo = tempo;
   if( tempo < 255 ) {
      tempo += 1;
   }
   update_tempo_layer( old_tempo, tempo );
}

void tempo_down( ClickRecognizerRef recognizer,
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

void handle_run_click( ClickRecognizerRef recognizer,
                       Window* win );

void beat( void )
{
   if( should_stop_beating( num_beats++ ) ) {
      // This stops beating.
      handle_run_click( 0, 0 );
      return;
   }

   // tempo = beats per min
   // beat_interval = ms per beat
   // 60,000 ms per min
   //
   // ms_per_min / tempo = ms/min / beats/min 
   //                    = ms/beat
   // 
   layer_mark_dirty( &visual_beat_layer );
   draw_beat = 1;
   // This integer division will be off by up to 1ms.
   beat_interval = 60000 / tempo;
   beat_timer = app_timer_send_event( my_ctx, beat_interval, 0 );
   clear_beat_timer = app_timer_send_event( my_ctx,
                                            beat_interval / 2,
                                            0 );
   if( vibe_enabled ) {
      vibes_enqueue_custom_pattern( vibe_pat );
   }
}

void handle_run_click( ClickRecognizerRef recognizer,
                       Window* win )
{
   running = ! running;
   text_layer_set_text( &run_layer, ( running ? "stop" : "start" ) );
   
   if( running ) {
      num_beats = 0;
      beat();
   } else {
      app_timer_cancel_event( my_ctx, beat_timer );
   }
}

bool handle_beat_timeout( AppContextRef app_ctx,
                          AppTimerHandle handle,
                          uint32_t cookie )
{
   if( handle == beat_timer ) {
      beat();
   } else if( handle == clear_beat_timer ) {
      draw_beat = 0;
      layer_mark_dirty( &visual_beat_layer );
   } else {
      return false;
   }

   return true;
}


void config_click_provider( ClickConfig** config,
                            Window* window )
{
   config[BUTTON_ID_SELECT]->click.handler =
      (ClickHandler) &handle_run_click;
   config[BUTTON_ID_SELECT]->long_click.handler =
      (ClickHandler) &switch_to_menu;
   config[BUTTON_ID_SELECT]->long_click.delay_ms = 500;
}

void metronome_win_appear( Window* win )
{
   spinner_activate();
   timer_stack_push( handle_beat_timeout );
}

void metronome_win_disappear( Window* win )
{
   timer_stack_pop();
   spinner_deactivate( &tempo_spin );
}

void metronome_win_lay_out( void )
{
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
  text_layer_set_font( &tempo_layer,
                       fonts_get_system_font( FONT_KEY_BITHAM_42_BOLD ) );

  text_layer_init( &bpm_layer, GRect( xorg, yorg + font_height,
                                      box_w, box_h ) );
  text_layer_set_text_alignment( &bpm_layer,
                                 GTextAlignmentCenter );
  text_layer_set_font( &bpm_layer,
                       fonts_get_system_font( FONT_KEY_ROBOTO_CONDENSED_21 ) );
  text_layer_set_text( &bpm_layer, bpm_str );
  

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
  layer_add_child( &window.layer, &bpm_layer.layer );
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
  window.window_handlers.appear = (WindowHandler) &metronome_win_appear;
  window.window_handlers.disappear = (WindowHandler) &metronome_win_disappear;

  spinner_init( &tempo_spin,
                &window,
                (ClickHandler) tempo_up,
                (ClickHandler) tempo_down,
                (ClickConfigProvider) &config_click_provider,
                my_ctx );

  metronome_win_lay_out();

  window_stack_push(&window, true /* Animated */);

  // Menu configuration.

  window_init(&menu_win, "Menu Win");
  
  simple_menu_layer_init( &menu_lay,
                          GRect( 0, 0, 144, 141 ),
                          &menu_win,
                          menu_sect,
                          ARRAY_LENGTH(menu_sect),
                          NULL );
  menu_win.window_handlers.appear = (WindowHandler) &update_menu;
  layer_add_child( &menu_win.layer, (Layer*) &menu_lay );

  vibe_dur_win_init();

  stop_after_win_init();

  timer_stack_init_once();

  spinner_init_once();
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
     .init_handler = &handle_init,
     .timer_handler = &timer_stack_handle_timeout
     // .timer_handler = &handle_timeout,
  };
  tempo = 96;
  min_tempo = 48;
  max_tempo = 216;
  running = 0;
  vibe_enabled = 1;
  app_event_loop(params, &handlers);
}
