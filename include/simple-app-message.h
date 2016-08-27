#pragma once
#include "pebble-string-dict/pebble-string-dict.h"

typedef void (*SimpleAppMessageCallback) (StringDict* message, void* context);

// open app message

// register a handler
// deregister a handler
