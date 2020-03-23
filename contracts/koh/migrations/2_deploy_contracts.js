var KingAutomaton = artifacts.require("KingAutomaton");
var Util = artifacts.require("Util");
var DEX = artifacts.require("DEX");
var KingOfTheHill = artifacts.require("KingOfTheHill");
var Proposals = artifacts.require("Proposals");

module.exports = function(deployer) {
  deployer.deploy(Util);
  deployer.link(Util, KingOfTheHill);
  deployer.link(Util, KingAutomaton);
  deployer.link(Util, Proposals);
  deployer.deploy(DEX);
  deployer.link(DEX, KingAutomaton);
  deployer.deploy(Proposals);
  deployer.link(Proposals, KingAutomaton);

  numSlots = 256;
  difficultyBits = 16;
  mask = "0x10000";
  initialDailySupply ="406080000";
  approval_percentage = 10;
  contest_percentage = -10;
  treasury_limit_percentage = 2;

  deployer.deploy(KingAutomaton, numSlots, difficultyBits, mask, initialDailySupply,
      approval_percentage, contest_percentage, treasury_limit_percentage);
};
