#include <pebble.h>
#include <inttypes.h>
#include "engineering.h"

static Window *window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer, *s_battery_layer;
static TextLayer *s_day_label, *s_num_label;

static GPath *s_minute_arrow, *s_hour_arrow;
static char s_date_buffer[7], s_temp_buffer[5];

//static AppSync s_sync;
//static uint8_t s_sync_buffer[64];

static GColor gcolor_background, gcolor_hour_marks, gcolor_minute_marks, gcolor_numbers, gcolor_hour_hand, gcolor_minute_hand;
static bool b_inverse_when_disconnected, b_show_battery_status, b_vibrate_on_disconnect;

static int s_battery_level;
static char s_battery_buffer[5];

static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void handle_bluetooth_change(bool connected, bool use_vibration)
{
    // If inversing, then always hide layer, else show depending on connected status
    if (b_inverse_when_disconnected) {
        layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), 1);
    } else {
        layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
    }

    if(!connected && b_vibrate_on_disconnect && use_vibration) {
        // Issue a vibrating alert
        vibes_double_pulse();
    }

    if(!connected && b_inverse_when_disconnected) {
	    gcolor_background = GColorWhite;
	    gcolor_minute_hand = GColorBlack;
	
	#ifdef PBL_COLOR
		gcolor_hour_marks = GColorDarkGray;
		gcolor_minute_marks = GColorLightGray;
		gcolor_numbers = GColorDarkGray;
		gcolor_hour_hand = GColorRed;
	#else
		gcolor_hour_marks = GColorBlack;
		gcolor_minute_marks = GColorBlack;
		gcolor_numbers = GColorBlack;
		gcolor_hour_hand = GColorBlack;
    #endif
        window_set_background_color(window, gcolor_background);    
      	//layer_mark_dirty(window_get_root_layer(window));
    }
    if (connected && b_inverse_when_disconnected) {
    	// Default colors
	    gcolor_background = GColorBlack;
	    gcolor_minute_hand = GColorWhite;
	
	#ifdef PBL_COLOR
		gcolor_hour_marks = GColorLightGray;
		gcolor_minute_marks = GColorDarkGray;
		gcolor_numbers = GColorLightGray;
		gcolor_hour_hand = GColorRed;
	#else
		gcolor_hour_marks = GColorWhite;
		gcolor_minute_marks = GColorWhite;
		gcolor_numbers = GColorWhite;
		gcolor_hour_hand = GColorWhite;
    #endif
        window_set_background_color(window, gcolor_background);
       	//layer_mark_dirty(window_get_root_layer(window));
    }
}


static void bluetooth_callback(bool connected) {    
    handle_bluetooth_change(connected, 1);
}

