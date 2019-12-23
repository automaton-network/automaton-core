var KingAutomaton = artifacts.require("./KingAutomaton.sol");

module.exports = function(deployer) {
  numSlots = 256;
  difficultyBits = 16;
  mask = "0";
  initialDailySupply ="406080000";
  approval_percentage = 10;
  contest_percentage = -10;
  deployer.deploy(KingAutomaton, numSlots, difficultyBits, mask, initialDailySupply,
      approval_percentage, contest_percentage);
};
