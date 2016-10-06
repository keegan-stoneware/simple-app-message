'use strict';
var simpleAppMessage = require('@keegan-stoneware/simple-app-message');
var testData = {
  keyNull: null,
  keyBool: true,
  keyInt: 257,
  keyData: [1, 2, 3, 4],
  keyString: 'test'
};

Pebble.addEventListener('ready', function() {
  simpleAppMessage.send('TEST', testData, function(e) {
    console.log('SimpleAppMessageTest - handler: ' + JSON.stringify(e));
  });
});
