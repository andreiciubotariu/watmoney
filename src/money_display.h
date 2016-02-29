#pragma once

#include <pebble.h>

struct MoneyDisplay;
typedef struct MoneyDisplay MoneyDisplay;

MoneyDisplay *money_display_create(const GRect *bounds);

Layer *money_display_get_layer(const MoneyDisplay *display);

void money_display_set_amounts(MoneyDisplay *display, unsigned int meal, unsigned int flex);

void money_display_destroy(const MoneyDisplay *display);
