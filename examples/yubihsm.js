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

  // r = hsm.genKey('secp256r1', 'ecp256');
  // console.log("genKey", r);
}
// genKey()

async function genKey1() {
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

  r = hsm.genKey('secp256r1', 'ecp256');
  console.log("genKey", r);
}
// genKey1()

//  *<tt>in_len</tt> is not 20, 28, 34, 48, 64 or 66. See #yh_rc for other possible
// 签名数据的长度必须是。。
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
  const data = Buffer.from('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa', 'utf-8').toString('hex');
  console.log(data);
  r = hsm.signEcdsa(23830, data);
  console.log("signEcdsa", r);
}
signEcdsa()


async function signEcdsa1() {
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
  const data = Buffer.from('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa', 'utf-8').toString('hex');
  console.log(data);
  // public_key: '8a891ad210569f0ac5621c62fd26cc1cbc4c909f9c76a4be7035332cbc01831a18393a8edfda28c58d7b366de4c1617f6c612c14c761065fb93adf8218acb820',
  r = hsm.signEcdsa(37852, data);
  console.log("signEcdsa", r);
}
// signEcdsa1()


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