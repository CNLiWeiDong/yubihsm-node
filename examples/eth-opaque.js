
'use strict'
const Web3 = require('web3');
const { YubiHsm } = require('bindings')('yubihsm.node')
const request = require('request');
const usb_connection = "yhusb://";
const web3 = new Web3();

async function rpcRequest(uri, method, params) {
    try {
        const jsonData = JSON.stringify({
            jsonrpc: '2.0',
            method,
            params,
            id: 1,
        });

        const resJson = await request({
            method: 'POST',
            uri,
            headers: {
                'Content-Type': 'application/json',
            },
            body: jsonData,
            // json: true,
        });
        return JSON.parse(resJson);
    } catch (e) {
        throw e;
    }
}

async function main() {
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
    // {
    //   privKey: 'c1ab38bb9f927df468a00241e4b9c3c55ae347aa393815e877e65eea66c12ccd',
    //   address: '0x5F6EcdB4fFC4517B54225b700579BEa9589b35c6'
    // }
    // importOpaque 57407
    const txInfo = {
        chainId: 97,
        from: '0x5F6EcdB4fFC4517B54225b700579BEa9589b35c6',
        nonce: 0,
        gasPrice: `0x${Number(110000000000).toString(16)}`,
        gasLimit: `0x${Number(23000).toString(16)}`,
        to: '0x21b81f47b5fd95b5770f5d670b48b19e1e98dc79',
        data: '0x',
        value: '0x1',
    };
    console.log(`0x${r}`)
    const ret = await web3.eth.accounts.signTransaction(txInfo, `0x${r}`);
    console.log({
        txhash: ret.transactionHash,
        txhex: ret.rawTransaction,
    });
    r = null;

    console.log(await rpcRequest(
        'https://bsctestapi.terminet.io/rpc',
        'eth_sendRawTransaction',
        [ret.rawTransaction],
    ));
}

main();

