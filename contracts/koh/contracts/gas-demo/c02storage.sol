pragma solidity ^0.5.11;

contract OptimusPrime02a {
  uint256 public s;

  // 3 SLOAD instructions
  function readStorage() public view returns (bool) {
    return (s == 25) || (s == 50) || (s == 75);
  }

  // 1 SLOAD and 3 MLOAD instructions
  function readMemory() public view returns (bool) {
    uint256 m = s;
    return (m == 25) || (m == 50) || (m == 75);
  }

  // 1 SSTORE instruction
  function writeStorage(uint256 _s) public {
    s = _s;
  }
}
