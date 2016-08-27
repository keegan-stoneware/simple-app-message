#include <pebble.h>
#include "simple-app-message/simple-app-message.h"

static Window *s_window;
static TextLayer *s_text_layer;

static StringDict* dict;

static void prv_window_load(Window *window) {
  dict = string_dict_create();

  string_dict_write_null(dict, "nullKey");
  string_dict_write_bool(dict, "trueKey", true);
  string_dict_write_bool(dict, "falseKey", false);
  string_dict_write_int(dict, "intKey", 3542);
  string_dict_write_string(dict, "strKey", "Hello World!");
  string_dict_write_data(dict, "dataKey", 12, dict);
}

static void prv_window_unload(Window *window) {
  StringDict *d = string_dict_get_data(dict, "dataKey");

  APP_LOG(APP_LOG_LEVEL_INFO, "trueKey: %s", string_dict_get_bool(d, "trueKey") ? "true" : "false");
  APP_LOG(APP_LOG_LEVEL_INFO, "falseKey: %s", string_dict_get_bool(d, "falseKey") ? "true" : "false");
  APP_LOG(APP_LOG_LEVEL_INFO, "intKey: %ld", string_dict_get_int(d, "intKey"));
  APP_LOG(APP_LOG_LEVEL_INFO, "strKey: %s", string_dict_get_string(d, "strKey"));

  string_dict_destroy(dict);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  window_stack_push(s_window, true);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
