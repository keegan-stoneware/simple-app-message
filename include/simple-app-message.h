#pragma once

#include <simple-dict/simple-dict.h>

typedef void (*SimpleAppMessageReceivedCallback)(const SimpleDict *message, void *context);

typedef struct SimpleAppMessageCallbacks {
  SimpleAppMessageReceivedCallback message_received;
} SimpleAppMessageCallbacks;

void simple_app_message_init_with_chunk_size(uint32_t chunk_size);

AppMessageResult simple_app_message_open(void);

void simple_app_message_register_callbacks(const char *namespace,
                                           const SimpleAppMessageCallbacks *callbacks,
                                           void *context);

void simple_app_message_deregister_callbacks(const char *namespace);
