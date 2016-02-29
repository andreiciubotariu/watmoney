#pragma once

#include <pebble.h>

// Displays a percentage as a bar on rectangular displays and as a radial fill on circular pebbles
// The percentage provided used to display the "rightmost" portion of the layer
struct PercentageDisplay;
typedef struct PercentageDisplay PercentageDisplay;

PercentageDisplay *percentage_display_create(const GRect *bounds);

Layer *percentage_display_get_layer(const PercentageDisplay *display);

void percentage_display_set_percentage(PercentageDisplay *display, unsigned int percent);

void percentage_display_set_colors(PercentageDisplay *display, GColor primary, GColor secondary);

void percentage_display_destroy(const PercentageDisplay *display);
