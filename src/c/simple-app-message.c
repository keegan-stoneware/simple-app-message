#include "simple-app-message.h"

#include "simple-app-message-assembly.h"
#include "simple-app-message-namespace.h"

#include "pebble-events/pebble-events.h"

#include <pebble.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

//! SIMPLE_APP_MESSAGE_CHUNK_DATA, SIMPLE_APP_MESSAGE_CHUNK_REMAINING,
//! SIMPLE_APP_MESSAGE_CHUNK_TOTAL, SIMPLE_APP_MESSAGE_CHUNK_NAMESPACE
//! @note: Doesn't include SIMPLE_APP_MESSAGE_CHUNK_SIZE because that should be sent in a separate
//! message that only includes that key
#define SIMPLE_APP_MESSAGE_MAX_NUM_KEYS_IN_MESSAGE (4)

//! One uint32_t for each key, namespace max size bytes, one uint32_t for remaining chunk value,
//! one uint32_t for total chunk value, and at least 1 byte for the chunk size data
#define SIMPLE_APP_MESSAGE_MIN_INBOX_SIZE                              \
    ((SIMPLE_APP_MESSAGE_MAX_NUM_KEYS_IN_MESSAGE * sizeof(uint32_t)) + \
     SIMPLE_APP_MESSAGE_NAMESPACE_MAX_SIZE_BYTES +                     \
     sizeof(uint32_t) +                                                \
     sizeof(uint32_t) +                                                \
     1                                                                 \
    )

typedef struct SimpleAppMessageState {
  bool open;
  // TODO change this to a ref counter so we can provide a safe deinitializer
  bool initialized;
  LinkedRoot *namespace_list;
  uint32_t chunk_size;
  SimpleAppMessageAssembly *assembly;
} SimpleAppMessageState;

static SimpleAppMessageState s_sam_state;

static void prv_send_chunk_size_response(uint32_t chunk_size) {
  DictionaryIterator *chunk_size_message_iter;
  const AppMessageResult chunk_size_begin_result =
      app_message_outbox_begin(&chunk_size_message_iter);
  if(chunk_size_begin_result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to begin writing chunk size response, error code: %d",
            chunk_size_begin_result);
    return;
  }

  const DictionaryResult dict_write_result =
      dict_write_uint32(chunk_size_message_iter, MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_SIZE,
                        chunk_size);
  if (dict_write_result != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to write chunk size response to dict, error code: %d",
            dict_write_result);
    return;
  }

  const AppMessageResult chunk_size_send_result = app_message_outbox_send();
  if (chunk_size_send_result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to send chunk size response, error code: %d",
            chunk_size_send_result);
    return;
  }
}

static void prv_assembly_deserialize_callback(const char *key,
                                              SimpleAppMessageAssemblyDataType type,
                                              const void *value, size_t n, void *context) {
  SimpleDict *dict = context;
  if (!dict) {
    return;
  }

  if (!key) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Skipping deserialization of key/value pair with missing key");
    return;
  }

  switch (type) {
    case SimpleAppMessageAssemblyDataType_Data:
      simple_dict_update_data(dict, key, value, n);
      break;
    case SimpleAppMessageAssemblyDataType_Bool:
      simple_dict_update_bool(dict, key, *((bool *)value));
      break;
    case SimpleAppMessageAssemblyDataType_Int:
      simple_dict_update_int(dict, key, *((int *)value));
      break;
    case SimpleAppMessageAssemblyDataType_String:
      simple_dict_update_string(dict, key, value);
      break;
    case SimpleAppMessageAssemblyDataType_Null:
    case SimpleAppMessageAssemblyDataType_Count:
      APP_LOG(APP_LOG_LEVEL_WARNING, "Not handling deserialized key %s of type %d", key, type);
      break;
  }
}

