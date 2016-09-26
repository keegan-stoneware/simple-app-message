#include <inttypes.h>
#include <pebble.h>
#include <simple-app-message/simple-app-message.h>

#define SIMPLE_APP_MESSAGE_NAMESPACE ("TEST")
#define SIMPLE_APP_MESSAGE_CHUNK_SIZE (512)

static bool prv_print_dict(const char *key, SimpleDictDataType type, const void *data,
                           size_t data_size, void *context) {
  switch (type) {
    case SimpleDictDataType_Raw:
      APP_LOG(APP_LOG_LEVEL_INFO, "%s: [", key);
      for (size_t i = 0; i < data_size; i++) {
        APP_LOG(APP_LOG_LEVEL_INFO, "  0x%02X,", ((uint8_t *)data)[i]);
      }
      APP_LOG(APP_LOG_LEVEL_INFO, "]");
      return true;
    case SimpleDictDataType_Bool: {
      const bool value = *((bool *)data);
      APP_LOG(APP_LOG_LEVEL_INFO, "%s: %s", key, value ? "true" : "false");
      return true;
    }
    case SimpleDictDataType_Int: {
      const int value = *((int *)data);
      APP_LOG(APP_LOG_LEVEL_INFO, "%s: %d", key, value);
      return true;
    }
    case SimpleDictDataType_String: {
      const char *value = data;
      APP_LOG(APP_LOG_LEVEL_INFO, "%s: %s", key, value);
      return true;
    }
    case SimpleDictDataTypeCount:
      break;
  }
  APP_LOG(APP_LOG_LEVEL_ERROR, "Unexpected type %d", type);
  return false;
}

static void prv_simple_app_message_received_callback(const SimpleDict *message, void *context) {
  if (!message) {
    return;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Received SimpleAppMessage for namespace TEST:");
  simple_dict_foreach(message, prv_print_dict, NULL);
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
