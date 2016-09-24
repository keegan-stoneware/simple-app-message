#include <inttypes.h>
#include <pebble.h>
#include <simple-app-message/simple-app-message.h>

#define SIMPLE_APP_MESSAGE_NAMESPACE ("TEST")
#define SIMPLE_APP_MESSAGE_CHUNK_SIZE (512)

#define CHECK_SIMPLE_DICT_PRIMITIVE(dict, type, key, expected_value, formatter)              \
  do {                                                                                       \
    type data;                                                                               \
    if (!simple_dict_get_##type((dict), (key), &data)) {                                     \
      APP_LOG(APP_LOG_LEVEL_ERROR, key" not in dict");                                       \
    } else if (data != (expected_value)) {                                                   \
      APP_LOG(APP_LOG_LEVEL_ERROR, key": "formatter" != "formatter, data, (expected_value)); \
    } else {                                                                                 \
      APP_LOG(APP_LOG_LEVEL_INFO, key": "formatter, data);                                   \
    }                                                                                        \
  } while (0)

/*
#define CHECK_SIMPLE_DICT_DATA(dict, type, key, length, formatter)     \
  do {                                                                 \
    const type *data = string_dict_get_data((dict), (key));            \
    if (data) {                                                        \
      for (int i = 0; i < (length); i++) {                             \
        APP_LOG(APP_LOG_LEVEL_INFO, key"[%d]: "formatter, i, data[i]); \
      }                                                                \
    }                                                                  \
  } while (0)
*/

static void prv_simple_app_message_received_callback(const SimpleDict *message, void *context) {
  // We're expecting:
  // - keyNull: null,
  // - keyBool: true,
  // - keyInt: 257,
  // - keyData: [1, 2, 3, 4],
  // - keyString: "test"
//  CHECK_SIMPLE_DICT_PRIMITIVE(message, int, "keyNull", 0, "%d");
  CHECK_SIMPLE_DICT_PRIMITIVE(message, bool, "keyBool", true, "%d");
  CHECK_SIMPLE_DICT_PRIMITIVE(message, int, "keyInt", 257, "%d");
//  CHECK_SIMPLE_DICT_DATA(message, int, "keyData", 4, "%d");
//  CHECK_SIMPLE_DICT(message, string, "keyString", "%s");
}

static void prv_window_unload(Window *window) {
  window_destroy(window);
}

static void prv_init(void) {
  const SimpleAppMessageCallbacks simple_app_message_callbacks = (SimpleAppMessageCallbacks) {
    .message_received = prv_simple_app_message_received_callback,
  };
  simple_app_message_register_callbacks(SIMPLE_APP_MESSAGE_NAMESPACE, &simple_app_message_callbacks,
                                        NULL);
  simple_app_message_init_with_chunk_size(SIMPLE_APP_MESSAGE_CHUNK_SIZE);
  simple_app_message_open();

  Window *window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .unload = prv_window_unload,
  });

  window_stack_push(window, true);
}

int main(void) {
  prv_init();
  app_event_loop();
}
