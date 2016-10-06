#pragma once

#include <@keegan-stoneware/simple-dict/simple-dict.h>

#define SIMPLE_APP_MESSAGE_NAMESPACE_MAX_SIZE_BYTES (16)

typedef void (*SimpleAppMessageReceivedCallback)(const SimpleDict *message, void *context);

size_t simple_app_message_get_minimum_inbox_size(void);

size_t simple_app_message_get_maximum_inbox_size(void);

bool simple_app_message_request_inbox_size(uint32_t inbox_size);

AppMessageResult simple_app_message_open(void);

typedef struct SimpleAppMessageCallbacks {
  SimpleAppMessageReceivedCallback message_received;
} SimpleAppMessageCallbacks;


bool simple_app_message_register_callbacks(const char *namespace,
                                           const SimpleAppMessageCallbacks *callbacks,
                                           void *context);

void simple_app_message_deregister_callbacks(const char *namespace);
