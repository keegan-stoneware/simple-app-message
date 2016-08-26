'use strict';

var stubs = require('../stubs');
stubs.Pebble();
stubs.messageKeys();

var assert = require('assert');
var utils = require('../../../src/js/utils');

describe('utils', function() {
  describe('.objectToMessageKeys', function() {
    it('returns an object where the keys are the message key int', function() {
      var result = utils.objectToMessageKeys({
        SIMPLE_APP_MESSAGE_CHUNK_DATA: [1, 2, 3],
        SIMPLE_APP_MESSAGE_CHUNK_SIZE: 64,
        SIMPLE_APP_MESSAGE_CHUNK_REMAINING: 2,
        SIMPLE_APP_MESSAGE_CHUNK_NAMESPACE: 'TEST'
      });

      assert.deepEqual(result, {
        0: [1, 2, 3],
        1: 64,
        2: 2,
        3: 'TEST'
      });
    });
  });
});
