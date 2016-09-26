'use strict';

/**
 * serialize and object into an Array ready for transport via appMessage
 * @param {object} data
 * @return {Array}
 */
module.exports = function(data) {
  var keys = Object.keys(data);
  var length = keys.length;
  var result = [];
  var TYPES = {
    NULL: 0,
    BOOL: 1,
    INT: 2,
    DATA: 3,
    STRING: 4
  };

  /**
   * @private
   * @param {null|boolean|string|Array|number} val
   * @param {boolean} [useTerminator=true]
   * @return {void}
   */
  function _pushResult(val, useTerminator) {
    if (useTerminator) {
      result = result.concat(val, '\0');
    } else {
      result = result.concat(val);
    }
  }

  if (length > 255) {
    throw new Error('Number of items must be less than 255');
  }

  // number of keys
  _pushResult(length);

  for (var i = 0; i < length; i++) {
    var key = keys[i];
    var val = data[key];

    // key
    _pushResult(key.split(''), true);

    switch (typeof val) {
      case 'object' :
        if (Array.isArray(val)) {
          _pushResult(TYPES.DATA);
          _pushResult(
            [
              (val.length >>> 0) & 255,
              (val.length >>> 8) & 255

            ].concat(val)
          );
        } else {
          _pushResult(TYPES.NULL);
        }

        break;

      case 'number' :
        _pushResult(TYPES.INT);
        _pushResult([
          (val >>> 0) & 255,
          (val >>> 8) & 255,
          (val >>> 16) & 255,
          (val >>> 24) & 255
        ]);
        break;

      case 'string' :
        _pushResult(TYPES.STRING);
        _pushResult(val.split(''), true);
        break;

      case 'boolean' :
        _pushResult(TYPES.BOOL);
        _pushResult(val ? 1 : 0);
        break;
    }

  }

  return result;
};
