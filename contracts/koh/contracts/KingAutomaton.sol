pragma solidity ^0.6.2;

library Helpers {
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

  // Returns the Ethereum address corresponding to the input public key.
  function getAddressFromPubKey(bytes32 pubkeyX, bytes32 pubkeyY) public pure returns (uint256) {
    return uint256(keccak256(abi.encodePacked(pubkeyX, pubkeyY))) & 0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
  }

  // Verifies that signature of a message matches the given public key.
  function verifySignature(bytes32 pubkeyX, bytes32 pubkeyY, bytes32 hash, uint8 v, bytes32 r, bytes32 s) public pure returns (bool) {
    uint256 addr = getAddressFromPubKey(pubkeyX, pubkeyY);
    address addr_r = ecrecover(hash, v, r, s);
    return addr == uint256(addr_r);
  }
}

library DEX {
  enum OrderType {None, Buy, Sell, Auction}

  struct Order {
    uint256 AUTO;
    uint256 ETH;
    address payable owner;
    OrderType orderType;
  }

  struct Data {
    mapping(uint256 => Order) orders;
    mapping(address => uint256) withdraws;
    uint256 ids;
  }

  function removeOrder(Data storage self, uint256 _id) public {
    Order memory o;
    o.AUTO = 0;
    o.ETH = 0;
    o.owner = address(0);
    o.orderType = OrderType.None;

    self.orders[_id] = o;
  }

  function addOrder(Data storage self, uint256 _AUTO, uint256 _value, address payable _sender, OrderType _orderType) public returns (uint256) {
    uint256 id = ++self.ids;
    self.orders[id] = Order(_AUTO, _value, _sender, _orderType);
    return id;
  }

  function sellNow(Data storage self, uint256 _id, uint256 _AUTO, uint256 _ETH) public {
    Order memory o = self.orders[_id];
    require(o.owner != address(0), "Invalid Order ID");
    require(o.AUTO == _AUTO, "Order AUTO does not match requested size");
    require(o.ETH == _ETH, "Order ETH does not match requested size");
    require(o.orderType == OrderType.Buy, "Invalid order type");
    uint256 withdraw = self.withdraws[msg.sender];
    require(withdraw + _ETH > withdraw);
    self.withdraws[msg.sender] += _ETH;
  }

  function buyNow(Data storage self, uint256 _id, uint256 _AUTO, uint256 _value) public {
    Order memory o = self.orders[_id];
    require(o.owner != address(0), "Invalid Order ID");
    require(o.AUTO == _AUTO, "Order AUTO does not match requested size");
    require(o.ETH == _value, "Order ETH does not match requested size");
    require(o.orderType == OrderType.Sell, "Invalid order type");
    uint256 withdraw = self.withdraws[o.owner];
    require(withdraw + _value > withdraw);
    self.withdraws[o.owner] += _value;
  }
}

library Miner {
  struct ValidatorSlot {
    address owner;
    uint256 difficulty;
    uint256 last_claim_time;
  }

  struct Data {
    mapping (uint256 => ValidatorSlot) slots;

    uint256 minDifficulty;          // Minimum difficulty
    uint256 mask;                   // Prevents premine
    uint256 numTakeOvers;           // Number of times a slot was taken over by a new king.
    uint256 rewardPerSlotPerSecond; // Validator reward per slot per second.
  }

  function initMining(Data storage self, uint256 nSlots, uint256 minDifficultyBits, uint256 predefinedMask, uint256 initialDailySupply) public {
    require(nSlots > 0);
    require(minDifficultyBits > 0);

    self.minDifficulty = (2 ** minDifficultyBits - 1) << (256 - minDifficultyBits);
    if (predefinedMask == 0) {
      // Prevents premining with a known predefined mask.
      self.mask = uint256(keccak256(abi.encodePacked(now, msg.sender)));
    } else {
      // Setup predefined mask, useful for testing purposes.
      self.mask = predefinedMask;
    }

    self.rewardPerSlotPerSecond = (1 ether * initialDailySupply) / 1 days / nSlots;
  }

  function claimSlot(Data storage self, uint256 slot, uint256 coeff_k, uint256 key) public returns
      (address receiver, uint256 reward) {
    // Check if the key can take over the slot and become the new king.
    require(key > self.minDifficulty && key > self.slots[slot].difficulty, "Low key difficulty");
    // Kick out prior king if any and reward them.
    uint256 lastTime = self.slots[slot].last_claim_time;
    if (lastTime != 0) {
      require (lastTime < now, "mining same slot in same block not allowed!");

      reward = ((now - lastTime) * self.rewardPerSlotPerSecond * coeff_k) >> 128;
      receiver = self.slots[slot].owner;
    } else {
      // Reward first time validators as if they held the slot for 1 hour.
      reward = (3600) * self.rewardPerSlotPerSecond;
      receiver = msg.sender;
    }
    // Update the slot with data for the new king.
    self.slots[slot].owner = msg.sender;
    self.slots[slot].difficulty = key;
    self.slots[slot].last_claim_time = now;
    self.numTakeOvers++;
  }
}

