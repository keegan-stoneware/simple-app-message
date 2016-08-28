#pragma once

#include "pebble-string-dict/pebble-string-dict.h"

typedef void (*SimpleAppMessageReceivedCallback)(const StringDict *message, void *context);

typedef struct SimpleAppMessageCallbacks {
  SimpleAppMessageReceivedCallback message_received;
} SimpleAppMessageCallbacks;

void simple_app_message_request_inbox_size(uint32_t size);

AppMessageResult simple_app_message_open(void);

void simple_app_message_register_callbacks(const char *namespace,
                                           const SimpleAppMessageCallbacks *callbacks,
                                           void *context);

void simple_app_message_deregister_callbacks(const char *namespace);
