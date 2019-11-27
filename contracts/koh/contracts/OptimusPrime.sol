pragma solidity ^0.5.11;

// Optimization testbed, currently exploring optimizing the voting process.

contract OptimusPrime {
  int256 constant INT256_MIN = int256(uint256(1) << 255);
  int256 constant INT256_MAX = int256(~(uint256(1) << 255));
  uint256 constant UINT256_MIN = 0;
  uint256 constant UINT256_MAX = ~uint256(0);
  uint256 constant MSB_SET = 1 << 255;

  // Voters
  uint256 public slots;
  address[] public owners;

  uint256 public numChoices;
  uint256 public bitsPerVote;
  uint256 public votesPerWord;
  uint256 public votesWords;
  uint256 public paidVotes;

  mapping (uint256 => uint256) public votes;
  mapping (uint256 => uint256) public voteCount;
  mapping (uint256 => uint256) public payGas1;
  mapping (uint256 => uint256) public payGas2;

  //////////////////////////////////////////////////////////////////////////////

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

  constructor() public {
    uint _packedSlotsAndNumChoices = 0x100000002;
    slots = _packedSlotsAndNumChoices >> 16;
    numChoices = _packedSlotsAndNumChoices & 0xFFFF;
    require(numChoices <= 255);
    bitsPerVote = msb(numChoices + 1) + 1;
    votesPerWord = 255 / bitsPerVote;
    owners.length = slots;
    votesWords = slots / votesPerWord;

    for (uint i = 0; i <= numChoices; i++) {
      voteCount[i] = MSB_SET;
    }
  }

  function unpaidSlots() public view returns (uint256) {
    return slots - paidVotes;
  }

  function payForGas(uint256 _slotsToPay) public {
    require(paidVotes + _slotsToPay <= slots);
    uint _newLength = paidVotes + _slotsToPay;
    uint _votesWords = votesWords;
    for (uint i = 1; i <= _slotsToPay; i++) {
      uint idx = _newLength - i;
      payGas1[idx] = payGas2[idx] = MSB_SET;
      if (_newLength - i <= _votesWords) {
        votes[idx] = MSB_SET;
      }
    }
    paidVotes = _newLength;
  }

  function setOwner(uint256 _slot) public {
    owners[_slot] = msg.sender;
  }

  function castVote(uint256 _slot, uint _choice) public {
    require(msg.sender == owners[_slot], "Invalid slot owner");
    require(_choice <= numChoices, "Invalid choice");

    // Calculate masks.
    uint index = _slot / votesPerWord;
    uint offset = (_slot % votesPerWord) * bitsPerVote;
    uint baseMask = (1 << bitsPerVote) - 1;
    uint mask = baseMask << offset;

    index = index * 8;

    // Modify vote selection.
    uint vote = votes[index];
    uint oldChoice = (vote & mask) >> offset;
    if (oldChoice > 0) {
      voteCount[oldChoice]--;
    }
    vote &= (mask ^ UINT256_MAX);
    vote |= _choice << offset;
    votes[index] = vote;
    voteCount[_choice]++;
    payGas1[_slot] = 0;
    payGas2[_slot] = 0;
  }

  function getVote(uint256 _slot) public view returns (uint) {
    // Calculate masks.
    uint index = _slot / votesPerWord;
    uint offset = (_slot % votesPerWord) * bitsPerVote;
    uint baseMask = (1 << bitsPerVote) - 1;
    uint mask = baseMask << offset;

    index = index * 4;

    // Get vote
    return (votes[index] & mask) >> offset;
  }
}
