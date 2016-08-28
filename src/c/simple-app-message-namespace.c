#include "simple-app-message-namespace.h"

struct SimpleAppMessageNamespace {
  char *name;
  SimpleAppMessageCallbacks callbacks;
  void *user_context;
};

SimpleAppMessageNamespace *simple_app_message_namespace_create(const char *name) {
  if (!name) {
    return NULL;
  }

  SimpleAppMessageNamespace *namespace = malloc(sizeof(*namespace));
  if (!namespace) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Could not malloc memory for new namespace object \"%s\"", name);
    return NULL;
  }

  char *name_copy = malloc(strlen(name) + 1);
  if (!name_copy) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Could not malloc memory for new namespace name \"%s\"", name);
    free(namespace);
    return NULL;
  }

  *namespace = (SimpleAppMessageNamespace) {
    .name = name_copy,
  };

  return namespace;
}

void simple_app_message_namespace_set_callbacks(SimpleAppMessageNamespace *namespace,
                                                const SimpleAppMessageCallbacks *callbacks,
                                                void *context) {
  if (!namespace) {
    return;
  }

  namespace->callbacks = callbacks ? *callbacks : (SimpleAppMessageCallbacks) {0};
  namespace->user_context = context;
}

static bool prv_find_namespace_in_list_callback(void *object1, void *object2) {
  SimpleAppMessageNamespace *namespace1 = object1;
  SimpleAppMessageNamespace *namespace2 = object2;
  return (strcmp(namespace1->name, namespace2->name) == 0);
}

SimpleAppMessageNamespace *simple_app_message_namespace_find_in_list(LinkedRoot *root,
                                                                     const char *name,
                                                                     uint16_t *index_out) {
  if (!root) {
    return NULL;
  }

  SimpleAppMessageNamespace dummy_namespace = (SimpleAppMessageNamespace) {
    .name = (char *)name,
  };
  const int16_t index = linked_list_find_compare(root, &dummy_namespace,
                                                 prv_find_namespace_in_list_callback);
  if (index == -1) {
    return NULL;
  }

  if (index_out) {
    *index_out = (uint16_t)index;
  }

  return linked_list_get(root, (uint16_t)index);
}

void simple_app_message_namespace_get_callbacks(const SimpleAppMessageNamespace *namespace,
                                                SimpleAppMessageCallbacks *callbacks_out,
                                                void **context_out) {
  if (!namespace) {
    return;
  }

  if (callbacks_out) {
    *callbacks_out = namespace->callbacks;
  }

  if (context_out) {
    *context_out = namespace->user_context;
  }
}

void simple_app_message_namespace_destroy(SimpleAppMessageNamespace *namespace) {
  if (!namespace) {
    return;
  }

  free(namespace->name);
  free(namespace);
}
