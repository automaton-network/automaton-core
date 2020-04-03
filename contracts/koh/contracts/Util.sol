pragma solidity ^0.6.2;

library Util {
  // Returns the Ethereum address corresponding to the input public key.
  function getAddressFromPubKey(bytes32 pubkeyX, bytes32 pubkeyY) public pure returns (uint256) {
    return uint256(keccak256(abi.encodePacked(pubkeyX, pubkeyY))) & 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
  }

  // Verifies that signature of a message matches the given public key.
  function verifySignature(
    bytes32 pubkeyX,
    bytes32 pubkeyY,
    bytes32 hash,
    uint8 v,
    bytes32 r,
    bytes32 s) public pure returns (bool) {
    uint256 addr = getAddressFromPubKey(pubkeyX, pubkeyY);
    address addr_r = ecrecover(hash, v, r, s);
    return addr == uint256(addr_r);
  }

  function msb(uint256 x) public pure returns (uint8 r) {
    if (x >= 0x100000000000000000000000000000000) {x >>= 128; r += 128;}
    if (x >= 0x10000000000000000) {x >>= 64; r += 64;}
    if (x >= 0x100000000) {x >>= 32; r += 32;}
    if (x >= 0x10000) {x >>= 16; r += 16;}
    if (x >= 0x100) {x >>= 8; r += 8;}
    if (x >= 0x10) {x >>= 4; r += 4;}
    if (x >= 0x4) {x >>= 2; r += 2;}
    if (x >= 0x2) r += 1; // No need to shift x anymore
  }
}
