#pragma once

#include <pebble.h>

typedef struct SimpleAppMessageAssembly SimpleAppMessageAssembly;

SimpleAppMessageAssembly *simple_app_message_assembly_create(size_t chunk_size);

//! Update assembly with new state. If the assembly was complete, it will be reset and updated
//! with the provided state. If the provided state is not valid for the assembly, the assembly
//! will be reset.
//! @return True if the assembly was successfully updated, false otherwise
bool simple_app_message_assembly_update(SimpleAppMessageAssembly *assembly, const Tuple *namespace,
                                        const Tuple *total_chunks, const Tuple *chunks_remaining,
                                        const Tuple *chunk_data);

bool simple_app_message_assembly_is_complete(const SimpleAppMessageAssembly *assembly);

//! Must match enum in serialize.js
typedef enum SimpleAppMessageAssemblyDataType {
  SimpleAppMessageAssemblyDataType_Null,
  SimpleAppMessageAssemblyDataType_Bool,
  SimpleAppMessageAssemblyDataType_Int,
  SimpleAppMessageAssemblyDataType_Data,
  SimpleAppMessageAssemblyDataType_String,

  SimpleAppMessageAssemblyDataType_Count
} SimpleAppMessageAssemblyDataType;

typedef void (*SimpleAppMessageDeserializeCallback)(const char *key,
                                                    SimpleAppMessageAssemblyDataType type,
                                                    const void *value, size_t n, void *context);

bool simple_app_message_deserialize(const SimpleAppMessageAssembly *assembly,
                                    SimpleAppMessageDeserializeCallback callback, void *context);

void simple_app_message_assembly_destroy(SimpleAppMessageAssembly *assembly);
