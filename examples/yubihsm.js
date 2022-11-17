'use strict'
const Web3 = require('web3');
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
  // secp256k1
  r = hsm.signEcdsa(23830, data);
  console.log("signEcdsa", r);
}
// signEcdsa()


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
  // secp256r1
  // public_key: '8a891ad210569f0ac5621c62fd26cc1cbc4c909f9c76a4be7035332cbc01831a18393a8edfda28c58d7b366de4c1617f6c612c14c761065fb93adf8218acb820',
  r = hsm.signEcdsa(37852, data);
  console.log("signEcdsa", r);
}
// signEcdsa1()

async function getPublicKey() {
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
  
  // public_key: '8a891ad210569f0ac5621c62fd26cc1cbc4c909f9c76a4be7035332cbc01831a18393a8edfda28c58d7b366de4c1617f6c612c14c761065fb93adf8218acb820',
  r = hsm.getPublicKey(15804);
  console.log("getPublicKey", r);

  // secp256k1
  // key_id: 15804
  // public_key: 72eb853fd05bcf2fc7a03126e7d0e1415f02c9e923128ac60e87873d066a929534d7ad8675223e412b40039af4952f608ac6b9fabfd792bf6c85639e954ffe93

  // secp256r1
  // public_key: 'fafc44ed8460e569c40a55b1fc6a4aea73881595b3ccc778b85a1f6b1bebb17b0e43b5dfe96152c21e73b917f2c77732579be25e8d7746c5e47c59c2dd075ece',
  // key_id: 26646
}
// getPublicKey()

async function importOpaque() {
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

  const web3 = new Web3();
  const { privateKey } = web3.eth.accounts.create();

  const privKey = privateKey.slice(2);
  const { address } = web3.eth.accounts.privateKeyToAccount(privKey);
  console.log({ privKey, address });
  r = hsm.importOpaque('eth-private', 'opaque-data', privKey);
  console.log("importOpaque", r);
  // {
  //   privKey: 'c1ab38bb9f927df468a00241e4b9c3c55ae347aa393815e877e65eea66c12ccd',
  //   address: '0x5F6EcdB4fFC4517B54225b700579BEa9589b35c6'
  // }
  // importOpaque 57407
}
// importOpaque()

async function getOpaque() {
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

  r = hsm.getOpaque(57407);
  console.log("getOpaque", r);
}
getOpaque()

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