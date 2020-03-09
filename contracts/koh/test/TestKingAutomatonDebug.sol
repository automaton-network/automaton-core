pragma solidity ^0.6.2;

import "truffle/Assert.sol";
import "truffle/DeployedAddresses.sol";
import "../contracts/KingAutomaton.sol";

contract KingAutomatonDebug is KingAutomaton {
  // Set mask to 0x10000 to trigger debug mode.
  constructor() KingAutomaton (16, 4, 0x10000, 406080000, 10, -10, 2) public {}
}

contract TestKingAutomatonDebug {
  KingAutomatonDebug koh;

  function beforeEach() public {
    koh = new KingAutomatonDebug();
  }

  function afterEach() public {
    delete koh;
  }

  function testDebugMask() public {
    Assert.equal(koh.mask(), 0x10000, "Mask should have been 0x10000");
  }

  function testDebugFlagTrue() public {
    Assert.isTrue(koh.debugging(), "When mask is set Debug flag should be true!");
  }

  function testSetOwnerDebugOnly() public {
    koh.setOwner(0, address(1));
  }
}
