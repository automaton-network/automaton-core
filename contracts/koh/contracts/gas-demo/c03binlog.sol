pragma solidity ^0.5.11;

// 0x8000000000000000000000000000000000000000000000000000000000000000
// 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
// 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF

contract OptimusPrime03 {
  function msb1(uint256 x) public pure returns (uint256 y) {
    while (x > 1) {
      x >>= 1;
      y++;
    }
  }

  function msb2(uint256 x) public pure returns (uint256 y) {
    // If most significant bit is set, x would overflow and become 0 when adding 1.
    // Therefore we handle this edge case prior.
    if ((x & 0x8000000000000000000000000000000000000000000000000000000000000000) > 0) { return 255; }

    // Smear set bits in x to the right.
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x |= x >> 64;
    x |= x >> 128;

    // Leave only the most significant set bit on.
    x++;
    x >>= 1;

    // x is now guaranteed to be a power of 2.
    if ((x & 0xAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA) > 0) { y |= 1; }
    if ((x & 0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC) > 0) { y |= 2; }
    if ((x & 0xF0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0) > 0) { y |= 4; }
    if ((x & 0xFF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00) > 0) { y |= 8; }
    if ((x & 0xFFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000) > 0) { y |= 16; }
    if ((x & 0xFFFFFFFF00000000FFFFFFFF00000000FFFFFFFF00000000FFFFFFFF00000000) > 0) { y |= 32; }
    if ((x & 0xFFFFFFFFFFFFFFFF0000000000000000FFFFFFFFFFFFFFFF0000000000000000) > 0) { y |= 64; }
    if ((x & 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000000000000000000000) > 0) { y |= 128; }
  }

  function msb3(uint256 x) public pure returns (uint256 r) {
    if (x >= 0x100000000000000000000000000000000) {x >>= 128; r += 128;}
    if (x >= 0x10000000000000000) {x >>= 64; r += 64;}
    if (x >= 0x100000000) {x >>= 32; r += 32;}
    if (x >= 0x10000) {x >>= 16; r += 16;}
    if (x >= 0x100) {x >>= 8; r += 8;}
    if (x >= 0x10) {x >>= 4; r += 4;}
    if (x >= 0x4) {x >>= 2; r += 2;}
    if (x >= 0x2) r += 1; // No need to shift x anymore
  }

  function msb4(uint256 x) public pure returns (uint256 r) {
    assembly {
      if gt(x, 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF) { x := shr(128, x) r := add(r, 128) }
      if gt(x, 0xFFFFFFFFFFFFFFFF) { x := shr(64, x) r := add(r, 64) }
      if gt(x, 0xFFFFFFFF) { x := shr(32, x) r := add(r, 32) }
      if gt(x, 0xFFFF) { x := shr(16, x) r := add(r, 16) }
      if gt(x, 0xFF) { x := shr(8, x) r := add(r, 8) }
      if gt(x, 0xF) { x := shr(4, x) r := add(r, 4) }
      if gt(x, 0x3) { x := shr(2, x) r := add(r, 2) }
      if gt(x, 0x1) { r := add(r, 1) }
    }
  }
}
