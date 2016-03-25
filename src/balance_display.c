#include "balance_display.h"

#include <pebble.h>

struct BalanceDisplay {
  Layer *layer;
  unsigned int meal_balance;
  unsigned int flex_balance;
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
  const BalanceDisplay *display = layer_get_data(layer);

  const size_t buffer_size = 10;
  char buffer[buffer_size];
  snprintf(buffer, buffer_size, "%u", (display->meal_balance + display->flex_balance));
  const GColor text_color = GColorBlack;
  graphics_context_set_text_color(context, text_color);
  const GRect max_bounding_box = layer_get_bounds(layer);
  GRect box = max_bounding_box;
  const GFont total_balance_font = fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS);
  box.size.h = prv_get_text_height(buffer, &total_balance_font, &max_bounding_box);
  prv_draw_text(context, buffer, &total_balance_font, &box);

  snprintf(buffer, buffer_size, "%u", display->meal_balance);
  const GFont individual_balance_font = fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
  const int16_t balance_font_height = prv_get_text_height(buffer, &individual_balance_font,
                                                          &max_bounding_box);
  box = max_bounding_box;
  box.size.h = balance_font_height;
  box.size.w = (max_bounding_box.size.w / 2);
  box.origin.y = max_bounding_box.size.h - box.size.h;
  prv_draw_text(context, buffer, &individual_balance_font, &box);

  const char *meal_string = "MEAL";
  snprintf(buffer, buffer_size, meal_string);
  const GFont text_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  const int16_t text_height = prv_get_text_height(buffer, &text_font, &max_bounding_box);
  box.size.h = text_height;
  box.origin.y -= box.size.h;
  prv_draw_text(context, buffer, &text_font, &box);
  box.origin.y =+ box.size.h;

  box.size.h = balance_font_height;
  snprintf(buffer, buffer_size, "%u", display->flex_balance);
  box.origin.x = box.size.w;
  box.origin.y = max_bounding_box.size.h - box.size.h;
  prv_draw_text(context, buffer, &individual_balance_font, &box);

  const char *flex_string = "FLEX";
  snprintf(buffer, buffer_size, flex_string);
  box.size.h = text_height;
  box.origin.y -= box.size.h;
  prv_draw_text(context, buffer, &text_font, &box);
}

BalanceDisplay *balance_display_create(const GRect *bounds) {
  Layer *layer = layer_create_with_data(*bounds, sizeof(BalanceDisplay));
  BalanceDisplay *data = layer_get_data(layer);
  *data = (BalanceDisplay){0};
  data->layer = layer;
  layer_set_update_proc(layer, prv_update_proc);
  return data;
}

Layer *balance_display_get_layer(const BalanceDisplay *display) {
  return display->layer;
}

void balance_display_set_amounts(BalanceDisplay *display, unsigned int meal, unsigned int flex) {
  display->meal_balance = meal;
  display->flex_balance = flex;
  layer_mark_dirty(display->layer);
}

void balance_display_destroy(const BalanceDisplay *display) {
  layer_destroy(display->layer); // display freed as part of layer_destroy
}
