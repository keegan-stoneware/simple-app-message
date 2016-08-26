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

/**
 * @param {string} namespace
 * @param {object} data
 * @param {function} callback
 * @return {void}
 */
simpleAppMessage.send = function(namespace, data, callback) {
  var self = this;

  if (!self._chunkSize) {

    // fetch chunk size
    Pebble.addEventListener('appmessage', function(e) {
      var chunkSize = e.payload[messageKeys.SIMPLE_APP_MESSAGE_CHUNK_SIZE];

      if (!chunkSize || chunkSize <= 0) {
        throw new Error('simpleAppMessage: Fetched chunk size is invalid');
      }

      self._chunkSize = chunkSize;
    });

    Pebble.sendAppMessage(objectToMessageKeys({
      SIMPLE_APP_MESSAGE_CHUNK_SIZE: 1
    }), function() {
      // success
      self._sendData(namespace, data, callback);
    }, function(error) {
      console.log('simpleAppMessage: Failed to request chunk size!');
      console.log(JSON.stringify(error));
    });
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

  while (dataSerialized.length > 0) {
    chunks.push(dataSerialized.splice(0, self._chunkSize));
  }

  var chain = Plite.resolve(true);
  chunks.forEach(function(chunk, index) {
    chain = chain.then(function() {
      return self._sendChunk(namespace, chunk, chunks.length - index - 1);
    });
  });

  chain = chain.then(callback);
  chain = chain.catch(callback);
};

/**
 * @private
 * @param {string} namespace
 * @param {object} data
 * @param {number} remainingChunks
 * @return {Plite}
 */
simpleAppMessage._sendChunk = function(namespace, data, remainingChunks) {
  return Plite(function(resolve, reject) {
    Pebble.sendAppMessage(objectToMessageKeys({
      SIMPLE_APP_MESSAGE_CHUNK_DATA: data,
      SIMPLE_APP_MESSAGE_CHUNK_REMAINING: remainingChunks,
      SIMPLE_APP_MESSAGE_CHUNK_NAMESPACE: namespace
    }), resolve, reject);
  });
};

module.exports = simpleAppMessage;
