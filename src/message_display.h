#pragma once

#include <pebble.h>

struct MessageDisplay;
typedef struct MessageDisplay MessageDisplay;

MessageDisplay *message_display_create(const GRect *bounds, GColor background_color);

Layer *message_display_get_layer(const MessageDisplay *display);

void message_display_hide(const MessageDisplay *display);

void message_display_show_not_connected_text(MessageDisplay *display);

void message_display_show_wait_text(MessageDisplay *display);

void message_display_show_error_text(MessageDisplay *display);

void message_display_destroy(const MessageDisplay *display);