static void prv_app_message_inbox_received_callback(DictionaryIterator *iterator, void *context) {
  if (!s_sam_state.initialized || !s_sam_state.open) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Received SimpleAppMessage packet but module not initialized/open");
    return;
  }

  if (!s_sam_state.assembly) {
    s_sam_state.assembly = simple_app_message_assembly_create(s_sam_state.chunk_size);
    if (!s_sam_state.assembly) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create SimpleAppMessage assembly state");
      return;
    }
  }

  // Send back the chunk size, if requested, and return
  if (dict_find(iterator, MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_SIZE)) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Received request for SimpleAppMessage chunk size");
    prv_send_chunk_size_response(s_sam_state.chunk_size);
    return;
  }

  const Tuple *message_namespace = dict_find(iterator,
                                             MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_NAMESPACE);
  if (!message_namespace) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Namespace missing from SimpleAppMessage packet");
    return;
  }

  if (strlen(message_namespace->value->cstring) + 1 > SIMPLE_APP_MESSAGE_NAMESPACE_MAX_SIZE_BYTES) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Ignoring SimpleAppMessage packet with namespace larger than max allowed size");
    return;
  }

  SimpleAppMessageNamespace *namespace =
      simple_app_message_namespace_find_in_list(s_sam_state.namespace_list,
                                                message_namespace->value->cstring,
                                                NULL /* index */);
  SimpleAppMessageCallbacks user_callbacks = (SimpleAppMessageCallbacks) {0};
  void *user_context = NULL;
  if (!simple_app_message_namespace_get_callbacks(namespace, &user_callbacks, &user_context)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown namespace in SimpleAppMessage packet");
    return;
  }

  const Tuple *chunks_remaining = dict_find(iterator,
                                            MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_REMAINING);
  const Tuple *total_chunks = dict_find(iterator,
                                        MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_TOTAL);
  const Tuple *chunk_data = dict_find(iterator,
                                      MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_DATA);
  if (!simple_app_message_assembly_update(s_sam_state.assembly, message_namespace, total_chunks,
                                          chunks_remaining, chunk_data)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unexpected SimpleAppMessage packet received");
    return;
  }

  if (!simple_app_message_assembly_is_complete(s_sam_state.assembly)) {
    return;
  }

  SimpleDict *dict = simple_dict_create();
  if (!dict) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create SimpleDict for deserializing SimpleAppMessage");
    return;
  }

  if (!simple_app_message_deserialize(s_sam_state.assembly, prv_assembly_deserialize_callback,
                                     dict)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to deserialize SimpleAppMessage");
    simple_dict_destroy(dict);
    return;
  }

  if (user_callbacks.message_received) {
    user_callbacks.message_received(dict, user_context);
  }

  simple_dict_destroy(dict);
}

static void prv_app_message_inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "SimpleAppMessage packet dropped, reason: %d", reason);
}

size_t simple_app_message_get_minimum_inbox_size(void) {
  return SIMPLE_APP_MESSAGE_MIN_INBOX_SIZE;
}

size_t simple_app_message_get_maximum_inbox_size(void) {
  return app_message_inbox_size_maximum();
}

bool simple_app_message_request_inbox_size(uint32_t inbox_size) {
  if (s_sam_state.open) {
    return false;
  }

  // Subtract one to account for the minimum chunk size of 1 included in
  // SIMPLE_APP_MESSAGE_MIN_INBOX_SIZE
  const int64_t requested_chunk_size = (int64_t)inbox_size - SIMPLE_APP_MESSAGE_MIN_INBOX_SIZE - 1;
  if (requested_chunk_size <= 0) {
    return false;
  }

  s_sam_state.chunk_size = MAX((uint32_t)requested_chunk_size, s_sam_state.chunk_size);

  events_app_message_request_inbox_size(inbox_size);

  if (s_sam_state.initialized) {
    return true;
  }

  events_app_message_subscribe_handlers((EventAppMessageHandlers) {
    .received = prv_app_message_inbox_received_callback,
    .dropped = prv_app_message_inbox_dropped_callback,
  }, NULL);

  s_sam_state.initialized = true;
  return true;
}

AppMessageResult simple_app_message_open(void) {
  if (!s_sam_state.initialized) {
    return APP_MSG_INVALID_STATE;
  }

  const AppMessageResult open_success = events_app_message_open();
  s_sam_state.open = (open_success == APP_MSG_OK);
  return open_success;
}

bool simple_app_message_register_callbacks(const char *namespace_name,
                                           const SimpleAppMessageCallbacks *callbacks,
                                           void *context) {
  if (!namespace_name ||
      (strlen(namespace_name) + 1 > SIMPLE_APP_MESSAGE_NAMESPACE_MAX_SIZE_BYTES)) {
    return false;
  }

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
  return true;
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
