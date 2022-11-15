const yubihsm = require('bindings')('yubihsm.node');
module.exports = yubihsm.YubiHsm;