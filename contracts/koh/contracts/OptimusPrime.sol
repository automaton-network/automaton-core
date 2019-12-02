pragma solidity ^0.5.11;

// Optimization testbed, currently exploring optimizing the voting process.

contract OptimusPrime {
  int256 constant INT256_MIN = int256(uint256(1) << 255);
  int256 constant INT256_MAX = int256(~(uint256(1) << 255));
  uint256 constant UINT256_MIN = 0;
  uint256 constant UINT256_MAX = ~uint256(0);
  uint256 constant MSB_SET = 1 << 255;
  uint256 constant ALL_BUT_MSB = MSB_SET - 1;

  // Voters
  uint256 public slots;
  mapping(uint256 => address) public owners;

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
    slots = 65536;
    numChoices = 2;
    require(numChoices <= 255);
    bitsPerVote = msb(numChoices) + 1;
    votesPerWord = 255 / bitsPerVote;
    votesWords = slots / votesPerWord;

    for (uint i = 0; i <= numChoices; i++) {
      voteCount[i] = MSB_SET;
    }
  }

  function unpaidSlots() public view returns (uint256) {
    return slots - paidVotes;
  }

  // Pay for multiple slots at once, 32 seems to be a reasonable amount.
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

    // Reduce the vote count.
    uint vote = votes[index];
    uint oldChoice = (vote & mask) >> offset;
    if (oldChoice > 0) {
      voteCount[oldChoice]--;
    }

    // Modify vote selection.
    vote &= (mask ^ UINT256_MAX); // get rid of current choice using a mask.
    vote |= _choice << offset;    // replace current choice using a mask.
    votes[index] = vote;          // actually update the storage slot.
    voteCount[_choice]++;         // update the total vote count based on the choice.

    // Incentivize voters by giving them a refund.
    payGas1[_slot] = 0;
    payGas2[_slot] = 0;
  }

  function getVote(uint256 _slot) public view returns (uint) {
    // Calculate masks.
    uint index = _slot / votesPerWord;
    uint offset = (_slot % votesPerWord) * bitsPerVote;
    uint baseMask = (1 << bitsPerVote) - 1;
    uint mask = baseMask << offset;

    // Get vote
    return (votes[index] & mask) >> offset;
  }

  function getVoteCount(uint256 _choice) public view returns(uint256) {
    return voteCount[_choice] & ALL_BUT_MSB;
  }
}
