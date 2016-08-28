#pragma once

#include "simple-app-message.h"

#include "@smallstoneapps/linked-list/linked-list.h"

#include <pebble.h>

typedef struct SimpleAppMessageNamespace SimpleAppMessageNamespace;

SimpleAppMessageNamespace *simple_app_message_namespace_create(const char *name);

void simple_app_message_namespace_set_callbacks(SimpleAppMessageNamespace *namespace,
                                                const SimpleAppMessageCallbacks *callbacks,
                                                void *context);

SimpleAppMessageNamespace *simple_app_message_namespace_find_in_list(LinkedRoot *root,
                                                                     const char *name,
                                                                     uint16_t *index_out);

void simple_app_message_namespace_get_callbacks(const SimpleAppMessageNamespace *namespace,
                                                SimpleAppMessageCallbacks *callbacks_out,
                                                void **context_out);

void simple_app_message_namespace_destroy(SimpleAppMessageNamespace *namespace);
