'use strict';

var messageKeys = require('message_keys');
var objectToMessageKeys = require('./utils').objectToMessageKeys;
var serialize = require('./lib/serialize');
var Plite = require('plite');

/**
 * @return {void}
 */
var simpleAppMessage = {};

simpleAppMessage._chunkSize = 0;
simpleAppMessage._timeout = 10000;
simpleAppMessage._chunkDelay = 100;
simpleAppMessage._maxNamespaceLenth = 16;

/**
 * @param {string} namespace
 * @param {object} data
 * @param {function} callback
 * @return {void}
 */
simpleAppMessage.send = function(namespace, data, callback) {
  var self = this;
  var requestTimeout;

  var chunkSizeResponseHandler = function(e) {
    var chunkSize = e.payload[messageKeys.SIMPLE_APP_MESSAGE_CHUNK_SIZE];

    // this app message was not meant for us.
    if (typeof chunkSize === 'undefined') {
      return;
    }

    Pebble.removeEventListener('appmessage', chunkSizeResponseHandler);
    clearTimeout(requestTimeout);

    if (!chunkSize || chunkSize <= 0) {
      throw new Error('simpleAppMessage: Fetched chunk size is invalid.');
    }

    self._chunkSize = chunkSize;
    self._sendData(namespace, data, callback);
  };

  if (namespace.length > simpleAppMessage._maxNamespaceLenth) {
    callback({
      error: 'simpleAppMessage: namespace length must not exceed ' +
             simpleAppMessage._maxNamespaceLenth
    });
    return;
  }

  if (!self._chunkSize) {

    // fetch chunk size
    Pebble.addEventListener('appmessage', chunkSizeResponseHandler);

    Pebble.sendAppMessage(
      objectToMessageKeys({SIMPLE_APP_MESSAGE_CHUNK_SIZE: 1}),
      function() {},
      function(error) {
        console.log('simpleAppMessage: Failed to request chunk size.');
        console.log(JSON.stringify(error));
        callback(error);
      }
    );

    requestTimeout = setTimeout(function() {
      Pebble.removeEventListener('appmessage', chunkSizeResponseHandler);
      callback('simpleAppMessage: Request for chunk size timed out.');
    }, self._timeout);
  } else {
    self._sendData(namespace, data, callback);
  }
};

/**
 * @private
 * @param {string} namespace
 * @param {object} data
 * @param {function} callback
 * @return {void}
 */
simpleAppMessage._sendData = function(namespace, data, callback) {
  var self = this;
  var dataSerialized = serialize(data);
  var chunks = [];

  if (!self._chunkSize) {
    throw new Error('simpleAppMessage: Chunk size is invalid');
  }

  while (dataSerialized.length > 0) {
    chunks.push(dataSerialized.splice(0, self._chunkSize));
  }

  var chain = Plite.resolve(true);
  chunks.forEach(function(chunk, index) {
    chain = chain.then(function() {
      var remaining = chunks.length - index - 1;
      return self._sendChunk(namespace, chunk, remaining, chunks.length);
    });
  });

  chain = chain.then(callback);
  chain = chain.catch(callback);
};

/**
 * @private
 * @param {string} namespace
 * @param {object} data
 * @param {number} remaining - remaining chunks
 * @param {number} total - total number of chunks
 * @return {Plite}
 */
simpleAppMessage._sendChunk = function(namespace, data, remaining, total) {
  return Plite(function(resolve, reject) {
    setTimeout(function() {
      Pebble.sendAppMessage(objectToMessageKeys({
        SIMPLE_APP_MESSAGE_CHUNK_DATA: data,
        SIMPLE_APP_MESSAGE_CHUNK_REMAINING: remaining,
        SIMPLE_APP_MESSAGE_CHUNK_TOTAL: total,
        SIMPLE_APP_MESSAGE_CHUNK_NAMESPACE: namespace
      }), resolve, reject);
    }, simpleAppMessage._chunkDelay);
  });
};

module.exports = simpleAppMessage;