library Proposals {
  enum BallotBoxState {Uninitialized, PrepayingGas, Active, Inactive}
  enum ProposalState {Uninitialized, Started, Accepted, Rejected, Contested, Completed}

  uint256 constant MSB_SET = 1 << 255;
  uint256 constant UINT256_MAX = ~uint256(0);
  /*
  ##################################
  # BallotBoxState # ProposalState #
  ##################################
  # Uninitialized  # Uninitialized #
  # -------------- # ------------- #
  # Any state      # Uninitialized # -> there is no proposal connected to this ballot
  # -------------- # ------------- #
  # PrepayingGas   # Started       #
  # -------------- # ------------- #
  # Active         # Started       # -> first period of voting until initialEndDate
  #                # ------------- #
  #                # Accepted      # -> enough "yes" votes during the initial voting period
  #                # ------------- #
  #                # Contested     # -> too many "no" votes after the first approval ot the proposal
  # -------------- # ------------- #
  # Inactive       # Rejected      # -> not enough "yes" votes during the initial period or during a contest period
  #                # ------------- #
  #                # Completed     # -> the proposal was successfully implemented
  ##################################
  */

  struct BallotBox {
    BallotBoxState state;
    uint256 numChoices;
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

    uint256 budgetPeriodLen;
    uint256 remainingPeriods;
    uint256 budgetPerPeriod;
    uint256 nextPaymentDate;

    ProposalState state;

    uint256 initialEndDate;
    uint256 contestEndDate;
  }

  struct Data {
    int256 approvalPercentage;
    int256 contestPercentage;
    uint256 treasuryLimitPercentage;

    mapping (uint256 => BallotBox) ballotBoxes;
    mapping (uint256 => Proposal) proposals;
  }

  modifier validBallotBoxID(Data storage self, uint256 id) {
    require(self.ballotBoxes[id].state != BallotBoxState.Uninitialized, "Invalid ballot box ID!");
    _;
  }

  function createBallotBox(Data storage self, uint256 _choices, uint256 _id) public {
    require (_choices > 1, "Number of choices must be bigger than 1!");
    BallotBox storage b = self.ballotBoxes[_id];
    b.numChoices = _choices;
    b.state = BallotBoxState.PrepayingGas;
    for (uint256 i = 0; i <= _choices; i++) {
      b.voteCount[i] = MSB_SET;
    }
  }

  function createProposal(Data storage self, uint256 id, address payable contributor, string memory title,
      string memory documents_link, bytes memory documents_hash, uint256 budget_period_len, uint256 num_periods, uint256 budget_per_period) public {
    Proposal storage p = self.proposals[id];
    p.contributor = contributor;
    p.title = title;
    p.documentsLink = documents_link;
    p.documentsHash = documents_hash;
    p.state = ProposalState.Started;

    p.budgetPeriodLen = budget_period_len;
    p.remainingPeriods = num_periods;
    p.budgetPerPeriod = budget_per_period;
  }

  // Pay for multiple slots at once, 32 seems to be a reasonable amount.
  function payForGas(Data storage self, uint256 _id, uint256 _slotsToPay, uint256 _slotsLength) public validBallotBoxID(self, _id) {
    BallotBox storage b = self.ballotBoxes[_id];
    uint256 _paidSlots = b.paidSlots;
    require((_slotsLength - _paidSlots) >= _slotsToPay, "Too many slots!");
    uint256 _newLength = _paidSlots + _slotsToPay;
    uint256 votesPerWord = 255 / (Helpers.msb(b.numChoices) + 1);
    uint256 votesWords = (_slotsLength + votesPerWord - 1) / votesPerWord;
    for (uint256 i = 1; i <= _slotsToPay; i++) {
      uint256 idx = _newLength - i;
      b.payGas1[idx] = b.payGas2[idx] = MSB_SET;
      if (_newLength - i <= votesWords) {
        b.votes[idx] = MSB_SET;
      }
    }
    b.paidSlots = _newLength;
    if (_newLength == _slotsLength) {
      b.state = BallotBoxState.Active;
    }
  }

  function calcVoteDifference(Data storage self, uint256 _id, uint256 _numSlots) view public validBallotBoxID(self, _id) returns (int256) {
    BallotBox storage b = self.ballotBoxes[_id];
    int256 yes = int256(b.voteCount[1]);
    int256 no = int256(b.voteCount[2]);
    return (yes - no) * 100 / int256(_numSlots);
  }

  function castVote(Data storage self, uint256 _id, uint256 _slot, uint8 _choice) public validBallotBoxID(self, _id) {
    BallotBox storage box = self.ballotBoxes[_id];
    uint256 numChoices = box.numChoices;
    require(_choice <= numChoices, "Invalid choice");
    require(box.state == BallotBoxState.Active, "Ballot is not active!");

    uint256 bitsPerVote = Helpers.msb(numChoices) + 1;
    uint256 votesPerWord = 255 / bitsPerVote;

    // Calculate masks.
    uint256 index = _slot / votesPerWord;
    uint256 offset = (_slot % votesPerWord) * bitsPerVote;
    uint256 mask = ((1 << bitsPerVote) - 1) << offset;

    // Reduce the vote count.
    uint256 vote = box.votes[index];
    uint256 oldChoice = (vote & mask) >> offset;
    if (oldChoice > 0) {
      box.voteCount[oldChoice]--;
    }

    // Modify vote selection.
    vote &= (mask ^ UINT256_MAX);        // get rid of current choice using a mask.
    vote |= uint256(_choice) << offset;  // replace current choice using a mask.
    box.votes[index] = vote;               // actually update the storage slot.
    box.voteCount[_choice]++;              // update the total vote count based on the choice.

    // Incentivize voters by giving them a refund.
    box.payGas1[_slot] = 0;
    box.payGas2[_slot] = 0;
  }

  function getVote(Data storage self, uint256 _id, uint256 _slot) public view validBallotBoxID(self, _id) returns (uint256) {
    uint256 numChoices =  self.ballotBoxes[_id].numChoices;
    uint256 bitsPerVote = Helpers.msb(numChoices) + 1;
    uint256 votesPerWord = 255 / bitsPerVote;

    // Calculate masks.
    uint256 index = _slot / votesPerWord;
    uint256 offset = (_slot % votesPerWord) * bitsPerVote;
    uint256 mask = ((1 << bitsPerVote) - 1) << offset;

    // Get vote
    return (self.ballotBoxes[_id].votes[index] & mask) >> offset;
  }

  function completeProposal(Data storage self, uint256 _id) public {
    require(self.ballotBoxes[_id].state == BallotBoxState.Active, "Ballot is not active!");
    Proposal storage p = self.proposals[_id];
    ProposalState p_state = p.state;
    require(p_state == ProposalState.Started || p_state == ProposalState.Contested, "Invalid proposal state!");
    p.state = ProposalState.Completed;
    self.ballotBoxes[_id].state = BallotBoxState.Inactive;
  }
}

