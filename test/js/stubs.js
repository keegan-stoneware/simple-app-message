'use strict';

var sinon = require('sinon');
var fixtures = require('./fixtures');
var mockRequire = require('mock-require');

/**
 * @return {void}
 */
module.exports.Pebble = function() {
  global.Pebble = {
    addEventListener: sinon.stub(),
    removeEventListener: sinon.stub(),
    openURL: sinon.stub(),
    sendAppMessage: sinon.stub(),
    getActiveWatchInfo: sinon.stub().returns(fixtures.activeWatchInfo()),
    getAccountToken: sinon.stub().returns(fixtures.accountToken()),
    getWatchToken: sinon.stub().returns(fixtures.watchToken())
  };
};

module.exports.messageKeys = function() {
  return mockRequire('message_keys', fixtures.messageKeys());
};
