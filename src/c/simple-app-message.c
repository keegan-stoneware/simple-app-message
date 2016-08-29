#include "simple-app-message.h"

#include "simple-app-message-namespace.h"

#include "pebble-events/pebble-events.h"

#include <pebble.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct SimpleAppMessageState {
  bool initialized;
  LinkedRoot *namespace_list;
  uint32_t chunk_size;
} SimpleAppMessageState;

static SimpleAppMessageState s_sam_state;

static void prv_send_chunk_size_response(void) {
  DictionaryIterator *chunk_size_message_iter;
  const AppMessageResult chunk_size_begin_result =
      app_message_outbox_begin(&chunk_size_message_iter);
  if(chunk_size_begin_result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to begin writing chunk size response, error code: %d",
            chunk_size_begin_result);
    return;
  }

  dict_write_uint32(chunk_size_message_iter, MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_SIZE,
                    s_sam_state.chunk_size);
  const AppMessageResult chunk_size_send_result = app_message_outbox_send();
  if (chunk_size_send_result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to send chunk size response, error code: %d",
            chunk_size_send_result);
  }
}

static void prv_app_message_inbox_received_callback(DictionaryIterator *iterator, void *context) {
  if (!s_sam_state.initialized) {
    return;
  }

  // Send back the chunk size, if requested, and return
  if (dict_find(iterator, MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_SIZE)) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Received request for Simple App Message chunk size");
    prv_send_chunk_size_response();
    return;
  }
  // TODO update our message assembly state
  // TODO if we're done processing chunks, pass the resulting StringDict to the appropriate callback
}

static void prv_app_message_inbox_dropped_callback(AppMessageResult reason, void *context) {
  // TODO reset message assembly state
}

void simple_app_message_init_with_chunk_size(uint32_t chunk_size) {
  s_sam_state.chunk_size = MAX(chunk_size, s_sam_state.chunk_size);

  events_app_message_request_inbox_size(chunk_size);

  if (s_sam_state.initialized) {
    return;
  }

  events_app_message_subscribe_handlers((EventAppMessageHandlers) {
    .received = prv_app_message_inbox_received_callback,
    .dropped = prv_app_message_inbox_dropped_callback,
  }, NULL);

  s_sam_state.initialized = true;
}

AppMessageResult simple_app_message_open(void) {
  return events_app_message_open();
}

void simple_app_message_register_callbacks(const char *namespace_name,
                                           const SimpleAppMessageCallbacks *callbacks,
                                           void *context) {
  if (!s_sam_state.namespace_list) {
    s_sam_state.namespace_list = linked_list_create_root();
  }

  SimpleAppMessageNamespace *namespace =
      simple_app_message_namespace_find_in_list(s_sam_state.namespace_list, namespace_name, NULL);
  if (!namespace) {
    namespace = simple_app_message_namespace_create(namespace_name);
    linked_list_append(s_sam_state.namespace_list, namespace);
  }

  simple_app_message_namespace_set_callbacks(namespace, callbacks, context);
}

void simple_app_message_deregister_callbacks(const char *namespace_name) {
  uint16_t index_of_namespace_in_list;
  SimpleAppMessageNamespace *namespace =
      simple_app_message_namespace_find_in_list(s_sam_state.namespace_list, namespace_name,
                                                &index_of_namespace_in_list);
  if (namespace) {
    linked_list_remove(s_sam_state.namespace_list, index_of_namespace_in_list);
    simple_app_message_namespace_destroy(namespace);
  }
}
