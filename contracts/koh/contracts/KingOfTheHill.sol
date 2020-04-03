pragma solidity ^0.6.2;

import "./Util.sol";

// solhint-disable-next-line
abstract contract KingOfTheHill {
  event NewSlotKing(uint256 slot, address newOwner);

  struct ValidatorSlot {
    address owner;
    uint256 difficulty;
    uint256 last_claim_time;
  }

  mapping (uint256 => ValidatorSlot) slots;
  uint256 public numSlots;

  uint256 public minDifficulty;          // Minimum difficulty
  uint256 public mask;                   // Prevents premine
  uint256 public numTakeOvers;           // Number of times a slot was taken over by a new king.

  /** Claims slot based on a signature.
    * @param pubKeyX X coordinate of the public key used to claim the slot
    * @param pubKeyY Y coordinate of the public key used to claim the slot
    * @param v recId of the signature needed for ecrecover
    * @param r R portion of the signature
    * @param s S portion of the signature
  */

  function claimSlot(bytes32 pubKeyX, bytes32 pubKeyY, uint8 v, bytes32 r, bytes32 s) public {
    require(Util.verifySignature(pubKeyX, pubKeyY, bytes32(uint256(msg.sender)), v, r, s), "Signature not valid");
    uint256 id = uint256(pubKeyX) % numSlots;
    uint256 key = uint256(pubKeyX) ^ mask;
    ValidatorSlot memory slot = slots[id];
    require(key > minDifficulty && key > slot.difficulty, "Low key difficulty");

    uint256 lastTime = slot.last_claim_time;
    if (lastTime != 0) {
      require (lastTime < now, "mining same slot in same block not allowed!");
    }
    slotAcquired(id);
    // Update the slot with data for the new king.
    ValidatorSlot memory newSlot;
    newSlot.owner = msg.sender;
    newSlot.difficulty = key;
    newSlot.last_claim_time = now;

    slots[id] = newSlot;
    numTakeOvers++;

    emit NewSlotKing(id, msg.sender);
  }

  function slotAcquired(uint256 _id) internal virtual;

  function setMinDifficulty(uint256 minDifficultyBits) internal {
    require(minDifficultyBits > 0);
    minDifficulty = (2 ** minDifficultyBits - 1) << (256 - minDifficultyBits);
  }

  function setMask(uint256 _mask) internal {
    mask = _mask;
  }

  function getSlot(uint256 slot) public view returns (address owner, uint256 difficulty, uint256 last_claim_time) {
    ValidatorSlot memory s = slots[slot];
    owner = s.owner;
    difficulty = s.difficulty;
    last_claim_time = s.last_claim_time;
  }

  function getSlotOwner(uint256 slot) public view returns(address) {
    return slots[slot].owner;
  }

  function getSlotDifficulty(uint256 slot) public view returns(uint256) {
    return slots[slot].difficulty;
  }

  function getSlotLastClaimTime(uint256 slot) public view returns(uint256) {
    return slots[slot].last_claim_time;
  }

  function getOwners(uint256 start, uint256 len) public view returns(address[] memory result) {
    result = new address[](len);
    for(uint256 i = 0; i < len; i++) {
      result[i] = slots[start + i].owner;
    }
  }

  function getDifficulties(uint256 start, uint256 len) public view returns(uint256[] memory result) {
    result = new uint256[](len);
    for(uint256 i = 0; i < len; i++) {
      result[i] = slots[start + i].difficulty;
    }
  }

  function getLastClaimTimes(uint256 start, uint256 len) public view returns(uint256[] memory result) {
    result = new uint256[](len);
    for(uint256 i = 0; i < len; i++) {
      result[i] = slots[start + i].last_claim_time;
    }
  }
}
