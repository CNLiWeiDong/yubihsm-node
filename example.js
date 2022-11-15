'use strict'

const { YubiHsm } = require('bindings')('yubihsm.node')

const usb_connection = "yhusb://"
// const connector_connection = "http://127.0.0.1:12345"

async function createAuthkey() {
  const config = {
    url: usb_connection,
    domain: 'all',
    authkey: 1,
  }
  const hsm = new YubiHsm(config)
  let r =  hsm.connectDev();
  console.log("connectDev", r);
  r = hsm.openSession('password');
  console.log("openSession", r);

  // 创建 authkey成功 key_id 14988
  // r = hsm.createAuthkey('lwd-wallet', 'aa123456');
  // console.log("createAuthkey", r);

  r = hsm.changeAuthkeyPwd(14988, 'bb123456');
  console.log("changeAuthkeyPwd", r);
}
// createAuthkey()


// 修改authkey密码成功， 之前测试过用authkey_id 1的session去修改失败。用自己的session可以修改成功
async function changeAuthkeyPwd() {
  const config = {
    url: usb_connection,
    domain: 'all',
    authkey: 14988,
  }
  const hsm = new YubiHsm(config)
  let r =  hsm.connectDev();
  console.log("connectDev", r);
  r = hsm.openSession('aa123456');
  console.log("openSession", r);

  // r = hsm.changeAuthkeyPwd(14988, 'bb123456');
  // console.log("changeAuthkeyPwd", r);
}
// changeAuthkeyPwd()

// 删除成功，  新建的authkey没有删除delete-asymmetric-key的能力。所以使用默认admin删除的
async function deleteObject() {
  const config = {
    url: usb_connection,
    domain: 'all',
    authkey: 1,
  }
  const hsm = new YubiHsm(config)
  let r =  hsm.connectDev();
  console.log("connectDev", r);
  r = hsm.openSession('password');
  console.log("openSession", r);

  // r = hsm.deleteObject(0x64, 'asymmetric-key');
  // console.log("deleteObject", r);

  // r = hsm.deleteObject(39381, 'asymmetric-key');
  // console.log("deleteObject", r);
}
// deleteObject()


// 测试成功
async function genKey() {
  const config = {
    url: usb_connection,
    domain: 'all',
    authkey: 14988,
  }
  const hsm = new YubiHsm(config)
  let r =  hsm.connectDev();
  console.log("connectDev", r);
  r = hsm.openSession('bb123456');
  console.log("openSession", r);

  r = hsm.genKey('secp256k1', 'eck256');
  console.log("genKey", r);

  // r = hsm.genKey('secp256k1', 'ecp256');
  // console.log("genKey", r);
}
genKey()

async function signEcdsa() {
  const config = {
    url: usb_connection,
    domain: 'all',
    authkey: 14988,
  }
  const hsm = new YubiHsm(config)
  let r =  hsm.connectDev();
  console.log("connectDev", r);
  r = hsm.openSession('bb123456');
  console.log("openSession", r);

  r = hsm.signEcdsa('secp256k1', 'eck256');
  console.log("signEcdsa", r);

  // r = hsm.signEcdsa('secp256k1', 'ecp256');
  // console.log("signEcdsa", r);
}
// signEcdsa()

/* 
 * Test if destructor is getting called
 */
function forceGC() {
  if (global.gc) {
    global.gc();
  } else {
    console.warn('No GC hook! Start your program as `node --expose-gc ./addon.js`.');
  }
}
// forceGC();
// setTimeout(forceGC, 1000);