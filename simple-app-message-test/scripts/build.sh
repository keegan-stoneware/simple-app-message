#!/bin/bash
pebble clean && cd .. && pebble build && { cd simple-app-message-test && pebble build; } || cd simple-app-message-test

