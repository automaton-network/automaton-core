pragma solidity >=0.4.22 <0.6.0;

// truffle console
// KingAutomaton.deployed().then(inst => {koh = inst})
// koh.claimSlot('0x439ADC46A4FE473092BA43D3C596D8F6F192C411A3DFB8D9CBCABE36120CFABE', '0xD1EE124FCF6F153F5C02AF4C2261D791A28B380970734159518165FD38FA81EA', '0x0000000000000000000000000000000000000000', '0x1C', '0x61E3FA4EE6ED88A865C890630E50067AF03348CA403B10AB71A75F46B42F9C68', '0x0FE180B4FF6659FE220DA4C91949B7F64B8EE0E954B0AF3C1E6CB95591B3D408')

contract KingAutomaton {
  // *** DATA ***

  // Holds the difficulty for each validator slot.
  mapping(uint32 => uint) slots;

  // Holds the Ethereum reward address for each validator slot.
  mapping(uint32 => address) rewards;

  // Prevents keys premine prior to contract deployment.
  uint mask;

  // Total slots claimed.
  uint claimed;

  // *** GETTERS ***

  function getSlot(uint32 slot) public view returns(uint) {
    return slots[slot];
  }

  function getRewardAddress(uint32 slot) public view returns(address) {
    return rewards[slot];
  }

  function getMask() public view returns(uint) {
    return mask;
  }

  function getClaimed() public view returns(uint) {
    return claimed;
  }

  constructor() public {
    mask = uint(keccak256(abi.encode(blockhash(block.number-1))));
  }

  // Verifies that signature of a message matches the given public key.
  function verifySignature(
      bytes32 pubkeyX,
      bytes32 pubkeyY,
      bytes32 hash,
      uint8 v,
      bytes32 r,
      bytes32 s
    ) public pure returns (bool) {
    uint addr = getAddressFromPubKey(pubkeyX, pubkeyY);
    address addr_r = ecrecover(hash, v, r, s);
    return addr == uint(addr_r);
  }

  // Returns the Ethereum address corresponding to the input public key.
  function getAddressFromPubKey(
      bytes32 pubkeyX,
      bytes32 pubkeyY
    ) public pure returns (uint) {
    uint pkhash = uint(keccak256(abi.encodePacked(pubkeyX, pubkeyY)));
    return pkhash & 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
  }

  /** Claims slot based on a signature.
    * @param pubKeyX X coordinate of the public key used to claim the slot
    * @param pubKeyY Y coordinate of the public key used to claim the slot
    * @param rewardAddress Ethereum address where rewards will be sent
    * @param v recId of the signature needed for ecrecover
    * @param r R portion of the signature
    * @param s S portion of the signature
    */
  function claimSlot(
    bytes32 pubKeyX,
    bytes32 pubKeyY,
    address rewardAddress,
    uint8 v,
    bytes32 r,
    bytes32 s
  ) public {
    uint key = uint(pubKeyX) ^ mask;
    uint32 slot = uint32(uint256(key) & 0x00FFFF);

    // Check if the key can take over the slot and become the new king.
    if (key <= slots[slot]) {
      return;
    }

    // Make sure the signature is valid.
    if (!verifySignature(pubKeyX, pubKeyY, bytes32(uint(rewardAddress)), v, r, s)) {
      return;
    }

    claimed++;
    slots[slot] = key;
    rewards[slot] = rewardAddress;
  }
}
