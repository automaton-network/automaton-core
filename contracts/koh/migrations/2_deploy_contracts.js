var KingAutomaton = artifacts.require("./KingAutomaton.sol");

module.exports = function(deployer) {
  deployer.deploy(KingAutomaton);
};
