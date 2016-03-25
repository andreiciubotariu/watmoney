#include "percentage_display.h"

#include <pebble.h>

#define INSET_THICKNESS (15)
#define INNER_RING_OFFSET_THICKNESS (INSET_THICKNESS - 1)
#define BOUNDARY_COLOR (GColorBlack)

struct PercentageDisplay {
  Layer *layer;
  unsigned int percent;
  GColor primary_color;
  GColor secondary_color;
};

static PercentageDisplay *s_percentage_display;

#if defined(PBL_ROUND)
// Angles in degrees
static void prv_fill_radial(GContext *ctx, GColor color, GRect *box, int32_t start_angle,
                            int32_t end_angle) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_radial(ctx, *box, GOvalScaleModeFitCircle, INSET_THICKNESS,
                       DEG_TO_TRIGANGLE(start_angle), DEG_TO_TRIGANGLE(end_angle));
}

static void prv_update_proc(Layer *layer, GContext *context) {
  PercentageDisplay *display = layer_get_data(layer);
  GRect box = layer_get_bounds(layer);
  prv_fill_radial(context, display->secondary_color, &box, 0, 360);

  int32_t percent_degree = (display->percent / 100.0) * 360;
  prv_fill_radial(context, display->primary_color, &box, 0, percent_degree);
  prv_fill_radial(context, BOUNDARY_COLOR, &box, percent_degree, percent_degree + 1);
  prv_fill_radial(context, BOUNDARY_COLOR, &box, 0, 1);

  box.size.h -= 2 * INNER_RING_OFFSET_THICKNESS;
  box.origin.y += INNER_RING_OFFSET_THICKNESS;
  graphics_context_set_fill_color(context, BOUNDARY_COLOR);
  graphics_fill_radial(context, box, GOvalScaleModeFitCircle, 3 /*inset*/, 0, TRIG_MAX_ANGLE);
}
#else
static void prv_fill_rect(GContext *ctx, GColor color, GRect *box) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, *box, 0, GCornerNone);
}
static void prv_update_proc(Layer *layer, GContext *context) {
  PercentageDisplay *display = layer_get_data(layer);
  GRect box = layer_get_bounds(layer);
  prv_fill_rect(context, GColorImperialPurple, &box);

  int32_t percent_box = (display->percent / 100.0) * box.size.w;
  box.origin.x = box.size.w - percent_box;
  prv_fill_rect(context, GColorOrange, &box);

  box.origin.x -= 1;
  box.size.w = 2;
  prv_fill_rect(context, GColorBlack, &box);

  box = layer_get_bounds(layer);
  box.size.h = 2;
  prv_fill_rect(context, GColorBlack, &box);
}
#endif

PercentageDisplay *percentage_display_create(const GRect *bounds) {
  Layer *layer = layer_create_with_data(*bounds, sizeof(PercentageDisplay));
  PercentageDisplay *data = layer_get_data(layer);
  data->layer = layer;

  layer_set_update_proc(layer, prv_update_proc);
  return data;
}

Layer *percentage_display_get_layer(const PercentageDisplay *display) {
  return display->layer;
}

void percentage_display_set_percentage(PercentageDisplay *display, unsigned int percent) {
  display->percent = percent;
  layer_mark_dirty(display->layer);
}

void percentage_display_set_colors(PercentageDisplay *display, GColor primary, GColor secondary) {
  display->primary_color = primary;
  display->secondary_color = secondary;
}

void percentage_display_destroy(const PercentageDisplay *display) {
  layer_destroy(display->layer); // display freed as part of layer_destroy
}
