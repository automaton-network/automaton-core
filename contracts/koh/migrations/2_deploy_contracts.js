var KingAutomaton = artifacts.require("./KingAutomaton.sol");

module.exports = function(deployer) {
  numSlots = 4;
  difficultyBits = 16;
  mask = "0x010000";
  initialDailySupply ="406080000";
  deployer.deploy(KingAutomaton, numSlots, difficultyBits, mask, initialDailySupply);
};
