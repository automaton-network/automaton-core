pragma solidity ^0.6.2;

import "truffle/Assert.sol";
import "truffle/DeployedAddresses.sol";
import "../contracts/KingAutomaton.sol";

contract KingAutomatonNoDebug is KingAutomaton {
  // Set mask to 0 as it should be in the live contract.
  constructor() KingAutomaton (16, 4, 0, 406080000, 10, -10, 2) public {}
}

contract TestKingAutomatonDebug {
  KingAutomatonNoDebug koh;

  function beforeEach() public {
    koh = new KingAutomatonNoDebug();
  }

  function afterEach() public {
    delete koh;
  }

  function testDebugMask() public {
    Assert.notEqual(koh.mask(), 0, "Mask should not be 0 in live contract!");
  }

  function testDebugFlagFalse() public {
    Assert.isFalse(koh.debugging(), "When mask is not set Debug flag should be false!");
  }

  function internalSetOwner() public {
    koh.setOwner(0, address(1));
  }

  function testSetOwnerDebugOnly() public {
    bool r;
    (r, ) = address(this).call(abi.encodePacked(this.internalSetOwner.selector));
    Assert.isFalse(r, "setOwner should have failed!");
  }
}
