#include "simple-app-message.h"

#include "simple-app-message-namespace.h"

#include "pebble-events/pebble-events.h"

#include <pebble.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct SimpleAppMessageAssemblyState {
  char *namespace;
  uint8_t *buffer;
  uint32_t total_chunks;
  uint32_t chunks_remaining;
} SimpleAppMessageAssemblyState;

typedef struct SimpleAppMessageState {
  bool initialized;
  LinkedRoot *namespace_list;
  uint32_t chunk_size;
  SimpleAppMessageAssemblyState assembly_state;
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

  dict_write_uint32(chunk_size_message_iter, MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_SIZE, chunk_size);
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
    APP_LOG(APP_LOG_LEVEL_INFO, "Received request for SimpleAppMessage chunk size");
    prv_send_chunk_size_response(s_sam_state.chunk_size);
    return;
  }

  Tuple *message_namespace = dict_find(iterator, MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_NAMESPACE);
  Tuple *chunks_remaining = dict_find(iterator, MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_REMAINING);
  Tuple *total_chunks = dict_find(iterator, MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_TOTAL);
  Tuple *chunk_data = dict_find(iterator, MESSAGE_KEY_SIMPLE_APP_MESSAGE_CHUNK_DATA);
  if (!message_namespace || !chunks_remaining || !total_chunks || !chunk_data) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unexpected SimpleAppMessage packet (missing required state)");
    return;
  }

  // TODO probably remove me
  APP_LOG(APP_LOG_LEVEL_INFO, "Received SimpleAppMessage for namespace %s",
          message_namespace->value->cstring);

  SimpleAppMessageAssemblyState *assembly_state = &s_sam_state.assembly_state;

  const bool is_assembly_in_progress = (assembly_state->namespace && assembly_state->buffer);
  const bool is_message_expected_for_assembly_in_progress =
      (is_assembly_in_progress &&
       (strcmp(assembly_state->namespace, message_namespace->value->cstring) == 0) &&
       (assembly_state->total_chunks == total_chunks->value->uint32) &&
       (assembly_state->chunks_remaining == (chunks_remaining->value->uint32 + 1)));
  const bool is_message_expected_for_new_assembly =
      (!is_assembly_in_progress &&
       (chunks_remaining->value->uint32 == (total_chunks->value->uint32 - 1)));
  const bool is_message_expected =
      (is_message_expected_for_assembly_in_progress || is_message_expected_for_new_assembly);
  if (!is_message_expected) {
    // TODO reset assembly state
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unexpected SimpleAppMessage packet");
    return;
  }

  if (!is_assembly_in_progress) {
    // TODO replace with commented out strnlen usage once Pebble supports strnlen
    char *namespace_copy = malloc(strlen(message_namespace->value->cstring));
       // malloc(strnlen(message_namespace->value->cstring, message_namespace->length));
    if (!namespace_copy) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to malloc namespace copy for SimpleAppMessage assembly");
      // TODO reset assembly state
      return;
    }
    assembly_state->namespace = namespace_copy;

    uint8_t *buffer = malloc(s_sam_state.chunk_size * total_chunks->value->uint32);
    if (!buffer) {
      // TODO error, reset state, and return
    }
  }
  // TODO if no buffer or namespace in assembly state, create them now

  // TODO update assembly state: copy chunk's data to buffer, decrement chunks_remaining

  // TODO if we're done processing chunks, deserialize the buffer

  // TODO if serialization failed, print error and reset message assembly state before returning

  // TODO pass the deserialized SimpleDict to the appropriate callback based on namespace
}

static void prv_app_message_inbox_dropped_callback(AppMessageResult reason, void *context) {
  SimpleAppMessageAssemblyState *assembly_state = &s_sam_state.assembly_state;
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
