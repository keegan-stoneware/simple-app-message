#include "simple-app-message.h"

#include "simple-app-message-namespace.h"

#include "pebble-events/pebble-events.h"

#include <pebble.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct SimpleAppMessageState {
  LinkedRoot *namespace_list;
  uint32_t max_inbox_size;
} SimpleAppMessageState;

static SimpleAppMessageState s_sam_state;

void simple_app_message_request_inbox_size(uint32_t size) {
  s_sam_state.max_inbox_size = MAX(size, s_sam_state.max_inbox_size);
}

AppMessageResult simple_app_message_open(void) {
  return app_message_open(s_sam_state.max_inbox_size, 0);
  // TODO also need to register main callbacks here
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
