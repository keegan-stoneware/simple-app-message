'use strict';

module.exports.activeWatchInfo = function() {
  return {
    platform: 'chalk',
    model: 'qemu_platform_chalk',
    language: 'en_US',
    firmware: {
      major: 3,
      minor: 3,
      patch: 2,
      suffix: ''
    }
  };
};

module.exports.accountToken = function() {
  return '0123456789abcdef0123456789abcdef';
};

module.exports.watchToken = function() {
  return '0123456789abcdef0123456789abcdef';
};

module.exports.messageKeys = function() {
  return {
    SIMPLE_APP_MESSAGE_CHUNK_DATA: 0,
    SIMPLE_APP_MESSAGE_CHUNK_SIZE: 1,
    SIMPLE_APP_MESSAGE_CHUNK_REMAINING: 2,
    SIMPLE_APP_MESSAGE_CHUNK_NAMESPACE: 3
  };
};

module.exports.appMessageData = function() {
  return {
    testString: 'foo',
    testInt: 123,
    testNull: null,
    testData: [0xFF, 0x01, 0x07, 0x80],
    testBool: true
  };
};
