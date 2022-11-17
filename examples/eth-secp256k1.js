
'use strict'

const keccak256 = require('js-sha3').keccak256;
const Transaction = require('ethereumjs-tx').Transaction;
const common = require('ethereumjs-common').default;
const util = require('ethereumjs-util');
const { YubiHsm } = require('bindings')('yubihsm.node')

const usb_connection = "yhusb://";
const nodeUrl = 'https://rinkeby-light.eth.linkpool.io:443/';


async function main() {
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

    // secp256k1
    // key_id: 15804
    // public_key: 72eb853fd05bcf2fc7a03126e7d0e1415f02c9e923128ac60e87873d066a929534d7ad8675223e412b40039af4952f608ac6b9fabfd792bf6c85639e954ffe93
    // 0xf44ac71f93926f114266f85cfb3f232bfb1ed00e

    const uncompressedPublicKey = hsm.getPublicKey(15804);
    console.log("getPublicKey", uncompressedPublicKey);

    // 通过public key推算eth地址
    const address = '0x' + Buffer.from(keccak256(Buffer.from(uncompressedPublicKey, 'hex')), 'hex').slice(-20).toString('hex');
    console.log('Generated ethereum address:' + address);

    // 构建交易Tx Data
    const txParams = {
        chainId: 97,
        nonce: 0,
        gasPrice: 200000000000,
        gasLimit: 160000,
        to: '0x21b81f47b5fd95b5770f5d670b48b19e1e98dc79',
        value: '0x01',
        data: '0x00'
    }

    const customChain = common.forCustomChain(
        'mainnet',
        {
            name: 'custom-network',
            chainId: txParams.chainId
        },
        'petersburg'
    );

    const transaction = new Transaction(txParams, {common: customChain});

    // RLP Encode + Keccak256 拿到待签名的消息hash
    const messageHash = transaction.hash(false);
    console.log({ messageHash });
    const data = Buffer.from(messageHash).toString('hex');
    console.log({ data });
    const signature = Buffer.from(hsm.signEcdsa(15804, data), 'hex');
    console.log({ signature });
    console.log({ signature: signature.toString() });
   
    // 获取返回的签名r、s、v
    const rsv = {
        r: signature.slice(0, 32),
        s: signature.slice(32, 64),
        v: signature.slice(64, 66).compare(Buffer.from('00', 'hex')) === 0 ? Buffer.from('1b', 'hex') : Buffer.from('1c', 'hex')
    };

    // 将签名结果更新到交易中
    transaction.r = rsv.r;
    transaction.s = rsv.s;
    transaction.v = rsv.v;

    // EIP-155处理，根据chainId计算新的v值
    if (txParams.chainId != '') {
        transaction.v = util.bufferToInt(transaction.v) - 27 + txParams.chainId * 2 + 35;
    }
    // 序列化交易
    const serializedTx = transaction.serialize().toString('hex')
    console.log('transaction hash:' + '0x' + transaction.hash(true).toString('hex'))

    // Transaction ready for submission
    console.log('signed raw transaction(to broadcast):' + '0x' + serializedTx);

}

main();

