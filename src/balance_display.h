#pragma once

#include <pebble.h>

struct BalanceDisplay;
typedef struct BalanceDisplay BalanceDisplay;

BalanceDisplay *balance_display_create(const GRect *bounds);

Layer *balance_display_get_layer(const BalanceDisplay *display);

void balance_display_set_amounts(BalanceDisplay *display, unsigned int meal, unsigned int flex);

void balance_display_destroy(const BalanceDisplay *display);
