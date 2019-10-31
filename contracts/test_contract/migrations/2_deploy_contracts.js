var test_contract = artifacts.require("./test_contract.sol");

module.exports = function(deployer) {
  deployer.deploy(test_contract);
};
