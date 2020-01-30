let BN = web3.utils.BN;
let utils = require('./utils.js');
let ether = utils.ether;
let gwei = utils.gwei;
let calcGasUsed = utils.calcGasUsed;

let catchRevert            = require("./exceptions.js").catchRevert;
let catchOutOfGas          = require("./exceptions.js").catchOutOfGas;
let catchInvalidJump       = require("./exceptions.js").catchInvalidJump;
let catchInvalidOpcode     = require("./exceptions.js").catchInvalidOpcode;
let catchStackOverflow     = require("./exceptions.js").catchStackOverflow;
let catchStackUnderflow    = require("./exceptions.js").catchStackUnderflow;
let catchStaticStateChange = require("./exceptions.js").catchStaticStateChange;

describe('TestKingAutomatonDEX', async (accounts) => {
  const KingAutomaton = artifacts.require("KingAutomaton");

  beforeEach(async () => {
    accounts = await web3.eth.getAccounts();
    mainAcct = accounts[0];
    koh = await KingAutomaton.new(4, 16, "0x010000", "406080000", 10, -10, 2);
    minOrderAUTO = await koh.minOrderAUTO();
    minOrderETH = await koh.minOrderETH();
  });

  it('complete test buy() and sellNow()', async () => {
    await koh.buy(minOrderAUTO, { from: accounts[1], value: minOrderETH });
    assert.equal(await web3.eth.getBalance(koh.address), minOrderETH,
        "The ETH should be deposited in the contract");

    let startETHBalance = new BN(await web3.eth.getBalance(accounts[0]));
    let txReceipt = await koh.sellNow(0, minOrderAUTO, minOrderETH, { from: accounts[0] });

    // Calculate final balance including gas costs
    let gasUsed = new BN(txReceipt.receipt.gasUsed);
    let tx = await web3.eth.getTransaction(txReceipt.tx);
    let gasPrice = new BN(tx.gasPrice);
    let expectedETHBalance = startETHBalance.add(minOrderETH).sub(gasPrice.mul(gasUsed));
    let finalETHBalance = new BN(await web3.eth.getBalance(accounts[0]));
    let finalAUTOBalance = new BN(await koh.balanceOf(accounts[1]));

    assert.equal(await web3.eth.getBalance(koh.address), 0, "There should be no ETH left in the contract");
    assert.equal(await koh.getDEXBalance(), 0, "There should be no AUTO left in the DEX account");
    assert.equal(finalAUTOBalance.toString(), minOrderAUTO.toString(), "Buyer should end up with requested AUTO");
    assert.equal(finalETHBalance.toString(), expectedETHBalance.toString(), "Seller should end up with requested ETH");
  });

  it('complete test sell() and buyNow()', async () => {
    await koh.sell(minOrderAUTO, minOrderETH);
    DEXBalance = await koh.getDEXBalance();
    assert.equal(DEXBalance.toString(), minOrderAUTO.toString(),
        "The AUTO should be deposited in the DEX special purpose account");

    let startETHBalance = new BN(await web3.eth.getBalance(accounts[0]));
    let txReceipt = await koh.buyNow(0, minOrderAUTO, { from: accounts[1], value: minOrderETH });

    // Calculate final balance including gas costs
    let gasUsed = new BN(txReceipt.receipt.gasUsed);
    let tx = await web3.eth.getTransaction(txReceipt.tx);
    let gasPrice = new BN(tx.gasPrice);
    let expectedETHBalance = startETHBalance.add(minOrderETH); // .sub(gasPrice.mul(gasUsed));
    let finalETHBalance = new BN(await web3.eth.getBalance(accounts[0]));
    let finalAUTOBalance = new BN(await koh.balanceOf(accounts[1]));

    assert.equal(await web3.eth.getBalance(koh.address), 0, "There should be no ETH left in the contract");
    assert.equal(await koh.getDEXBalance(), 0, "There should be no AUTO left in the DEX account");
    assert.equal(finalETHBalance.toString(), expectedETHBalance.toString(),
        "Seller should end up with requested ETH");
    assert.equal(finalAUTOBalance.toString(), minOrderAUTO.toString(), "Buyer should end up with requested AUTO");
  });

  it('calling buy() without any ether should fail', async () => {
    await catchRevert(koh.buy(minOrderAUTO), "Minimum ETH requirement not met");
  });

  it('calling buy() with payment less than minOrderETH should fail', async () => {
    await catchRevert(koh.buy(minOrderAUTO, {value: minOrderETH.subn(1)}), "Minimum ETH requirement not met");
  });

  it('calling buy() with less than minOrderAUTO should fail', async () => {
    await catchRevert(koh.buy(minOrderAUTO.subn(1), {value: minOrderETH}), "Minimum AUTO requirement not met");
  });

  it('buy() should create and register order correctly', async () => {
    await koh.buy(minOrderAUTO, {value: minOrderETH});
    assert.equal(await web3.eth.getBalance(koh.address), minOrderETH.toString(),
        "Incorrect ETH balance in contract after buy()");
    assert.equal(await koh.getOrdersLength(), 1, "Order not created");
    let order = await koh.orders(0);
    assert.equal(order.AUTO.toString(), minOrderAUTO.toString(), "Invalid AUTO in order");
    assert.equal(order.ETH.toString(), minOrderETH.toString(), "Invalid ETH in order");
    assert.equal(order.owner, accounts[0], "Invalid owner in order");
    assert.equal(order.orderType, 0, "Invalid order type");
  });

  it('calling sell() with less than minOrderAUTO should fail', async () => {
    await catchRevert(koh.sell(0, minOrderETH), "Minimum AUTO requirement not met");
  });

  it('calling sell() with less than minOrderETH should fail', async () => {
    await catchRevert(koh.sell(minOrderAUTO, 0), "Minimum ETH requirement not met");
  });

  it('calling sell() with insufficient AUTO should fail', async () => {
    await catchRevert(koh.sell(minOrderAUTO, minOrderETH, {from: accounts[3]}));
  });

  it('sell() should create and register order correctly', async () => {
    await koh.sell(minOrderAUTO, minOrderETH);
    assert.equal((await koh.getDEXBalance()).toString(), minOrderAUTO.toString(),
        "Incorrect AUTO balance in DEX after sell()");
    assert.equal(await koh.getOrdersLength(), 1, "Order not created");
    let order = await koh.orders(0);
    assert.equal(order.AUTO.toString(), minOrderAUTO.toString(), "Invalid AUTO in order");
    assert.equal(order.ETH.toString(), minOrderETH.toString(), "Invalid ETH in order");
    assert.equal(order.owner, accounts[0], "Invalid owner in order");
    assert.equal(order.orderType, 1, "Invalid order type");
  });

  it('cancel() after a buy() should return ETH back to owner', async () => {
    let startETHBalance = new BN(await web3.eth.getBalance(accounts[0]));
    let startAUTOBalance = new BN(await koh.balanceOf(accounts[0]));

    let tx1 = await koh.buy(minOrderAUTO, {value: minOrderETH});
    let tx2 = await koh.cancelOrder(0);

    // Calculate final balance including gas costs
    let gasUsed = (await calcGasUsed(tx1)).add(await calcGasUsed(tx2));
    let expectedETHBalance = startETHBalance.sub(gasUsed);
    let finalETHBalance = new BN(await web3.eth.getBalance(accounts[0]));
    let finalAUTOBalance = new BN(await koh.balanceOf(accounts[0]));

    assert.equal(finalAUTOBalance.toString(), startAUTOBalance.toString(), "AUTO should not have changed");
    assert.equal(finalETHBalance.toString(), expectedETHBalance.toString(),
        "ETH should have been returned (minus gas costs)");
  });

  it('cancel() after a sell() should return AUTO back to owner', async () => {
    let startETHBalance = new BN(await web3.eth.getBalance(accounts[0]));
    let startAUTOBalance = new BN(await koh.balanceOf(accounts[0]));

    let tx1 = await koh.sell(minOrderAUTO, minOrderETH);
    let tx2 = await koh.cancelOrder(0);

    let gas1 = await calcGasUsed(tx1);
    let gas2 = await calcGasUsed(tx2);

    // Calculate final balance including gas costs
    let gasUsed = gas1.add(gas2);
    let expectedETHBalance = startETHBalance.sub(gasUsed);
    let finalETHBalance = new BN(await web3.eth.getBalance(accounts[0]));
    let finalAUTOBalance = new BN(await koh.balanceOf(accounts[0]));

    assert.equal(finalETHBalance.toString(), expectedETHBalance.toString(),
        "Should not have changed ETH balance except for gas costs");
    assert.equal(finalAUTOBalance.toString(), startAUTOBalance.toString(), "AUTO should have been returned");
  });
});
