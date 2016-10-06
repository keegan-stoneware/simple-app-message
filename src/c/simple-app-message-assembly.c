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

  const char *namespace_string = namespace->value->cstring;

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

static bool prv_deserialize_bool(const uint8_t **cursor, const uint8_t **data_out, size_t *n_out) {
  if (!cursor) {
    return false;
  }

  if (data_out) {
    *data_out = *cursor;
  } else {
    return false;
  }

  const size_t bool_size = sizeof(bool);
  if (n_out) {
    *n_out = bool_size;
  } else {
    return false;
  }

  *cursor += bool_size;

  return true;
}

static bool prv_deserialize_int(const uint8_t **cursor, const uint8_t **data_out, size_t *n_out) {
  if (!cursor) {
    return false;
  }

  if (data_out) {
    *data_out = *cursor;
  } else {
    return false;
  }

  const size_t int_size = sizeof(int);
  if (n_out) {
    *n_out = int_size;
  } else {
    return false;
  }

  *cursor += int_size;

  return true;
}

static bool prv_deserialize_data(const uint8_t **cursor, const uint8_t **data_out, size_t *n_out) {
  if (!cursor) {
    return false;
  }

  uint16_t data_size = *((uint16_t *)*cursor);
  *cursor += sizeof(data_size);

  if (data_out) {
    *data_out = *cursor;
  } else {
    return false;
  }

  if (n_out) {
    *n_out = data_size;
  } else {
    return false;
  }

  *cursor += data_size;

  return true;
}

static bool prv_deserialize_string(const uint8_t **cursor, const uint8_t **data_out,
                                   size_t *n_out) {
  if (!cursor) {
    return false;
  }

  const char *data = *((char **)cursor);
  const size_t data_length = strlen(data) + 1;

  if (data_out) {
    *data_out = (uint8_t *)data;
  } else {
    return false;
  }

  if (n_out) {
    *n_out = data_length;
  } else {
    return false;
  }

  *cursor += data_length;

  return true;
}

//! @return True if successfully set data_out to deserialized data and n_out to deserialized data
//! length
typedef bool (*AssemblyDeserializeFunc)(const uint8_t **cursor, const uint8_t **data_out,
                                        size_t *n_out);

static const AssemblyDeserializeFunc s_deserialize_funcs[SimpleAppMessageAssemblyDataType_Count] = {
  [SimpleAppMessageAssemblyDataType_Null] = NULL,
  [SimpleAppMessageAssemblyDataType_Bool] = prv_deserialize_bool,
  [SimpleAppMessageAssemblyDataType_Int] = prv_deserialize_int,
  [SimpleAppMessageAssemblyDataType_Data] = prv_deserialize_data,
  [SimpleAppMessageAssemblyDataType_String] = prv_deserialize_string,
};

bool simple_app_message_deserialize(const SimpleAppMessageAssembly *assembly,
                                    SimpleAppMessageDeserializeCallback callback, void *context) {
  if (!simple_app_message_assembly_is_complete(assembly)) {
    return false;
  }

  const uint8_t *cursor = assembly->state.buffer;
  uint8_t keys_left_to_read = *(cursor++);
  while ((keys_left_to_read > 0) && (cursor < assembly->state.buffer_cursor)) {
    const char *key = (char *)cursor;
    const size_t key_length = strlen(key);
    cursor += key_length + 1;

    const SimpleAppMessageAssemblyDataType type = (SimpleAppMessageAssemblyDataType)*(cursor++);
    if (type >= SimpleAppMessageAssemblyDataType_Count) {
      return false;
    }

    const uint8_t *data = NULL;
    size_t n = 0;
    const AssemblyDeserializeFunc deserialize_func = s_deserialize_funcs[type];
    if (deserialize_func && !deserialize_func(&cursor, &data, &n)) {
      return false;
    }

    if (callback) {
      callback(key, type, data, n, context);
    }

    keys_left_to_read--;
  }

  return (keys_left_to_read == 0) && (cursor == assembly->state.buffer_cursor);
}

void simple_app_message_assembly_destroy(SimpleAppMessageAssembly *assembly) {
  prv_assembly_deinit(assembly);
  free(assembly);
}
