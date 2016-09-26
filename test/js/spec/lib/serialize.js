'use strict';

var assert = require('assert');
var serialize = require('../../../../src/js/lib/serialize');

describe('serialize', function() {

  // @TODO Could really use more extensive tests here
  it('correctly serializes the data', function() {
    var data = {
      Null: null,
      Bool0: false,
      Bool1: true,
      Int: 257,
      Data: [1, 2, 3, 4],
      String: 'test'
    };
    var expected = [
      6, 'N', 'u', 'l', 'l', '\u0000', 0, 'B', 'o', 'o', 'l', '0', '\u0000', 1, 0,
      'B', 'o', 'o', 'l', '1', '\u0000', 1, 1, 'I', 'n', 't', '\u0000', 2, 1, 1, 0, 0,
      'D', 'a', 't', 'a', '\u0000', 3, 4, 0, 1, 2, 3, 4, 'S', 't', 'r', 'i', 'n',
      'g', '\u0000', 4, 't', 'e', 's', 't', '\u0000'
    ];

    assert.deepEqual(serialize(data), expected);
  });

  it('throws for objects with too many keys', function() {
    var data = {};
    for (var i = 0; i < 256; i++) {
      data['test' + i] = true;
    }

    assert.throws(function() { serialize(data); }, /255/);
  });
});
