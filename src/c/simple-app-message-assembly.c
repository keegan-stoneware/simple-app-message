#include "simple-app-message-assembly.h"

typedef struct SimpleAppMessageAssemblyState {
  char *namespace;
  uint8_t *buffer;
  uint8_t *buffer_cursor;
  uint32_t total_chunks;
  uint32_t chunks_remaining;
} SimpleAppMessageAssemblyState;

struct SimpleAppMessageAssembly {
  size_t chunk_size;
  SimpleAppMessageAssemblyState state;
};

SimpleAppMessageAssembly *simple_app_message_assembly_create(size_t chunk_size) {
  SimpleAppMessageAssembly *assembly = calloc(1, sizeof(SimpleAppMessageAssembly));
  if (assembly) {
    assembly->chunk_size = chunk_size;
  }
  return assembly;
}

static void prv_assembly_deinit(SimpleAppMessageAssembly *assembly) {
  if (!assembly) {
    return;
  }
  free(assembly->state.namespace);
  free(assembly->state.buffer);
}

static void prv_assembly_reset(SimpleAppMessageAssembly *assembly) {
  if (!assembly) {
    return;
  }
  prv_assembly_deinit(assembly);
  assembly->state = (SimpleAppMessageAssemblyState) {0};
}

static bool prv_assembly_in_progress(const SimpleAppMessageAssembly *assembly) {
  return (assembly && assembly->state.namespace && assembly->state.buffer &&
          (assembly->state.buffer_cursor > assembly->state.buffer));
}

bool simple_app_message_assembly_update(SimpleAppMessageAssembly *assembly, const Tuple *namespace,
                                        const Tuple *total_chunks, const Tuple *chunks_remaining,
                                        const Tuple *chunk_data) {
  if (!assembly || !namespace || !chunks_remaining || !total_chunks || !chunk_data) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unexpected SimpleAppMessage packet (missing required state)");
    return false;
  }

  // Reset the assembly state if we previously had completed an assembly
  if (simple_app_message_assembly_is_complete(assembly)) {
    prv_assembly_reset(assembly);
  }

  // TODO probably remove me
  const char *namespace_string = namespace->value->cstring;
  APP_LOG(APP_LOG_LEVEL_INFO, "Received SimpleAppMessage for namespace %s", namespace_string);

  const bool is_assembly_in_progress = prv_assembly_in_progress(assembly);
  const bool is_message_expected_for_assembly_in_progress =
      is_assembly_in_progress &&
      (strcmp(assembly->state.namespace, namespace_string) == 0) &&
      (assembly->state.total_chunks == total_chunks->value->uint32) &&
      (assembly->state.chunks_remaining == (chunks_remaining->value->uint32 + 1));
  const bool is_message_expected =
      (!is_assembly_in_progress || is_message_expected_for_assembly_in_progress);
  if (!is_message_expected) {
    prv_assembly_reset(assembly);
    return false;
  }

  // If no buffer or namespace in assembly state, create them now
  if (!is_assembly_in_progress) {
    // TODO replace with commented out strnlen usage once Pebble supports strnlen
    char *namespace_copy = malloc(namespace->length);
    // malloc(strnlen(namespace_string, namespace->length));
    if (!namespace_copy) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to malloc namespace copy for SimpleAppMessage assembly");
      prv_assembly_reset(assembly);
      return false;
    }
    strncpy(namespace_copy, namespace_string, namespace->length);
    namespace_copy[namespace->length - 1] = '\0';
    assembly->state.namespace = namespace_copy;

    uint8_t *buffer = malloc(assembly->chunk_size * total_chunks->value->uint32);
    if (!buffer) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to malloc buffer for SimpleAppMessage assembly");
      prv_assembly_reset(assembly);
      return false;
    }
    assembly->state.buffer = buffer;
    assembly->state.buffer_cursor = buffer;
    assembly->state.total_chunks = total_chunks->value->uint32;
    assembly->state.chunks_remaining = assembly->state.total_chunks;
  }

  memcpy(assembly->state.buffer_cursor, chunk_data->value->data, chunk_data->length);
  assembly->state.buffer_cursor += chunk_data->length;
  assembly->state.chunks_remaining--;

  return true;
}

bool simple_app_message_assembly_is_complete(const SimpleAppMessageAssembly *assembly) {
  return (prv_assembly_in_progress(assembly) && (assembly->state.total_chunks > 0) &&
          (assembly->state.chunks_remaining == 0));
}

bool simple_app_message_deserialize(const SimpleAppMessageAssembly *assembly,
                                    SimpleAppMessageDeserializeCallback callback, void *context) {
  if (!simple_app_message_assembly_is_complete(assembly)) {
    return false;
  }

  const uint8_t *cursor = assembly->state.buffer;
  while (cursor < assembly->state.buffer_cursor) {
    // TODO deserialize and call user-provided callback
    APP_LOG(APP_LOG_LEVEL_WARNING, "0x%02X\t%c", *cursor, *cursor);
    cursor++;
  }

  return true;
}

void simple_app_message_assembly_destroy(SimpleAppMessageAssembly *assembly) {
  prv_assembly_deinit(assembly);
  free(assembly);
}
