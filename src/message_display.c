#include "message_display.h"

#include <pebble.h>

#define MESSAGE_FONT (FONT_KEY_GOTHIC_28_BOLD)
#define MESSAGE_COLOR (GColorBlack)

const char *TEXT_WAIT = "WAIT";
const char *TEXT_ERROR = "ERROR \U0001F625";

struct MessageDisplay {
  Layer *layer;
  GColor background_color;
  const char *message;
};

static int16_t prv_get_text_height(const char *text, const GFont *font, const GRect *box) {
  return graphics_text_layout_get_content_size(text,
                                               *font,
                                               *box,
                                               GTextOverflowModeTrailingEllipsis,
                                               GTextAlignmentCenter).h;
}

static void prv_draw_text(GContext *ctx, const char *text, const GFont *font, const GRect *box) {
   graphics_draw_text(ctx, text, *font, *box, GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentCenter, NULL);
}

static void prv_update_proc(Layer *layer, GContext *context) {
  MessageDisplay *display = layer_get_data(layer);
  GRect box = layer_get_bounds(layer);
  graphics_context_set_fill_color(context, display->background_color);
  graphics_fill_rect(context, box, 0, GCornerNone);

  GFont font = fonts_get_system_font(MESSAGE_FONT);
  int16_t height = prv_get_text_height(display->message, &font, &box);
  box.origin.y = (box.size.h - height) / 2;
  box.size.h = height;
  graphics_context_set_text_color(context, MESSAGE_COLOR);
  prv_draw_text(context, display->message, &font, &box);
}

MessageDisplay *message_display_create(const GRect *bounds, GColor background_color) {
  Layer *layer = layer_create_with_data(*bounds, sizeof(MessageDisplay));
  MessageDisplay *data = (MessageDisplay *) layer_get_data(layer);
  *data = (MessageDisplay){0};
  data->layer = layer;
  data->background_color = background_color;
  layer_set_update_proc(layer, prv_update_proc);
  return data;
}

Layer *message_display_get_layer(const MessageDisplay *display) {
  return display->layer;
}

void message_display_hide(const MessageDisplay *display){
  layer_set_hidden(display->layer, true);
}

static void prv_show(const MessageDisplay *display) {
  layer_set_hidden(display->layer, false);
}

void message_display_show_wait_text(MessageDisplay *display) {
  display->message = TEXT_WAIT;
  prv_show(display);
  layer_mark_dirty(display->layer);
}

void message_display_show_error_text(MessageDisplay *display) {
  display->message = TEXT_ERROR;
  prv_show(display);
  layer_mark_dirty(display->layer);
}

void message_display_destroy(const MessageDisplay *display) {
  layer_destroy(display->layer); // display freed as part of layer_destroy
}
