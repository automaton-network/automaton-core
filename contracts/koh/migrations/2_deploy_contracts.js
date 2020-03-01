var KingAutomaton = artifacts.require("KingAutomaton");
var Helpers = artifacts.require("Helpers");
var DEX = artifacts.require("DEX");
var Miner = artifacts.require("Miner");
var Proposals = artifacts.require("Proposals");

module.exports = function(deployer) {
  deployer.deploy(Helpers);
  deployer.link(Helpers, KingAutomaton);
  deployer.deploy(DEX);
  deployer.link(DEX, KingAutomaton);
  deployer.deploy(Miner);
  deployer.link(Miner, KingAutomaton);
  deployer.deploy(Proposals);
  deployer.link(Proposals, KingAutomaton);

  numSlots = 256;
  difficultyBits = 16;
  mask = "0";
  initialDailySupply ="406080000";
  approval_percentage = 10;
  contest_percentage = -10;
  treasury_limit_percentage = 2;

  deployer.deploy(KingAutomaton, numSlots, difficultyBits, mask, initialDailySupply,
      approval_percentage, contest_percentage, treasury_limit_percentage);
};
