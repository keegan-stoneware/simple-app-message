'use strict';
var stubs = require('../stubs');
stubs.messageKeys();

var assert = require('assert');
var simpleAppMessage = require('../../../src/js/index');
var utils = require('../../../src/js/utils');
var fixtures = require('../fixtures');
var sinon = require('sinon');
var serialize = require('../../../src/js/lib/serialize');

describe('simpleAppMessage', function() {
  beforeEach(function() {
    stubs.Pebble();
    simpleAppMessage._chunkSize = 0;
  });

  describe('.send', function() {
    it('fetches the chunk size if not already defined then calls ._sendData()',
    function(done) {
      var appMessageData = fixtures.appMessageData();
      sinon.stub(simpleAppMessage, '_sendData').callsArg(2);

      Pebble.sendAppMessage.callsArg(1);

      simpleAppMessage.send('TEST', appMessageData, function() {
        assert.strictEqual(simpleAppMessage._sendData.callCount, 1);
        simpleAppMessage._sendData.restore();
        done();
      });

      Pebble.addEventListener
        .withArgs('appmessage')
        .callArgWith(1, {
          payload: utils.objectToMessageKeys({ SIMPLE_APP_MESSAGE_CHUNK_SIZE: 64 })
        });

      assert(Pebble.sendAppMessage.calledWith(
        utils.objectToMessageKeys({ SIMPLE_APP_MESSAGE_CHUNK_SIZE: 1 })
      ));
      assert.strictEqual(simpleAppMessage._chunkSize, 64);
    });

    it('does not fetch the chunk size if already defined then calls ._sendData',
    function(done) {
      var appMessageData = fixtures.appMessageData();
      sinon.stub(simpleAppMessage, '_sendData').callsArg(2);

      simpleAppMessage._chunkSize = 64;

      Pebble.sendAppMessage.callsArg(1);

      simpleAppMessage.send('TEST', appMessageData, function() {
        assert.strictEqual(simpleAppMessage._sendData.callCount, 1);
        simpleAppMessage._sendData.restore();
        done();
      });

      assert(Pebble.sendAppMessage.neverCalledWith(
        utils.objectToMessageKeys({ SIMPLE_APP_MESSAGE_CHUNK_SIZE: 1 })
      ));
      assert.strictEqual(simpleAppMessage._chunkSize, 64);
    });

    it('throws if the returned chunk size is zero', function() {
      var error = {error: 'someError'};
      simpleAppMessage.send('TEST', {}, function() {});
      sinon.stub(simpleAppMessage, '_sendData');
      sinon.stub(console, 'log');

      Pebble.sendAppMessage.callArgWith(2, error);

      assert(Pebble.sendAppMessage.calledWith(
        utils.objectToMessageKeys({ SIMPLE_APP_MESSAGE_CHUNK_SIZE: 1 })
      ));

      assert(console.log.calledWithMatch('Failed to request chunk size'));
      assert(console.log.calledWith(JSON.stringify(error)));

      simpleAppMessage._sendData.restore();
      console.log.restore();
    });

    it('logs an error for failed app messages', function() {
      simpleAppMessage.send('TEST', {}, function() {});

      assert.throws(function() {
        Pebble.addEventListener
          .withArgs('appmessage')
          .callArgWith(1, {
            payload: utils.objectToMessageKeys({ SIMPLE_APP_MESSAGE_CHUNK_SIZE: 0 })
          });
      });
      assert(Pebble.sendAppMessage.calledWith(
        utils.objectToMessageKeys({ SIMPLE_APP_MESSAGE_CHUNK_SIZE: 1 })
      ));
      assert.strictEqual(simpleAppMessage._chunkSize, 0);
    });

  });

  describe('._sendData', function() {
    it('calls _sendChunk for each chunk in order', function(done) {
      var callback = sinon.spy(function() {
        done();
      });
      var data = {test1: 'value1', test2: 'value2'};

      sinon.spy(simpleAppMessage, '_sendChunk');
      simpleAppMessage._chunkSize = 12;

      Pebble.sendAppMessage.onFirstCall().callsArg(1);
      Pebble.sendAppMessage.onSecondCall().callsArg(1);
      Pebble.sendAppMessage.onThirdCall().callsArg(1);

      simpleAppMessage._sendData('TEST', data, callback);

      var chunk1 = serialize(data).slice(0, 12);
      var chunk2 = serialize(data).slice(12, 24);
      var chunk3 = serialize(data).slice(24);
      sinon.assert.callOrder(
        simpleAppMessage._sendChunk.withArgs('TEST', chunk1, 2),
        simpleAppMessage._sendChunk.withArgs('TEST', chunk2, 1),
        simpleAppMessage._sendChunk.withArgs('TEST', chunk3, 0),
        callback
      );

      simpleAppMessage._sendChunk.restore();
    });

    it('passes an error to the callback if failed', function(done) {
      var expectedError = {some: 'error'};
      simpleAppMessage._chunkSize = 16;

      // success on first message
      Pebble.sendAppMessage.onFirstCall().callsArg(1);

      // fail on second
      Pebble.sendAppMessage.onSecondCall().callsArgWith(2, expectedError);

      var testData = {test1: 'TEST1', test2: 'TEST2'};
      simpleAppMessage._sendData('TEST', testData, function(error) {
        assert.deepEqual(error, expectedError);
        done();
      });
    });

    it('does not pass an error to the callback if successful', function(done) {
      simpleAppMessage._chunkSize = 16;

      // success on first and second message
      Pebble.sendAppMessage.onFirstCall().callsArg(1);
      Pebble.sendAppMessage.onSecondCall().callsArgWith(1);

      var testData = {test1: 'TEST1', test2: 'TEST2'};
      simpleAppMessage._sendData('TEST', testData, function(error) {
        assert.strictEqual(typeof error, 'undefined');
        done();
      });

    });
  });

  describe('._sendChunk', function() {
    it('sends the chunk with the correct data and returns a promise', function() {
      var chunk = serialize({test1: 'TEST1', test2: 'TEST2'});
      var result = simpleAppMessage._sendChunk('TEST', chunk, 1);

      sinon.assert.calledWith(Pebble.sendAppMessage, utils.objectToMessageKeys({
        SIMPLE_APP_MESSAGE_CHUNK_DATA: chunk,
        SIMPLE_APP_MESSAGE_CHUNK_REMAINING: 1,
        SIMPLE_APP_MESSAGE_CHUNK_NAMESPACE: 'TEST'
      }));

      assert.strictEqual(typeof result.then, 'function');
      assert.strictEqual(typeof result.catch, 'function');
    });
  });
});
