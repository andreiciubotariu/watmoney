#include "balance_display.h"
#include "message_display.h"
#include "percentage_display.h"

#include <pebble.h>

#define WATMONEY_CALC_PERCENT(meal, flex) (100 * flex / (meal + flex))
#define WATMONEY_GLANCE_SUBTITLE_BUFFER_SIZE (50)

typedef struct AnimationData {
  unsigned int starting_meal_balance;
  int meal_balance_delta;
  unsigned int starting_flex_balance;
  int flex_balance_delta;
  unsigned int starting_percentage;
  int percent_delta;
} AnimationData;

typedef struct WatMoneyData {
	Window *main_window;
  BalanceDisplay *balance_display;
  MessageDisplay *message_display;
  PercentageDisplay *percentage_display;
  AnimationData animation_data;
  bool is_js_ready; // checked when sending appmessages
  unsigned int meal_balance;
  unsigned int flex_balance;
  char glance_subtitle_buffer[WATMONEY_GLANCE_SUBTITLE_BUFFER_SIZE];
} WatMoneyData;

static WatMoneyData *s_data;

static void prv_request_refresh(void);

static void prv_animation_update(Animation *animation, const AnimationProgress progress) {
  const AnimationData *data = &(s_data->animation_data);
  unsigned int anim_meal_balance = data->starting_meal_balance +
      (data->meal_balance_delta * progress / ANIMATION_NORMALIZED_MAX);
  unsigned int anim_flex_balance = data->starting_flex_balance +
      (data->flex_balance_delta * progress / ANIMATION_NORMALIZED_MAX);

  balance_display_set_amounts(s_data->balance_display, anim_meal_balance, anim_flex_balance);
  unsigned int percentage = data->starting_percentage +
      (data->percent_delta * progress / ANIMATION_NORMALIZED_MAX);

  percentage_display_set_percentage(s_data->percentage_display, percentage);
}

static void prv_animation_teardown(Animation *animation) {
  animation_destroy(animation);
  balance_display_set_amounts(s_data->balance_display, s_data->meal_balance, s_data->flex_balance);
  percentage_display_set_percentage(s_data->percentage_display,
                                    WATMONEY_CALC_PERCENT(s_data->meal_balance,
                                                          s_data->flex_balance));
}

static void prv_animation_create() {
  Animation *animation = animation_create();
  animation_set_duration(animation, 2000);
  static AnimationImplementation impl = {
    .update = prv_animation_update,
    .teardown = prv_animation_teardown,
  };
  animation_set_implementation(animation, &impl);
  animation_schedule(animation);
}

static void prv_update_app_glance(AppGlanceReloadSession *session, size_t limit, void *context) {
  if (limit < 1) {
    return;
  }
  const char *glance_text = context;
  const AppGlanceSlice entry = (AppGlanceSlice) {
    .layout = {
      .icon = PUBLISHED_ID_DOLLAR_SIGN_ICON,
      .subtitle_template_string = glance_text,
    },
    .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION,
  };

  const AppGlanceResult result = app_glance_add_slice(session, entry);
  if (result != APP_GLANCE_RESULT_SUCCESS) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Could not add AppGlance, Error %d", result);
  }
}

static void prv_inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Inbox received");
  animation_unschedule_all();

  s_data->glance_subtitle_buffer[0] = '\0';

  Tuple *error = dict_find(iterator, MESSAGE_KEY_Error);
  if (error) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Error retrieving balance");
    goto fail;
  }

  Tuple *js_ready = dict_find(iterator, MESSAGE_KEY_JSReady);
  if (js_ready) {
    s_data->is_js_ready = true;
    prv_request_refresh();
    return;
  }

  Tuple *meal_balance_tuple = dict_find(iterator, MESSAGE_KEY_MealBalance);
  if (!meal_balance_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "No meal balance, cannot proceed");
    goto fail;
  }

  Tuple *flex_balance_tuple = dict_find(iterator, MESSAGE_KEY_FlexBalance);
  if (!flex_balance_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "No flex balance, cannot proceed");
    goto fail;
  }
  goto process_data;

fail:
  message_display_show_error_text(s_data->message_display);
  return;

process_data:
  message_display_hide(s_data->message_display);
  const unsigned int meal_balance = meal_balance_tuple->value->uint32;
  const unsigned int flex_balance = flex_balance_tuple->value->uint32;

  if (meal_balance == 0 && meal_balance == 0) {
    balance_display_set_amounts(s_data->balance_display, s_data->meal_balance,
                                s_data->flex_balance);
    percentage_display_set_percentage(s_data->percentage_display, 0);
  } else {
    AnimationData *data = &(s_data->animation_data);
    data->starting_meal_balance = s_data->meal_balance;
    data->meal_balance_delta = (meal_balance - s_data->meal_balance);

    data->starting_flex_balance = s_data->flex_balance;
    data->flex_balance_delta = (flex_balance - s_data->flex_balance);

    data->starting_percentage = WATMONEY_CALC_PERCENT(s_data->meal_balance, s_data->flex_balance);
    data->percent_delta = (WATMONEY_CALC_PERCENT(meal_balance, flex_balance) -
                           data->starting_percentage);
    if ((data->meal_balance_delta != 0) || (data->flex_balance_delta != 0)) {
      prv_animation_create();
    }
  }
  s_data->meal_balance = meal_balance;
  s_data->flex_balance = flex_balance;
  persist_write_data(MESSAGE_KEY_MealBalance, &(s_data->meal_balance),
                     sizeof(s_data->meal_balance));
  persist_write_data(MESSAGE_KEY_FlexBalance, &(s_data->flex_balance),
                     sizeof(s_data->flex_balance));

  snprintf(s_data->glance_subtitle_buffer, WATMONEY_GLANCE_SUBTITLE_BUFFER_SIZE,
           "Meal: %u | Flex: %u", s_data->meal_balance, s_data->flex_balance);
}

