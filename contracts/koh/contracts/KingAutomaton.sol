pragma solidity ^0.5.11;

contract KingAutomaton {
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Initialization
  //////////////////////////////////////////////////////////////////////////////////////////////////

  constructor(uint256 numSlots, uint8 minDifficultyBits, uint256 predefinedMask, uint256 initialDailySupply) public {
    initMining(numSlots, minDifficultyBits, predefinedMask, initialDailySupply);
    initNames();
    initTreasury();

    // Check if we're on a testnet (We will not using predefined mask when going live)
    if (predefinedMask != 0) {
      // If so, fund the owner for debugging purposes.
      debugging = true;
      mint(msg.sender, 1000000 ether);
    }
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // ERC20 Token
  //////////////////////////////////////////////////////////////////////////////////////////////////

  // Special purpose community managed addresses.
  address treasuryAddress = address(1);
  address DEXAddress = address(2);

  string public constant name = "Automaton Network Validator Bootstrap";
  string public constant symbol = "AUTO";
  uint8 public constant decimals = 18;
  uint256 public totalSupply = 0;
  uint256 public maxSupply = 1000000000000 ether;

  // solhint-disable-next-line no-simple-event-func-name
  event Transfer(address indexed _from, address indexed _to, uint256 _value);
  event Approval(address indexed _owner, address indexed _spender, uint256 _value);

  uint256 constant private MAX_uint = 2**256 - 1;
  mapping (address => uint256) public balances;
  mapping (address => mapping (address => uint256)) public allowed;

  function transfer(address _to, uint256 _value) public returns (bool success) {
    require(balances[msg.sender] >= _value);
    balances[msg.sender] -= _value;
    balances[_to] += _value;
    emit Transfer(msg.sender, _to, _value); //solhint-disable-line indent, no-unused-vars
    return true;
  }

  function transferFrom(address _from, address _to, uint256 _value) public returns (bool success) {
    uint256 allowance = allowed[_from][msg.sender];
    require(balances[_from] >= _value && allowance >= _value);
    balances[_to] += _value;
    balances[_from] -= _value;
    if (allowance < MAX_uint) {
        allowed[_from][msg.sender] -= _value;
    }
    emit Transfer(_from, _to, _value); //solhint-disable-line indent, no-unused-vars
    return true;
  }

  function balanceOf(address _owner) public view returns (uint256 balance) {
    return balances[_owner];
  }

  function approve(address _spender, uint256 _value) public returns (bool success) {
    allowed[msg.sender][_spender] = _value;
    emit Approval(msg.sender, _spender, _value); //solhint-disable-line indent, no-unused-vars
    return true;
  }

  function allowance(address _owner, address _spender) public view returns (uint256 remaining) {
    return allowed[_owner][_spender];
  }

  // This is only to be used with special purpose community accounts like Treasury, DEX.
  // Those accounts help to represent the total supply correctly.
  function transferInternal(address _from, address _to, uint256 _value) private returns (bool success) {
    require(balances[_from] >= _value, "Insufficient balance");
    balances[_to] += _value;
    balances[_from] -= _value;
    emit Transfer(_from, _to, _value); //solhint-disable-line indent, no-unused-vars
    return true;
  }

  function mint(address _receiver, uint256 _value) private {
    balances[_receiver] += _value;
    totalSupply += _value;
    emit Transfer(address(0), _receiver, _value); //solhint-disable-line indent, no-unused-vars
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Treasury
  //////////////////////////////////////////////////////////////////////////////////////////////////

  int256 constant INT256_MIN = int256(uint256(1) << 255);
  int256 constant INT256_MAX = int256(~(uint256(1) << 255));
  uint256 constant UINT256_MIN = 0;
  uint256 constant UINT256_MAX = ~uint256(0);
  uint256 constant MSB_SET = 1 << 255;
  uint256 constant ALL_BUT_MSB = MSB_SET - 1;

  uint256 constant NUM_CHOICES = 2;
  uint256 constant BITS_PER_VOTE = 2;
  uint256 constant VOTES_PER_WORD = 127;
  uint256 constant BASE_MASK = (1 << BITS_PER_VOTE) - 1;

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

  enum BallotBoxState {Uninitialized, PrepayingGas, Active, Inactive}
  enum ProposalState {Uninitialized, Accepted, Rejected, Contested, Completed}

  struct BallotBox {
    BallotBoxState state;

    uint256 votesWords;
    uint256 paidSlots;

    mapping (uint256 => uint256) votes;
    mapping (uint256 => uint256) voteCount;
    mapping (uint256 => uint256) payGas1;
    mapping (uint256 => uint256) payGas2;
  }

  struct Proposal {
    address payable contributor;
    string title;
    string documentsLink;
    bytes documentsHash;

    ProposalState state;

    uint256 probationEndDate;
    uint256 probationPeriod;
    uint256 approvalPercentage;
  }

  bool public debugging = false;

  mapping (uint256 => BallotBox) public ballot_boxes;

  modifier validBallotBoxID(uint256 id) {
    require(ballot_boxes[id].state != BallotBoxState.Uninitialized, "Invalid ballot box ID!");
    _;
  }

  modifier activeBallot(uint256 id) {
    require(ballot_boxes[id].state == BallotBoxState.Active, "Ballot is not active!");
    _;
  }

  modifier debugOnly() {
    require(debugging, "Available only in debug mode!");
    _;
  }

  function initTreasury() private {}

  function createBallotBox(uint256 id) public {
    require(ballot_boxes[id].state == BallotBoxState.Uninitialized);

    BallotBox storage p = ballot_boxes[id];
    p.state = BallotBoxState.PrepayingGas;
    p.votesWords = (slots.length + VOTES_PER_WORD - 1) / VOTES_PER_WORD;

    for (uint i = 0; i <= NUM_CHOICES; i++) {
      p.voteCount[i] = MSB_SET;
    }

    payForGas(id, 1);
  }

  function unpaidSlots(uint256 ballot_box_id) public view validBallotBoxID(ballot_box_id) returns (uint256) {
    return slots.length - ballot_boxes[ballot_box_id].paidSlots;
  }

  // FOR TEST PURPOSES, TO BE DELETED
  function setOwner(uint256 _slot, address new_owner) public debugOnly {
    slots[_slot].owner = new_owner;
  }

  // Pay for multiple slots at once, 32 seems to be a reasonable amount.
  function payForGas(uint256 ballot_box_id, uint256 _slotsToPay) public validBallotBoxID(ballot_box_id) {
    BallotBox storage p = ballot_boxes[ballot_box_id];
    require((slots.length - p.paidSlots) >= _slotsToPay, "Too many slots!");
    uint _newLength = p.paidSlots + _slotsToPay;
    for (uint i = 1; i <= _slotsToPay; i++) {
      uint idx = _newLength - i;
      p.payGas1[idx] = p.payGas2[idx] = MSB_SET;
      if (_newLength - i <= p.votesWords) {
        p.votes[idx] = MSB_SET;
      }
    }
    p.paidSlots = _newLength;
    if (p.paidSlots == slots.length) {
        p.state = BallotBoxState.Active;
    }
  }

  function calcVoteDifference(uint256 ballot_box_id) view private validBallotBoxID(ballot_box_id) returns (uint256) {
    BallotBox storage p = ballot_boxes[ballot_box_id];
    uint256 yes = p.voteCount[1];
    uint256 no = p.voteCount[2];
    if (yes > no) {
      return (yes - no) / slots.length;
    }
    return (no - yes) / slots.length;
  }

  function castVote(uint256 ballot_box_id, uint256 _slot, uint8 _choice)
      public validBallotBoxID(ballot_box_id) activeBallot(ballot_box_id) {
    BallotBox storage p = ballot_boxes[ballot_box_id];
    require(msg.sender == slots[_slot].owner, "Invalid slot owner");
    require(_choice <= 2, "Invalid choice");

    // Calculate masks.
    uint index = _slot / VOTES_PER_WORD;
    uint offset = (_slot % VOTES_PER_WORD) * BITS_PER_VOTE;
    uint mask = BASE_MASK << offset;

    // Reduce the vote count.
    uint vote = p.votes[index];
    uint oldChoice = (vote & mask) >> offset;
    if (oldChoice > 0) {
      p.voteCount[oldChoice]--;
    }

    // Modify vote selection.
    vote &= (mask ^ UINT256_MAX);   // get rid of current choice using a mask.
    vote |= _choice << offset;      // replace current choice using a mask.
    p.votes[index] = vote;          // actually update the storage slot.
    p.voteCount[_choice]++;         // update the total vote count based on the choice.

    // Incentivize voters by giving them a refund.
    p.payGas1[_slot] = 0;
    p.payGas2[_slot] = 0;
  }

  function getVote(uint256 ballot_box_id, uint256 _slot) public view validBallotBoxID(ballot_box_id) returns (uint) {
    BallotBox storage p = ballot_boxes[ballot_box_id];

    // Calculate masks.
    uint index = _slot / VOTES_PER_WORD;
    uint offset = (_slot % VOTES_PER_WORD) * BITS_PER_VOTE;
    uint mask = BASE_MASK << offset;

    // Get vote
    return (p.votes[index] & mask) >> offset;
  }

  function getVoteCount(uint256 ballot_box_id, uint256 _choice) public view validBallotBoxID(ballot_box_id) returns(uint256) {
    return ballot_boxes[ballot_box_id].voteCount[_choice] & ALL_BUT_MSB;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Mining
  //////////////////////////////////////////////////////////////////////////////////////////////////

  event NewSlotKing(uint256 slot, address newOwner);

  struct ValidatorSlot {
    address owner;
    uint256 difficulty;
    uint256 last_claim_time;
  }
  ValidatorSlot[] public slots;

  uint256 public minDifficulty;          // Minimum difficulty
  uint256 public mask;                   // Prevents premine
  uint256 public numTakeOvers;           // Number of times a slot was taken over by a new king.
  uint256 public rewardPerSlotPerSecond; // Validator reward per slot per second.

  function initMining(uint256 numSlots, uint8 minDifficultyBits, uint256 predefinedMask, uint256 initialDailySupply) private {
    require(numSlots > 0);
    require(minDifficultyBits > 0);

    slots.length = numSlots;
    minDifficulty = (2 ** uint256(minDifficultyBits) - 1) << (256 - minDifficultyBits);
    if (predefinedMask == 0) {
      // Prevents premining with a known predefined mask.
      mask = uint256(keccak256(abi.encodePacked(now, msg.sender)));
    } else {
      // Setup predefined mask, useful for testing purposes.
      mask = predefinedMask;
    }

    rewardPerSlotPerSecond = (1 ether * initialDailySupply) / 1 days / numSlots;
  }

  function getSlotsNumber() public view returns(uint256) {
    return slots.length;
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

  function getMask() public view returns(uint256) {
    return mask;
  }

  function getMinDifficulty() public view returns(uint256) {
    return minDifficulty;
  }

  function getClaimed() public view returns(uint256) {
    return numTakeOvers;
  }

  /** Claims slot based on a signature.
    * @param pubKeyX X coordinate of the public key used to claim the slot
    * @param pubKeyY Y coordinate of the public key used to claim the slot
    * @param v recId of the signature needed for ecrecover
    * @param r R portion of the signature
    * @param s S portion of the signature
    */
  function claimSlot(
    bytes32 pubKeyX,
    bytes32 pubKeyY,
    uint8 v,
    bytes32 r,
    bytes32 s
  ) public {
    uint256 slot = uint256(pubKeyX) % slots.length;
    uint256 key = uint256(pubKeyX) ^ mask;

    // Check if the key can take over the slot and become the new king.
    require(key > minDifficulty && key > slots[slot].difficulty, "Low key difficulty");

    // Make sure the signature is valid.
    require(verifySignature(pubKeyX, pubKeyY, bytes32(uint256(msg.sender)), v, r, s), "Signature not valid");

    require(totalSupply < maxSupply, "Cap reached");

    /*
      Test in python:

      eth = 1000000000000000000.0
      def f(timeHours, totalSupply):
        rewardPerSlotPerSecond = 71716308593750000 # 64K slots, 406080000 reward per day
        timeDelta = timeHours * 3600 # time in seconds
        maxSupply = 1000000000000000000000000000000
        k = (1 << 128) - (((totalSupply * totalSupply) / maxSupply) << 128) / maxSupply
        reward = (timeDelta * rewardPerSlotPerSecond * k) >> 128
        return reward
    */
    // Kick out prior king if any and reward them.
    uint256 lastTime = slots[slot].last_claim_time;
    if (lastTime != 0) {
      require (lastTime < now, "mining same slot in same block not allowed!");

      // Fixed integer math based on the following formula:
      // reward = time * rewardRate * (1 - (totalSupply / maxSupply) ^ 2)
      uint256 timeDelta = now - lastTime;
      uint256 k = (1 << 128) - (((totalSupply * totalSupply) / maxSupply) << 128) / maxSupply;
      uint256 reward = (timeDelta * rewardPerSlotPerSecond * k) >> 128;
      mint(address(treasuryAddress), reward);
      mint(slots[slot].owner, reward);
    } else {
      // Reward first time validators as if they held the slot for 1 hour.
      uint256 value = (3600) * rewardPerSlotPerSecond;
      mint(address(treasuryAddress), value);
      mint(msg.sender, value);
    }

    // Update the slot with data for the new king.
    slots[slot].owner = msg.sender;
    slots[slot].difficulty = key;
    slots[slot].last_claim_time = now;

    numTakeOvers++;
    emit NewSlotKing(slot, msg.sender);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // User Registration
  //////////////////////////////////////////////////////////////////////////////////////////////////

  struct UserInfo {
    string userName;
    string info;
  }

  mapping (string => address) public mapNameToUser;
  mapping (address => UserInfo) public mapUsersInfo;
  address[] public userAddresses;

  function initNames() private {
    registerUserInternal(treasuryAddress, "Treasury", "");
    registerUserInternal(DEXAddress, "DEX", "");
  }

  function getUserName(address addr) public view returns (string memory) {
    return mapUsersInfo[addr].userName;
  }

  function registerUserInternal(address addr, string memory userName, string memory info) private {
    userAddresses.push(addr);
    mapNameToUser[userName] = addr;
    mapUsersInfo[addr].userName = userName;
    mapUsersInfo[addr].info = info;
  }

  function registerUser(string calldata userName, string calldata info) external {
    require(mapNameToUser[userName] == address(0));
    registerUserInternal(msg.sender, userName, info);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // DEX
  //////////////////////////////////////////////////////////////////////////////////////////////////

  uint256 public minOrderETH = 1 ether / 10;
  uint256 public minOrderAUTO = 1000 ether;

  enum OrderType { Buy, Sell, Auction }

  struct Order {
    uint256 AUTO;
    uint256 ETH;
    address payable owner;
    OrderType orderType;
  }

  Order[] public orders;

  function getDEXBalance() public view returns (uint256) {
    return balanceOf(DEXAddress);
  }

  function removeOrder(uint256 _id) private {
    orders[_id] = orders[orders.length - 1];
    orders.length--;
  }

  function getOrdersLength() public view returns (uint256) {
    return orders.length;
  }

  function buy(uint256 _AUTO) public payable returns (uint256 _id) {
    require(msg.value >= minOrderETH, "Minimum ETH requirement not met");
    require(_AUTO >= minOrderAUTO, "Minimum AUTO requirement not met");
    _id = orders.length;
    orders.push(Order(_AUTO, msg.value, msg.sender, OrderType.Buy));
  }

  function sellNow(uint256 _id, uint256 _AUTO, uint256 _ETH) public {
    require(_id < orders.length, "Invalid Order ID");
    Order memory o = orders[_id];
    require(o.AUTO == _AUTO, "Order AUTO does not match requested size");
    require(o.ETH == _ETH, "Order ETH does not match requested size");
    require(o.orderType == OrderType.Buy, "Invalid order type");
    transfer(o.owner, _AUTO);
    msg.sender.transfer(_ETH);
    removeOrder(_id);
  }

  function sell(uint256 _AUTO, uint256 _ETH) public returns (uint256 _id){
    require(_AUTO >= minOrderAUTO, "Minimum AUTO requirement not met");
    require(_ETH >= minOrderETH, "Minimum ETH requirement not met");
    transfer(DEXAddress, _AUTO);
    _id = orders.length;
    orders.push(Order(_AUTO, _ETH, msg.sender, OrderType.Sell));
  }

  function buyNow(uint256 _id, uint256 _AUTO) public payable {
    require(_id < orders.length, "Invalid Order ID");
    Order memory o = orders[_id];
    require(o.AUTO == _AUTO, "Order AUTO does not match requested size");
    require(o.ETH == msg.value, "Order ETH does not match requested size");
    require(o.orderType == OrderType.Sell, "Invalid order type");
    o.owner.transfer(msg.value);
    transferInternal(DEXAddress, msg.sender, _AUTO);
    removeOrder(_id);
  }

  function cancelOrder(uint256 _id) public {
    Order memory o = orders[_id];
    require(o.owner == msg.sender);

    if (o.orderType == OrderType.Buy) {
      msg.sender.transfer(o.ETH);
    }

    if (o.orderType == OrderType.Sell) {
      transferInternal(DEXAddress, msg.sender, o.AUTO);
    }

    removeOrder(_id);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Internal Helpers
  //////////////////////////////////////////////////////////////////////////////////////////////////

  // Returns the Ethereum address corresponding to the input public key.
  function getAddressFromPubKey(bytes32 pubkeyX, bytes32 pubkeyY) private pure returns (uint256) {
    return uint256(keccak256(abi.encodePacked(pubkeyX, pubkeyY))) & 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
  }

  // Verifies that signature of a message matches the given public key.
  function verifySignature(bytes32 pubkeyX, bytes32 pubkeyY, bytes32 hash,
      uint8 v, bytes32 r, bytes32 s) private pure returns (bool) {
    uint256 addr = getAddressFromPubKey(pubkeyX, pubkeyY);
    address addr_r = ecrecover(hash, v, r, s);
    return addr == uint256(addr_r);
  }
}
