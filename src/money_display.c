#include "money_display.h"

#include <pebble.h>

#define TOTAL_BALANCE_FONT (FONT_KEY_LECO_36_BOLD_NUMBERS)
#define INDIV_BALANCE_FONT (FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM)
#define TEXT_FONT (FONT_KEY_GOTHIC_24_BOLD)
#define STRING_MEAL ("MEAL")
#define STRING_FLEX ("FLEX")
#define TEXT_COLOR (GColorBlack)

struct MoneyDisplay {
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
  MoneyDisplay *display = (MoneyDisplay *) layer_get_data(layer);
  const size_t buffer_size = 10;
  char buffer[buffer_size];
  snprintf(buffer, buffer_size, "%u", (display->meal_balance + display->flex_balance));
  graphics_context_set_text_color(context, TEXT_COLOR);
  const GRect max_bounding_box = layer_get_bounds(layer);
  GRect box = max_bounding_box;
  GFont total_balance_font = fonts_get_system_font(TOTAL_BALANCE_FONT);
  box.size.h = prv_get_text_height(buffer, &total_balance_font, &max_bounding_box);
  prv_draw_text(context, buffer, &total_balance_font, &box);

  snprintf(buffer, buffer_size, "%u", display->meal_balance);
  GFont individual_balance_font = fonts_get_system_font(INDIV_BALANCE_FONT);
  int16_t height = prv_get_text_height(buffer, &individual_balance_font, &max_bounding_box);
  box = max_bounding_box;
  box.size.h = height;
  box.size.w = (max_bounding_box.size.w / 2);
  box.origin.y = max_bounding_box.size.h - box.size.h;
  prv_draw_text(context, buffer, &individual_balance_font, &box);

  snprintf(buffer, buffer_size, STRING_MEAL);
  GFont text_font = fonts_get_system_font(TEXT_FONT);
  int16_t text_height = prv_get_text_height(buffer, &text_font, &max_bounding_box);
  box.size.h = text_height;
  box.origin.y -= box.size.h;
  prv_draw_text(context, buffer, &text_font, &box);
  box.origin.y =+ box.size.h;

  box.size.h = height;
  snprintf(buffer, buffer_size, "%u", display->flex_balance);
  box.origin.x = box.size.w;
  box.origin.y = max_bounding_box.size.h - box.size.h;
  prv_draw_text(context, buffer, &individual_balance_font, &box);

  snprintf(buffer, buffer_size, STRING_FLEX);
  box.size.h = text_height;
  box.origin.y -= box.size.h;
  prv_draw_text(context, buffer, &text_font, &box);
}

MoneyDisplay *money_display_create(const GRect *bounds) {
  Layer *layer = layer_create_with_data(*bounds, sizeof(MoneyDisplay));
  MoneyDisplay *data = (MoneyDisplay *) layer_get_data(layer);
  *data = (MoneyDisplay){0};
  data->layer = layer;
  layer_set_update_proc(layer, prv_update_proc);
  return data;
}

Layer *money_display_get_layer(const MoneyDisplay *display) {
  return display->layer;
}

void money_display_set_amounts(MoneyDisplay *display, unsigned int meal, unsigned int flex) {
  display->meal_balance = meal;
  display->flex_balance = flex;
  layer_mark_dirty(display->layer);
}

void money_display_destroy(const MoneyDisplay *display) {
  layer_destroy(display->layer); // display freed as part of layer_destroy
}
