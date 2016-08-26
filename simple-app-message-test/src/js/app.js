'use strict';
var simpleAppMessage = require('pebble-simple-app-message');
var testData = {
  keyNull: null,
  keyBool: true,
  keyInt: 257,
  keyData: [1, 2, 3, 4],
  keyString: 'test'
};

Pebble.addEventListener('ready', function() {
  simpleAppMessage.send('TEST', testData, function(e) {
    console.log('KEEGAN: e', e);
  });
});
