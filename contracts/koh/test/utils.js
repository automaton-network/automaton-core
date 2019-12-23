module.exports = {
  ether: function(n) {
    return web3.utils.toWei(n + '', 'ether');
  },

  gwei: function(n) {
    return web3.utils.toWei(n + '', 'gwei');
  },

  calcGasUsed: async function(txReceipt) {
    let BN = web3.utils.BN;
    let gasUsed = new BN(txReceipt.receipt.gasUsed);
    let tx = await web3.eth.getTransaction(txReceipt.tx);
    let gasPrice = new BN(tx.gasPrice);
    return gasUsed.mul(gasPrice);
  },

  increaseTime: async function(seconds) {
    await web3.currentProvider.send(
        {jsonrpc: "2.0", method: "evm_increaseTime", params: [seconds], id: 0}, (err, result) => {
        if (err) {
          console.log(err);
        }
      });
  }
}
