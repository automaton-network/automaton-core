pragma solidity ^0.5.11;

// Demo 01 - Variable packing

// 1,2,3,4,5
// [1,2,3,4,5]
// ["0x01", "0x02", "0x03", "0x04", "0x05"]

// Examples 01a, 01b - global contract member variables.
// TX COST
// --------------------------
// OptimusPrime01a: 73014 gas
// OptimusPrime01b: 53264 gas
// --------------------------
// Savings:         19750 gas

contract OptimusPrime01a {
  uint128 a;
  uint256 b;
  uint128 c;

  constructor() public {
    a = 1;
    b = 2;
    c = 3;
  }
}

contract OptimusPrime01b {
  uint256 b;
  uint128 a;
  uint128 c;

  constructor() public {
    a = 1;
    b = 2;
    c = 3;
  }
}

// Examples 01a, 01b - global contract member structure variable
// TX COST
// --------------------------
// OptimusPrime01c: 73032 gas
// OptimusPrime01d: 53282 gas
// --------------------------
// Savings:         19750 gas

contract OptimusPrime01c {
  struct S {
    uint128 a;
    uint256 b;
    uint128 c;
  }

  S s;
  constructor() public {
    s.a = 1;
    s.b = 2;
    s.c = 3;
  }
}

contract OptimusPrime01d {
  struct S {
    uint256 b;
    uint128 a;
    uint128 c;
  }

  S s;
  constructor() public {
    s.a = 1;
    s.b = 2;
    s.c = 3;
  }
}

// - packing function arguments and results

contract OptimusPrime01e {
  function f01(uint8 a, uint8 b, uint8 c, uint8 d, uint8 e)
      public pure returns(uint8 x) {
    x = a + b + c + d + e;
  }

  function f02(uint8 a, uint8 b, uint8 c, uint8 d, uint8 e)
      public pure returns(uint256 x) {
    x = a + b + c + d + e;
  }

  function f03(uint256 a, uint256 b, uint256 c, uint256 d, uint256 e)
      public pure returns(uint256 x) {
    x = a + b + c + d + e;
  }

  function f04(uint256 a)
      public pure returns(uint256 x) {
    x = (a + (a >> 8) + (a >> 16) + (a >> 24) + (a >> 32)) & 0xff;
  }

  function f05(uint256[] memory a)
      public pure returns(uint256 x) {
    x = a[0] + a[1] + a[2] + a[3] + a[4];
  }

  function f06(uint256[5] memory a)
      public pure returns(uint256 x) {
    x = a[0] + a[1] + a[2] + a[3] + a[4];
  }

  function f07(bytes memory a)
      public pure returns(uint256 x) {
    x = uint8(a[0])
      + uint8(a[1])
      + uint8(a[2])
      + uint8(a[3])
      + uint8(a[4]);
  }

  function f08(byte[5] memory a)
      public pure returns(uint256 x) {
    x = uint8(a[0])
      + uint8(a[1])
      + uint8(a[2])
      + uint8(a[3])
      + uint8(a[4]);
  }

  function f09(bytes calldata a)
      external pure returns(uint256 x) {
    x = uint8(a[0])
      + uint8(a[1])
      + uint8(a[2])
      + uint8(a[3])
      + uint8(a[4]);
  }

  function f10(byte[5] calldata a)
      external pure returns(uint256 x) {
    x = uint8(a[0])
      + uint8(a[1])
      + uint8(a[2])
      + uint8(a[3])
      + uint8(a[4]);
  }
}
