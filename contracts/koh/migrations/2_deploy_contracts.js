var KingAutomaton = artifacts.require("./KingAutomaton.sol");

module.exports = function(deployer) {
  numSlots = 65536;
  difficultyBits = 16;
  mask = "53272589901149737477561849970166322707710816978043543010898806474236585144509";
  initialDailySupply ="406080000";
  deployer.deploy(KingAutomaton, numSlots, difficultyBits, mask, initialDailySupply);
};
