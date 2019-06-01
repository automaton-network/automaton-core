pragma solidity >=0.4.22 <0.6.0;

contract KingAutomaton {
  uint constant minDifficulty = 0xffff000000000000000000000000000000000000000000000000000000000000;

  struct ValidatorSlot {

    address owner;

    // Holds the difficulty for each validator slot.
    uint difficulty;

    // The last time the slot was claimed
    uint last_claim_time;
  }

  ValidatorSlot[] slots;

  // Total time held by a validator
  mapping(address => uint) total_time_held;

  // Prevents keys premine prior to contract deployment.
  uint mask;

  // Total slots claimed.
  uint claimed;

  function getSlotsNumber() public view returns(uint) {
    return slots.length;
  }

  function getSlotOwner(uint32 slot) public view returns(address) {
    return slots[slot].owner;
  }

  function getSlotDifficulty(uint32 slot) public view returns(uint) {
    return slots[slot].difficulty;
  }

  function getSlotLastClaimTime(uint32 slot) public view returns(uint) {
    return slots[slot].last_claim_time;
  }

  function getMask() public view returns(uint) {
    return mask;
  }

  function getClaimed() public view returns(uint) {
    return claimed;
  }

  function getTotalTimeHeld(address rewardAddress) public view returns(uint) {
      return total_time_held[rewardAddress];
  }

  constructor(uint numSlots, bool preventPremine) public {
    if (preventPremine) {
      mask = uint(keccak256(abi.encodePacked(now, msg.sender)));
    }
    slots.length = numSlots;
  }

  // Returns the Ethereum address corresponding to the input public key.
  function getAddressFromPubKey(
      bytes32 pubkeyX,
      bytes32 pubkeyY
    ) public pure returns (uint) {
    uint pkhash = uint(keccak256(abi.encodePacked(pubkeyX, pubkeyY)));
    return pkhash & 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
  }

  // Verifies that signature of a message matches the given public key.
  function verifySignature(
      bytes32 pubkeyX,
      bytes32 pubkeyY,
      bytes32 hash,
      uint8 v,
      bytes32 r,
      bytes32 s
    ) public payable returns (bool) {
    uint addr = getAddressFromPubKey(pubkeyX, pubkeyY);
    address addr_r = ecrecover(hash, v, r, s);
    return addr == uint(addr_r);
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
    uint32 slot = uint32(uint256(key) % slots.length);

    // Check if the key can take over the slot and become the new king.
    if (key <= minDifficulty || key <= slots[slot].difficulty) {
      return;
    }

    // Make sure the signature is valid.
    if (!verifySignature(pubKeyX, pubKeyY, bytes32(uint(rewardAddress)), v, r, s)) {
      return;
    }

    claimed++;
    slots[slot].difficulty = key;

    // Reward kicked out validator
    if(slots[slot].last_claim_time != 0) {
      total_time_held[rewardAddress] += now - slots[slot].last_claim_time;
    }

    // Record the time when the slot was acquired
    slots[slot].last_claim_time = now;
    slots[slot].owner = rewardAddress;
  }
}
