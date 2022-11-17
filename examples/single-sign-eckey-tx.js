const common = require('ethereumjs-common').default;
const util = require('ethereumjs-util');
const keccak256 = require('js-sha3').keccak256;
const Transaction = require('ethereumjs-tx').Transaction;
const hsm_util = require('./hsm_util.js');
const node_util = require('./node_util.js');
const nodeUrl = 'https://rinkeby-light.eth.linkpool.io:443/';
const hsm_constants = require('./hsm_constants.js');

async function main() {
    try {
        // 1、如果是单独为了获取公钥或者验签，这个pin可以填8-16个可见字符任意值
        // 2、如果是需要做签名，这个pin则需要是对应key的真实秘钥认证码
        const keyAuthPin = 'f@QbJPh$%&GkmIdr';

        // 密码机的服务连接密码
        const serviceConnectPassword = hsm_constants.serviceConnectPassword;

        // 连接密码机的ssl证书密码
        const sslCertPassword = hsm_constants.sslCertPassword;

        // 连接hsm
        hsm_util.connect(keyAuthPin, serviceConnectPassword, sslCertPassword);

        console.log('---------------- Sansec ECCSignKey1 publicKey:----------------');

        // ECCSignKey1 ~ ECCSignKey1000
        // const eccKeyLabelName = 'ECCSignKey1';
        // const eccKeyLabelName = 'ECCEncKey1';
        // const eccKeyLabelName = 'ECCSignKey2';
        const eccKeyLabelName = 'ECCSignKey3';

        // 获取未压缩的公钥
        let uncompressedPublicKey = hsm_util.getECKeyPublicKey(eccKeyLabelName);

        console.log(eccKeyLabelName + ':' + uncompressedPublicKey.toString('hex'));

        // 通过public key推算eth地址
        const address = '0x' + Buffer.from(keccak256(uncompressedPublicKey), 'hex').slice(-20).toString('hex');
        console.log('Generated ethereum address:' + address);

        const {result: nonce} = await node_util.getAddrNonce(nodeUrl, address);

        // 构建交易Tx Data
        const txParams = {
            chainId: 4,
            nonce: nonce,
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

        // 指定私钥，调用hsm进行签名
        const signature = hsm_util.ecKeySign('eth', eccKeyLabelName, messageHash);

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
    } catch (e) {
        console.error(e);
    } finally {
        hsm_util.disconnect();
    }
}

main();