contract KingAutomaton {

  int256 constant INT256_MIN = int256(uint256(1) << 255);
  int256 constant INT256_MAX = int256(~(uint256(1) << 255));
  uint256 constant UINT256_MIN = 0;
  uint256 constant UINT256_MAX = ~uint256(0);
  uint256 constant MSB_SET = 1 << 255;
  uint256 constant ALL_BUT_MSB = MSB_SET - 1;

  using Miner for Miner.Data;
  using Proposals for Proposals.Data;
  using DEX for DEX.Data;
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Initialization
  //////////////////////////////////////////////////////////////////////////////////////////////////

  constructor(uint256 nSlots, uint256 minDifficultyBits, uint256 predefinedMask, uint256 initialDailySupply,
      int256 approval_pct, int256 contest_pct, uint256 treasury_limit_pct) public {
    numSlots = nSlots;
    miner_data.initMining(nSlots, minDifficultyBits, predefinedMask, initialDailySupply);
    initNames();

    require(approval_pct > contest_pct, "Approval percentage must be bigger than contest percentage!");
    proposals_data.approvalPercentage = approval_pct;
    proposals_data.contestPercentage = contest_pct;
    proposals_data.treasuryLimitPercentage = treasury_limit_pct;
    // Check if we're on a testnet (We will not using predefined mask when going live)
    if (predefinedMask != 0) {
      // If so, fund the owner for debugging purposes.
      debugging = true;
      mint(msg.sender, 1000000 ether);
      balances[treasuryAddress] = 1000000 ether;
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

  mapping (address => uint256) public balances;
  mapping (address => mapping (address => uint256)) public allowed;

  function transfer(address _to, uint256 _value) public returns (bool success) {
    require(balances[msg.sender] >= _value && balances[_to] + _value >= balances[_to]);
    balances[msg.sender] -= _value;
    balances[_to] += _value;
    emit Transfer(msg.sender, _to, _value); //solhint-disable-line indent, no-unused-vars
    return true;
  }

  function transferFrom(address _from, address _to, uint256 _value) public returns (bool success) {
    uint256 allowance = allowed[_from][msg.sender];
    require(balances[_from] >= _value && allowance >= _value && balances[_to] + _value >= balances[_to]);
    balances[_to] += _value;
    balances[_from] -= _value;
    if (allowance < UINT256_MAX) {
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
    require(balances[_to] + _value > balances[_to]);
    balances[_to] += _value;
    balances[_from] -= _value;
    emit Transfer(_from, _to, _value); //solhint-disable-line indent, no-unused-vars
    return true;
  }

  function mint(address _receiver, uint256 _value) private {
    require(balances[_receiver] + _value > balances[_receiver] && totalSupply + _value > totalSupply);
    balances[_receiver] += _value;
    totalSupply += _value;
    emit Transfer(address(0), _receiver, _value); //solhint-disable-line indent, no-unused-vars
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Treasury
  //////////////////////////////////////////////////////////////////////////////////////////////////

  uint256 public constant PROPOSAL_START_PERIOD = 90 seconds; // 1 weeks;
  uint256 public constant CONTEST_PERIOD = 90 seconds;  //

  bool public debugging = false;

  uint256 private ballotBoxIDs = 99;  // Ensure special addresses are not already used

  Proposals.Data public proposals_data;

  modifier validBallotBoxID(uint256 id) {
    require(proposals_data.ballotBoxes[id].state != Proposals.BallotBoxState.Uninitialized, "Invalid ballot box ID!");
    _;
  }

  modifier slotOwner(uint256 _slot) {
    require(msg.sender == miner_data.slots[_slot].owner, "Invalid slot owner!");
    _;
  }

  modifier debugOnly() {
    require(debugging, "Available only in debug mode!");
    _;
  }

  function createBallotBox(uint256 _choices) public returns (uint256 id) {
    id = ++ballotBoxIDs;
    proposals_data.createBallotBox(_choices, id);
    payForGas(id, 1);
  }

  function createProposal(address payable contributor, string calldata title, string calldata documents_link,
      bytes calldata documents_hash, uint256 budget_period_len, uint256 num_periods, uint256 budget_per_period)
      external returns (uint256 _id) {
    require(num_periods * budget_per_period <= proposals_data.treasuryLimitPercentage * balances[treasuryAddress] / 100);
    _id = createBallotBox(2);
    proposals_data.createProposal(_id, contributor, title, documents_link, documents_hash, budget_period_len, num_periods, budget_per_period);
    transferInternal(treasuryAddress, address(_id), num_periods * budget_per_period);
  }

  function unpaidSlots(uint256 _id) public view returns (uint256) {
    Proposals.BallotBox memory b = proposals_data.ballotBoxes[_id];
    require(b.state != Proposals.BallotBoxState.Uninitialized, "Invalid ballot box ID!");
    return numSlots - b.paidSlots;
  }

  // Pay for multiple slots at once, 32 seems to be a reasonable amount.
  function payForGas(uint256 _id, uint256 _slotsToPay) public {
    proposals_data.payForGas(_id, _slotsToPay, numSlots);
  }

  function calcVoteDifference(uint256 _id) view public returns (int256) {
    return proposals_data.calcVoteDifference(_id, numSlots);
  }

  function castVote(uint256 _id, uint256 _slot, uint8 _choice) public slotOwner(_slot) {
    updateProposalState(_id);
    proposals_data.castVote(_id, _slot, _choice);
  }

  function getVote(uint256 _id, uint256 _slot) public view returns (uint) {
    Proposals.BallotBox storage b = proposals_data.ballotBoxes[_id];
    require(b.state != Proposals.BallotBoxState.Uninitialized, "Invalid ballot box ID!");
    uint256 bitsPerVote = Helpers.msb(b.numChoices) + 1;
    uint256 votesPerWord = 255 / bitsPerVote;
    uint256 index = _slot / votesPerWord;
    uint256 offset = (_slot % votesPerWord) * bitsPerVote;
    uint256 mask = ((1 << bitsPerVote) - 1) << offset;
    return (b.votes[index] & mask) >> offset;
  }

  function getVoteCount(uint256 _id, uint256 _choice) public view validBallotBoxID(_id) returns(uint256) {
    return proposals_data.ballotBoxes[_id].voteCount[_choice] & ALL_BUT_MSB;
  }

  function completeProposal(uint256 _id) private {
    updateProposalState(_id);
    proposals_data.completeProposal(_id);
  }

  function updateProposalState(uint256 _id) public validBallotBoxID(_id) {
    Proposals.Proposal storage p = proposals_data.proposals[_id];
    Proposals.ProposalState p_state = p.state;
    uint256 _initialEndDate = p.initialEndDate;
    if (p_state == Proposals.ProposalState.Started) {
      Proposals.BallotBox storage b = proposals_data.ballotBoxes[_id];
      if (b.state == Proposals.BallotBoxState.Active) {  // Gas is paid
        if (_initialEndDate != 0) {
          if (now >= _initialEndDate) {
            int256 vote_diff = calcVoteDifference(_id);
            if (vote_diff >= proposals_data.approvalPercentage) {
              p.state = Proposals.ProposalState.Accepted;
              p.nextPaymentDate = now;
            } else {
              p.state = Proposals.ProposalState.Rejected;
              b.state = Proposals.BallotBoxState.Inactive;
              transferInternal(address(_id), treasuryAddress, balances[address(_id)]);
            }
          }
        } else {  // Either gas has been just paid and time hasn't been set or the initial time hasn't passed
          p.initialEndDate = now + PROPOSAL_START_PERIOD;
        }
      }
    } else if (p_state == Proposals.ProposalState.Accepted) {
      int256 vote_diff = calcVoteDifference(_id);
      if (vote_diff <= proposals_data.contestPercentage) {
        p.state = Proposals.ProposalState.Contested;
        p.contestEndDate = now + CONTEST_PERIOD;
      }
    } else if (p_state == Proposals.ProposalState.Contested) {
      if (now >= p.contestEndDate) {
        int256 vote_diff = calcVoteDifference(_id);
        if (vote_diff >= proposals_data.approvalPercentage) {
          p.state = Proposals.ProposalState.Accepted;
        } else {
          p.state = Proposals.ProposalState.Rejected;
          proposals_data.ballotBoxes[_id].state = Proposals.BallotBoxState.Inactive;
          transferInternal(address(_id), treasuryAddress, balances[address(_id)]);
        }
      }
    }
  }

  // In case the contributor is inactive anyone could call the function AFTER all periods are passed and the funds locked
  // in the proposal address will be returned to treasury. Rejecting the proposal will have the same effect.
function claimReward(uint256 _id, uint256 _budget) public validBallotBoxID(_id) {
  updateProposalState(_id);
  Proposals.Proposal storage p = proposals_data.proposals[_id];
  require(p.state == Proposals.ProposalState.Accepted || p.state == Proposals.ProposalState.Contested, "Incorrect proposal state!");
  uint256 paymentDate = p.nextPaymentDate;
  uint256 remainingPeriods = p.remainingPeriods;
  uint256 periodLen = p.budgetPeriodLen;
  uint256 budgetPerPeriod = p.budgetPerPeriod;
  address proposalAddress = address(_id);

  require(_budget <= budgetPerPeriod, "Budget exceeded!");

  if (paymentDate > 0) {
    if (paymentDate <= now) {
      if (paymentDate + periodLen < now) {
        uint256 missedPeriods = (now - paymentDate) / periodLen;
        transferInternal(proposalAddress, treasuryAddress, missedPeriods * p.budgetPerPeriod);
        remainingPeriods -= missedPeriods;
        paymentDate += periodLen * missedPeriods;
      }
      if (remainingPeriods > 1) {
        require(p.contributor == msg.sender, "Invalid contributor!");
        transferInternal(proposalAddress, p.contributor, _budget);
        if (budgetPerPeriod > _budget) {
          transferInternal(proposalAddress, treasuryAddress, budgetPerPeriod - _budget);
        }
        paymentDate += periodLen;
        remainingPeriods--;
      } else {
        if (remainingPeriods == 1) {
          require(p.contributor == msg.sender, "Invalid contributor!");
          transferInternal(proposalAddress, p.contributor, _budget);
          if (budgetPerPeriod > _budget) {
            transferInternal(proposalAddress, treasuryAddress, budgetPerPeriod - _budget);
          }
        }
        p.nextPaymentDate = 0;
        remainingPeriods = 0;
        p.state = Proposals.ProposalState.Completed;
        proposals_data.ballotBoxes[_id].state = Proposals.BallotBoxState.Inactive;
      }
      p.remainingPeriods = remainingPeriods;
      p.nextPaymentDate = paymentDate;
    }
  }
}

  // Test functions, to be deleted

  function setOwner(uint256 _slot, address new_owner) public debugOnly {
    miner_data.slots[_slot].owner = new_owner;
  }

  function setOwnerAllSlots() public debugOnly {
    for (uint256 i = 0; i < numSlots; ++i) {
      miner_data.slots[i].owner = msg.sender;
    }
  }

  function castVotesForApproval(uint256 _id) public debugOnly returns(uint256){
    uint256 minNumYesVotes = uint256((int256(numSlots) * (proposals_data.approvalPercentage + 100) + 199) / 200);
    for (uint256 i = 0; i < minNumYesVotes; ++i) {
      castVote(_id, i, 1);
    }
    return minNumYesVotes;
  }

  function castVotesForRejection(uint256 _id) public debugOnly returns(uint256){
    uint256 minNumNoVotes = uint256((int256(numSlots) * (100 - proposals_data.contestPercentage) + 199) / 200);
    for (uint256 i = 0; i < minNumNoVotes; ++i) {
      castVote(_id, i, 2);
    }
    return minNumNoVotes;
  }

  function getVoteWord(uint256 _id, uint256 _idx) public view returns(uint256) {
    return proposals_data.ballotBoxes[_id].votes[_idx] & ALL_BUT_MSB;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Mining
  //////////////////////////////////////////////////////////////////////////////////////////////////

  event NewSlotKing(uint256 slot, address newOwner);

  Miner.Data public miner_data;
  uint256 numSlots;

  function getSlotOwner(uint256 slot) public view returns(address) {
    return miner_data.slots[slot].owner;
  }

  function getSlotDifficulty(uint256 slot) public view returns(uint256) {
    return miner_data.slots[slot].difficulty;
  }

  function getSlotLastClaimTime(uint256 slot) public view returns(uint256) {
    return miner_data.slots[slot].last_claim_time;
  }

  function getOwners(uint256 start, uint256 len) public view returns(address[] memory result) {
    result = new address[](len);
    for(uint256 i = 0; i < len; i++) {
      result[i] = miner_data.slots[start + i].owner;
    }
  }

  function getDifficulties(uint256 start, uint256 len) public view returns(uint256[] memory result) {
    result = new uint256[](len);
    for(uint256 i = 0; i < len; i++) {
      result[i] = miner_data.slots[start + i].difficulty;
    }
  }

  function getLastClaimTimes(uint256 start, uint256 len) public view returns(uint256[] memory result) {
    result = new uint256[](len);
    for(uint256 i = 0; i < len; i++) {
      result[i] = miner_data.slots[start + i].last_claim_time;
    }
  }

  function getMask() public view returns(uint256) {
    return miner_data.mask;
  }

  function getMinDifficulty() public view returns(uint256) {
    return miner_data.minDifficulty;
  }

  function getClaimed() public view returns(uint256) {
    return miner_data.numTakeOvers;
  }

  /** Claims slot based on a signature.
    * @param pubKeyX X coordinate of the public key used to claim the slot
    * @param pubKeyY Y coordinate of the public key used to claim the slot
    * @param v recId of the signature needed for ecrecover
    * @param r R portion of the signature
    * @param s S portion of the signature
    */
  function claimSlot(bytes32 pubKeyX, bytes32 pubKeyY, uint8 v, bytes32 r, bytes32 s) public {
    require(Helpers.verifySignature(pubKeyX, pubKeyY, bytes32(uint256(msg.sender)), v, r, s), "Signature not valid");
    require(totalSupply < maxSupply, "Cap reached");
    uint256 slot = uint256(pubKeyX) % numSlots;
    uint256 key = uint256(pubKeyX) ^ miner_data.mask;
    // Fixed integer math based on the following formula:
    // reward = time * rewardRate * (1 - (totalSupply / maxSupply) ^ 2)
    uint256 k = (1 << 128) - (((totalSupply * totalSupply) / maxSupply) << 128) / maxSupply;
    (address _receiver, uint256 _value) = miner_data.claimSlot(slot, k, key);
    mint(_receiver, _value);
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

  function registerUserInternal(address addr, string memory userName, string memory info) public {
    userAddresses.push(addr);
    mapNameToUser[userName] = addr;
    mapUsersInfo[addr].userName = userName;
    mapUsersInfo[addr].info = info;
  }

  function initNames() private {
    registerUserInternal(treasuryAddress, "Treasury", "");
    registerUserInternal(DEXAddress, "DEX", "");
  }

  function getUserName(address addr) public view returns (string memory) {
    return mapUsersInfo[addr].userName;
  }

  function registerUser(string calldata userName, string calldata info) external {
    require(mapNameToUser[userName] == address(0));
    registerUserInternal(msg.sender, userName, info);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // DEX
  //////////////////////////////////////////////////////////////////////////////////////////////////

  DEX.Data public dex_data;

  uint256 public constant minOrderETH = 1 ether / 10;
  uint256 public constant minOrderAUTO = 1000 ether;

  function withdraw(uint256 _value) external {
    require(_value <= dex_data.withdraws[msg.sender]);
    dex_data.withdraws[msg.sender] -= _value;
    (bool success, ) = msg.sender.call.value(_value)("");
    require(success);
  }

  function getOrdersLength() public view returns (uint256) {
    return dex_data.ids;
  }

  function buy(uint256 _AUTO) public payable returns (uint256) {
    require(msg.value >= minOrderETH, "Minimum ETH requirement not met");
    require(_AUTO >= minOrderAUTO, "Minimum AUTO requirement not met");
    return dex_data.addOrder(_AUTO, msg.value, msg.sender, DEX.OrderType.Buy);
  }

  function sellNow(uint256 _id, uint256 _AUTO, uint256 _ETH) public {
    dex_data.sellNow(_id, _AUTO, _ETH);
    dex_data.removeOrder(_id);
    transfer(dex_data.orders[_id].owner, _AUTO);
  }

  function sell(uint256 _AUTO, uint256 _ETH) public returns (uint256 _id){
    require(_AUTO >= minOrderAUTO, "Minimum AUTO requirement not met");
    require(_ETH >= minOrderETH, "Minimum ETH requirement not met");
    transfer(DEXAddress, _AUTO);
    return dex_data.addOrder(_AUTO, _ETH, msg.sender, DEX.OrderType.Sell);
  }

  function buyNow(uint256 _id, uint256 _AUTO) public payable {
    dex_data.buyNow(_id, _AUTO, msg.value);
    dex_data.removeOrder(_id);
    transferInternal(DEXAddress, msg.sender, _AUTO);
  }

  function cancelOrder(uint256 _id) public {
    DEX.Order memory o = dex_data.orders[_id];
    require(o.owner == msg.sender);
    dex_data.removeOrder(_id);

    if (o.orderType == DEX.OrderType.Buy) {
      require(dex_data.withdraws[msg.sender] + o.ETH > dex_data.withdraws[msg.sender]);
      dex_data.withdraws[msg.sender] += o.ETH;
    } else if (o.orderType == DEX.OrderType.Sell) {
      transferInternal(DEXAddress, msg.sender, o.AUTO);
    }
  }
}