static void load_persisted_values() {

	// INVERSE IF BLUETOOTH DISCONNECTED
	if (persist_exists(KEY_INVERSE_WHEN_DISCONNECTED)) {
	  b_inverse_when_disconnected = persist_read_bool(KEY_INVERSE_WHEN_DISCONNECTED);
	}
	// SHOW_BATTERY_STATUS
	if (persist_exists(KEY_SHOW_BATTERY_STATUS)) {
	  b_show_battery_status = persist_read_bool(KEY_SHOW_BATTERY_STATUS);
	}

	// VIBRATE ON DISCONNECT
	if (persist_exists(KEY_VIBRATE_ON_DISCONNECT)) {
	  b_vibrate_on_disconnect = persist_read_bool(KEY_VIBRATE_ON_DISCONNECT);
	  
	}
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {

 	Tuple *inverse_when_disconnected_t = dict_find(iter, KEY_INVERSE_WHEN_DISCONNECTED);
	if(inverse_when_disconnected_t) {
 		b_inverse_when_disconnected = inverse_when_disconnected_t->value->uint8;
		persist_write_bool(KEY_INVERSE_WHEN_DISCONNECTED, b_inverse_when_disconnected);
		handle_bluetooth_change(connection_service_peek_pebble_app_connection(), 0); 
 	}

	Tuple *show_battery_status_t = dict_find(iter, KEY_SHOW_BATTERY_STATUS);
	if(show_battery_status_t) {
 		b_show_battery_status = show_battery_status_t->value->uint8;
		persist_write_bool(KEY_SHOW_BATTERY_STATUS, b_show_battery_status);
		// Update meter
        layer_mark_dirty(s_battery_layer);
 	}
	Tuple *vibrate_on_disconnect_t = dict_find(iter, KEY_VIBRATE_ON_DISCONNECT);
	if(vibrate_on_disconnect_t) {
 		b_vibrate_on_disconnect = vibrate_on_disconnect_t->value->uint8;
		persist_write_bool(KEY_VIBRATE_ON_DISCONNECT, b_vibrate_on_disconnect);
 	}	
}

/*
static bool color_hour_marks(GDrawCommand *command, uint32_t index, void *context) {
  gdraw_command_set_stroke_color(command, gcolor_hour_marks);
  return true;
}

static bool color_minute_marks(GDrawCommand *command, uint32_t index, void *context) {
  gdraw_command_set_stroke_color(command, gcolor_minute_marks);
  return true;
}
*/

static int32_t get_angle_for_hour(int hour) {
  // Progress through 12 hours, out of 360 degrees
  return (hour * 360) / 12;
}

static int32_t get_angle_for_minute(int hour) {
  // Progress through 60 miunutes, out of 360 degrees
  return (hour * 360) / 60;
}


static void bg_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	GRect frame = grect_inset(bounds, GEdgeInsets(4 * INSET));
	GRect inner_hour_frame = grect_inset(bounds, GEdgeInsets((4 * INSET) + 8));
	GRect inner_minute_frame = grect_inset(bounds, GEdgeInsets((4 * INSET) + 6));
	
	graphics_context_set_stroke_color(ctx, gcolor_hour_marks);
	graphics_context_set_stroke_width(ctx, 3);

	// Hours marks
	for(int i = 0; i < 12; i++) {
		int hour_angle = get_angle_for_hour(i);
		GPoint p0 = gpoint_from_polar(frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(hour_angle));
		GPoint p1 = gpoint_from_polar(inner_hour_frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(hour_angle));
		graphics_draw_line(ctx, p0, p1);
	}
	
	// Minute Marks
	graphics_context_set_stroke_color(ctx, gcolor_minute_marks);
	graphics_context_set_stroke_width(ctx, 1);
	for(int i = 0; i < 60; i++) {
		if (i % 5) {
			int minute_angle = get_angle_for_minute(i);
			GPoint p0 = gpoint_from_polar(frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(minute_angle));
			GPoint p1 = gpoint_from_polar(inner_minute_frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(minute_angle));
			graphics_draw_line(ctx, p0, p1);
		}
	}
	
	// numbers
	graphics_context_set_text_color(ctx, gcolor_numbers);
	
	graphics_draw_text(ctx, "12", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(63, 18, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	graphics_draw_text(ctx, "1", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(85, 23, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "2", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(104, 43, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "3", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(112, 68, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "4", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(104, 93, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "5", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(85, 110, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "6", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(62, 118, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	graphics_draw_text(ctx, "7", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(39, 110, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, "8", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(20, 93, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, "9", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(14, 68, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, "10", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(20, 43, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, "11", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(39, 23, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
//	GPoint center = grect_center_point(&bounds);

	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	// date
	graphics_context_set_fill_color(ctx, gcolor_minute_hand);
	graphics_fill_circle(ctx,GPoint(bounds.size.w-36, bounds.size.h/2),10);
	graphics_context_set_text_color(ctx, gcolor_background);
	int offset = 0;
	graphics_draw_text(ctx, s_date_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), 
	    GRect(88, 72, 40 + offset, 14), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);


	// minute hand
	graphics_context_set_fill_color(ctx, gcolor_minute_hand);
	graphics_context_set_stroke_color(ctx, gcolor_minute_hand);
	graphics_context_set_stroke_width(ctx,2);
	gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
	//gpath_draw_filled(ctx, s_minute_arrow);
	gpath_draw_outline(ctx, s_minute_arrow);
	
	// hour hand
	graphics_context_set_fill_color(ctx, gcolor_hour_hand);
	//graphics_context_set_stroke_color(ctx, gcolor_hour_hand);
	gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
	gpath_draw_filled(ctx, s_hour_arrow);


	// dot in the middle
	graphics_fill_circle(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2), 4);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	strftime(s_date_buffer, sizeof(s_date_buffer), "%d", t);
	uppercase(s_date_buffer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    // Find the width of the bar
    int width = (int)((((float)s_battery_level)/100.0)*bounds.size.w);

    // Draw the background
    graphics_context_set_fill_color(ctx, gcolor_background);
    graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 1), 0, GCornerNone);

    // Draw the bar
    if (b_show_battery_status) {
        graphics_context_set_fill_color(ctx, gcolor_numbers);
        graphics_fill_rect(ctx, GRect(0, 0, width, 1), 0, GCornerNone);
    }

    if (s_battery_level <= 20) {
        memset(s_battery_buffer, '\0', 5);
        snprintf(s_battery_buffer, 4, "%d", s_battery_level);
        graphics_context_set_text_color(ctx, gcolor_numbers);
        int offset = 20;
        graphics_draw_text(ctx, s_battery_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), 
            GRect(bounds.size.w-offset, bounds.size.h-offset, offset, offset), 
            GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	s_simple_bg_layer = layer_create(bounds);
	layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
	layer_add_child(window_layer, s_simple_bg_layer);

	window_set_background_color(window, gcolor_background);

	s_date_layer = layer_create(bounds);
	layer_set_update_proc(s_date_layer, date_update_proc);
	layer_add_child(window_layer, s_date_layer);

	s_hands_layer = layer_create(bounds);
	layer_set_update_proc(s_hands_layer, hands_update_proc);
	layer_add_child(window_layer, s_hands_layer);
	
	// Create battery meter Layer
    s_battery_layer = layer_create(bounds);
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    layer_add_child(window_layer, s_battery_layer);
    
    // Create the Bluetooth icon GBitmap
    s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

    // Create the BitmapLayer to display the GBitmap
    s_bt_icon_layer = bitmap_layer_create(GRect(0, bounds.size.h-20, 20, 20));
    bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));

	load_persisted_values();

    // Ensure battery level is displayed from the start
    battery_callback(battery_state_service_peek());

    // Show the correct state of the BT connection from the start
    handle_bluetooth_change(connection_service_peek_pebble_app_connection(), 0);
}

static void window_unload(Window *window) {
	layer_destroy(s_simple_bg_layer);
	layer_destroy(s_date_layer);

	text_layer_destroy(s_day_label);
	text_layer_destroy(s_num_label);

	layer_destroy(s_hands_layer);
	layer_destroy(s_battery_layer);
	
	gbitmap_destroy(s_bt_icon_bitmap);
    bitmap_layer_destroy(s_bt_icon_layer);
}

static void init() {	
	
	// Default colors
	gcolor_background = GColorBlack;
	gcolor_minute_hand = GColorWhite;
	
	#ifdef PBL_COLOR
		gcolor_hour_marks = GColorLightGray;
		gcolor_minute_marks = GColorDarkGray;
		gcolor_numbers = GColorLightGray;
		gcolor_hour_hand = GColorRed;
	#else
		gcolor_hour_marks = GColorWhite;
		gcolor_minute_marks = GColorWhite;
		gcolor_numbers = GColorWhite;
		gcolor_hour_hand = GColorWhite;
    #endif

	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	window_stack_push(window, true);

	s_temp_buffer[0] = '\0';
	s_date_buffer[0] = '\0';

	// init hand paths
	s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
	s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	GPoint center = grect_center_point(&bounds);
	gpath_move_to(s_minute_arrow, center);
	gpath_move_to(s_hour_arrow, center);


    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
    
	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(64, 64);

	// Register for battery level updates
    battery_state_service_subscribe(battery_callback);
    
    // Register for Bluetooth connection updates
    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = bluetooth_callback
    });    
}

static void deinit() {
	gpath_destroy(s_minute_arrow);
	gpath_destroy(s_hour_arrow);
	
	battery_state_service_unsubscribe();
    connection_service_unsubscribe();
	tick_timer_service_unsubscribe();
	window_destroy(window);
}

int main() {
	init();
	app_event_loop();
	deinit();
}

char *uppercase(char *str) {
    for (int i = 0; str[i] != 0; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 0x20;
        }
    }

    return str;
}
