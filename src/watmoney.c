#include "money_display.h"
#include "percentage_display.h"
#include "message_display.h"
#include "util.h"

#include <pebble.h>

#define CALC_PERCENT(meal, flex) (100 * flex / (meal + flex))

typedef enum AppKey {
 AppKey_RequestRefresh = 0,
 AppKey_Error = 1,
 AppKey_MealBalance = 2,
 AppKey_FlexBalance = 3,
} AppKey;

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
  MoneyDisplay *money_display;
  MessageDisplay *message_display;
  PercentageDisplay *percentage_display;
  AnimationData animation_data;
  unsigned int meal_balance;
  unsigned int flex_balance;
} WatMoneyData;

static WatMoneyData *s_data;

static void prv_request_refresh(void);

static void prv_animation_update(Animation *animation, const AnimationProgress progress) {
  AnimationData *data = &(s_data->animation_data);
  unsigned int anim_meal_balance = data->starting_meal_balance +
      (data->meal_balance_delta * progress / ANIMATION_NORMALIZED_MAX);
  unsigned int anim_flex_balance = data->starting_flex_balance +
      (data->flex_balance_delta * progress / ANIMATION_NORMALIZED_MAX);

  money_display_set_amounts(s_data->money_display, anim_meal_balance, anim_flex_balance);
  unsigned int percentage = data->starting_percentage +
      (data->percent_delta * progress / ANIMATION_NORMALIZED_MAX);

  percentage_display_set_percentage(s_data->percentage_display, percentage);
}

static void prv_animation_teardown(Animation *animation) {
  animation_destroy(animation);
  money_display_set_amounts(s_data->money_display, s_data->meal_balance, s_data->flex_balance);
  percentage_display_set_percentage(s_data->percentage_display,
                                    CALC_PERCENT(s_data->meal_balance, s_data->flex_balance));
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

static void prv_inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Inbox received");
  animation_unschedule_all();

  Tuple *error = dict_find(iterator, AppKey_Error);
  if (error) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Error retrieving balance");
    goto fail;
  }

  Tuple *meal_balance_tuple = dict_find(iterator, AppKey_MealBalance);
  if (!meal_balance_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "No meal balance, cannot proceed");
    goto fail;
  }

  Tuple *flex_balance_tuple = dict_find(iterator, AppKey_FlexBalance);
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
    unsigned int meal_balance = meal_balance_tuple->value->uint32;
    unsigned int flex_balance = flex_balance_tuple->value->uint32;

    if (meal_balance == 0 && meal_balance == 0) {
      money_display_set_amounts(s_data->money_display, s_data->meal_balance, s_data->flex_balance);
      percentage_display_set_percentage(s_data->percentage_display, 0);
    } else {
      AnimationData *data = &(s_data->animation_data);
      data->starting_meal_balance = s_data->meal_balance;
      data->meal_balance_delta = (meal_balance - s_data->meal_balance);

      data->starting_flex_balance = s_data->flex_balance;
      data->flex_balance_delta = (flex_balance - s_data->flex_balance);

      data->starting_percentage = CALC_PERCENT(s_data->meal_balance, s_data->flex_balance);
      data->percent_delta = (CALC_PERCENT(meal_balance, flex_balance) - data->starting_percentage);
      if ((data->meal_balance_delta != 0) || (data->flex_balance_delta != 0)) {
        prv_animation_create();
      }
    }
    s_data->meal_balance = meal_balance;
    s_data->flex_balance = flex_balance;
    persist_write_data(AppKey_MealBalance, &(s_data->meal_balance), sizeof(s_data->meal_balance));
    persist_write_data(AppKey_FlexBalance, &(s_data->flex_balance), sizeof(s_data->flex_balance));
}

static void prv_inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped (%u)", reason);
}

static void prv_request_refresh(void) {
  message_display_show_wait_text(s_data->message_display);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, AppKey_RequestRefresh, 1);
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
  window_set_background_color(window, BACKGROUND_COLOR);
  GRect percentage_display_bounds = layer_get_bounds(window_get_root_layer(window));
  GRect money_display_bounds = percentage_display_bounds;
#if defined(PBL_ROUND)
  money_display_bounds.size.h -= 80;
  money_display_bounds.origin.y += 30;
  money_display_bounds.size.w -= 50;
  money_display_bounds.origin.x += 25;
#else
  money_display_bounds.size.h -= 65;
  percentage_display_bounds.origin.y = percentage_display_bounds.size.h - 50;
  percentage_display_bounds.size.h = 50;
#endif

  s_data->money_display = money_display_create(&money_display_bounds);
  layer_add_child(window_get_root_layer(window), money_display_get_layer(s_data->money_display));

  s_data->message_display = message_display_create(&money_display_bounds, BACKGROUND_COLOR);
  message_display_hide(s_data->message_display);
  layer_add_child(window_get_root_layer(window),
                  message_display_get_layer(s_data->message_display));

  s_data->percentage_display = percentage_display_create(&percentage_display_bounds);
  percentage_display_set_colors(s_data->percentage_display, GColorOrange, GColorImperialPurple);
  layer_add_child(window_get_root_layer(window),
                  percentage_display_get_layer(s_data->percentage_display));

  money_display_set_amounts(s_data->money_display, s_data->meal_balance, s_data->flex_balance);
  percentage_display_set_percentage(s_data->percentage_display,
    (100 * s_data->flex_balance) / (s_data->meal_balance + s_data->flex_balance));

  prv_request_refresh();
}

static void prv_window_unload(Window *window) {
  money_display_destroy(s_data->money_display);
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

  persist_read_data(AppKey_MealBalance, &(s_data->meal_balance), sizeof(s_data->meal_balance));
  persist_read_data(AppKey_FlexBalance, &(s_data->flex_balance), sizeof(s_data->flex_balance));

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