static void prv_inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped (%u)", reason);
}

static void prv_show_no_connection_text_if_needed(bool *showed_text_out) {
  bool is_connected = connection_service_peek_pebble_app_connection();
  if (!is_connected) {
    message_display_show_not_connected_text(s_data->message_display);
  }
  *showed_text_out = !is_connected;
}

static void prv_request_refresh(void) {
  bool showed_no_connection_text;
  prv_show_no_connection_text_if_needed(&showed_no_connection_text);
  if (showed_no_connection_text) {
    return;
  }
  if (!s_data->is_js_ready) {
    return;
  }
  message_display_show_wait_text(s_data->message_display);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, MESSAGE_KEY_RequestRefresh, 1);
  app_message_outbox_send();
}

static void prv_outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason,
                                       void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox send failed (%u), retrying", reason);
  prv_request_refresh();
}

static void prv_outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  prv_request_refresh();
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
}

static void prv_window_load(Window *window) {
  const GColor background_color = PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite);
  window_set_background_color(window, background_color);
  const GRect root_layer_bounds = layer_get_bounds(window_get_root_layer(window));
  GRect percentage_display_bounds = root_layer_bounds;
  GRect balance_display_bounds = root_layer_bounds;
  const int balance_display_height_reduction = PBL_IF_RECT_ELSE(65, 80);
  balance_display_bounds.size.h -= balance_display_height_reduction;
#if defined(PBL_ROUND)
  const int balance_display_bounds_top_margin = 30;
  balance_display_bounds.origin.y += balance_display_bounds_top_margin;

  const int balance_display_side_padding = 25;
  balance_display_bounds = grect_inset(balance_display_bounds,
                                       GEdgeInsets(0, balance_display_side_padding, 0));
#else
  const int percentage_display_height = 50;
  percentage_display_bounds.origin.y = percentage_display_bounds.size.h - percentage_display_height;
  percentage_display_bounds.size.h = percentage_display_height;
#endif

  s_data->balance_display = balance_display_create(&balance_display_bounds);
  layer_add_child(window_get_root_layer(window),
                  balance_display_get_layer(s_data->balance_display));

  s_data->percentage_display = percentage_display_create(&percentage_display_bounds);
  percentage_display_set_colors(s_data->percentage_display, GColorOrange, GColorImperialPurple);
  layer_add_child(window_get_root_layer(window),
                  percentage_display_get_layer(s_data->percentage_display));

  balance_display_set_amounts(s_data->balance_display, s_data->meal_balance, s_data->flex_balance);
  percentage_display_set_percentage(s_data->percentage_display,
                                    WATMONEY_CALC_PERCENT(s_data->meal_balance,
                                                          s_data->flex_balance));

  s_data->message_display = message_display_create(&root_layer_bounds, background_color);
  message_display_hide(s_data->message_display);
  layer_add_child(window_get_root_layer(window),
                  message_display_get_layer(s_data->message_display));

  bool showed_no_connection_text;
  prv_show_no_connection_text_if_needed(&showed_no_connection_text);
  if (!showed_no_connection_text) {
    message_display_show_wait_text(s_data->message_display);
  }
}

static void prv_window_unload(Window *window) {
  balance_display_destroy(s_data->balance_display);
  percentage_display_destroy(s_data->percentage_display);
  message_display_destroy(s_data->message_display);
}

static void prv_init(void) {
  s_data = malloc(sizeof(WatMoneyData));
  *s_data = (WatMoneyData){0};

  s_data->main_window = window_create();
  window_set_window_handlers(s_data->main_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload
  });
  window_set_click_config_provider(s_data->main_window, prv_click_config_provider);

  persist_read_data(MESSAGE_KEY_MealBalance, &(s_data->meal_balance), sizeof(s_data->meal_balance));
  persist_read_data(MESSAGE_KEY_FlexBalance, &(s_data->flex_balance), sizeof(s_data->flex_balance));

  app_message_register_inbox_received(prv_inbox_received_callback);
  app_message_register_inbox_dropped(prv_inbox_dropped_callback);
  app_message_register_outbox_failed(prv_outbox_failed_callback);
  app_message_register_outbox_sent(prv_outbox_sent_callback);

  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);

  window_stack_push(s_data->main_window, true /* animated */);
}

static void prv_deinit(void) {
  app_glance_reload(prv_update_app_glance, s_data->glance_subtitle_buffer);
  animation_unschedule_all();
  window_stack_pop_all(false);
  window_destroy(s_data->main_window);
  app_message_deregister_callbacks();
  free(s_data);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
