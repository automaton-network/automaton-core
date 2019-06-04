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
}
