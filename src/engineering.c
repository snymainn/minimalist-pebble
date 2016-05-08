#include <pebble.h>
#include <inttypes.h>
#include "engineering.h"

static Window *window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_num_label;

static GPath *s_minute_arrow, *s_hour_arrow;
static char s_date_buffer[7], s_temp_buffer[5];

static AppSync s_sync;
static uint8_t s_sync_buffer[64];

static GColor gcolor_background, gcolor_hour_marks, gcolor_minute_marks, gcolor_numbers, gcolor_hour_hand, gcolor_minute_hand;
static bool b_inverse_when_disconnected, b_show_battery_status;

static void load_persisted_values() {

	// INVERSE IF BLUETOOTH DISCONNECTED
	if (persist_exists(KEY_INVERSE_WHEN_DISCONNECTED)) {
	  b_inverse_when_disconnected = persist_read_int(KEY_INVERSE_WHEN_DISCONNECTED);
	}

	// SHOW_BATTERY_STATUS
	if (persist_exists(KEY_SHOW_BATTERY_STATUS)) {
	  b_show_battery_status = persist_read_int(KEY_SHOW_BATTERY_STATUS);
	}
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {

 	Tuple *inverse_when_disconnected_t = dict_find(iter, KEY_INVERSE_WHEN_DISCONNECTED);
	if(inverse_when_disconnected_t) {
		APP_LOG(APP_LOG_LEVEL_INFO, "Inverse when disconnected %d", inverse_when_disconnected_t->value->uint8);
 		b_inverse_when_disconnected = inverse_when_disconnected_t->value->uint8;
		persist_write_int(KEY_INVERSE_WHEN_DISCONNECTED, b_inverse_when_disconnected);
 	}

	Tuple *show_battery_status_t = dict_find(iter, KEY_SHOW_BATTERY_STATUS);
	if(show_battery_status_t) {
		APP_LOG(APP_LOG_LEVEL_INFO, "Show battery_status %d", show_battery_status_t->value->uint8);
 		b_show_battery_status = show_battery_status_t->value->uint8;
		persist_write_int(KEY_SHOW_BATTERY_STATUS, show_battery_status_t->value->uint8);
 	}
	
}

static bool color_hour_marks(GDrawCommand *command, uint32_t index, void *context) {
  gdraw_command_set_stroke_color(command, gcolor_hour_marks);
  return true;
}

static bool color_minute_marks(GDrawCommand *command, uint32_t index, void *context) {
  gdraw_command_set_stroke_color(command, gcolor_minute_marks);
  return true;
}


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
	
#ifdef PBL_RECT
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
#else
	graphics_draw_text(ctx, "12", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(80, 10, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	graphics_draw_text(ctx, "1", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(107, 20, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "2", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(130, 43, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "3", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(140, 74, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "4", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(130, 106, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "5", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(107, 126, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
	graphics_draw_text(ctx, "6", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(81, 136, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	graphics_draw_text(ctx, "7", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(53, 124, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, "8", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(29, 106, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, "9", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(20, 74, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, "10", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(28, 42, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, "11", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(50, 22, 20, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
#endif

}

static void hands_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
//	GPoint center = grect_center_point(&bounds);

	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	// date
	graphics_context_set_text_color(ctx, gcolor_numbers);
	int offset = 0;
#ifdef PBL_RECT
	graphics_draw_text(ctx, s_date_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(80, 75, 40 + offset, 14), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
#else
	graphics_draw_text(ctx, s_date_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(100, 78, 45 + offset, 14), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
#endif

	// minute hand
	graphics_context_set_fill_color(ctx, gcolor_minute_hand);
	gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
	gpath_draw_filled(ctx, s_minute_arrow);

	// hour hand
	graphics_context_set_fill_color(ctx, gcolor_hour_hand);
	gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
	gpath_draw_filled(ctx, s_hour_arrow);


	// dot in the middle
	graphics_fill_circle(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2), 4);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	strftime(s_date_buffer, sizeof(s_date_buffer), "%a %d", t);
	uppercase(s_date_buffer);
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

	load_persisted_values();
}

static void window_unload(Window *window) {
	layer_destroy(s_simple_bg_layer);
	layer_destroy(s_date_layer);

	text_layer_destroy(s_day_label);
	text_layer_destroy(s_num_label);

	layer_destroy(s_hands_layer);
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

	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(64, 64);
}

static void deinit() {
	gpath_destroy(s_minute_arrow);
	gpath_destroy(s_hour_arrow);

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
