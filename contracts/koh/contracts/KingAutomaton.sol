pragma solidity ^0.6.2;

import "./Util.sol";
import "./DEX.sol";
import "./Proposals.sol";
import "./KingOfTheHill.sol";

contract KingAutomaton is KingOfTheHill {

  int256 constant INT256_MIN = int256(uint256(1) << 255);
  int256 constant INT256_MAX = int256(~(uint256(1) << 255));
  uint256 constant UINT256_MIN = 0;
  uint256 constant UINT256_MAX = ~uint256(0);
  uint256 constant MSB_SET = 1 << 255;
  uint256 constant ALL_BUT_MSB = MSB_SET - 1;

  using Proposals for Proposals.Data;
  using DEX for DEX.Data;
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Initialization
  //////////////////////////////////////////////////////////////////////////////////////////////////

  constructor(uint256 nSlots, uint256 minDifficultyBits, uint256 predefinedMask, uint256 initialDailySupply,
      int256 approval_pct, int256 contest_pct, uint256 treasury_limit_pct) public {
    numSlots = nSlots;
    initMining(nSlots, minDifficultyBits, predefinedMask, initialDailySupply);
    initNames();

    require(approval_pct > contest_pct, "Approval percentage must be bigger than contest percentage!");
    proposalsData.approvalPercentage = approval_pct;
    proposalsData.contestPercentage = contest_pct;
    proposalsData.treasuryLimitPercentage = treasury_limit_pct;
    proposalsData.ballotBoxIDs = 99;  // Ensure special addresses are not already used
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

  uint256 public minPeriodLen = 3 days;

  bool public debugging = false;

  Proposals.Data public proposalsData;

  modifier validBallotBoxID(uint256 id) {
    require(proposalsData.ballotBoxes[id].state != Proposals.BallotBoxState.Uninitialized, "Invalid ballot box ID!");
    _;
  }

  modifier slotOwner(uint256 _slot) {
    require(msg.sender == slots[_slot].owner, "Invalid slot owner!");
    _;
  }

  modifier debugOnly() {
    require(debugging, "Available only in debug mode!");
    _;
  }

  function getBallotBox(uint256 _id) public view
  returns (Proposals.BallotBoxState state, uint256 numChoices, uint256 paidSlots) {
    Proposals.BallotBox memory b = proposalsData.ballotBoxes[_id];
    state = b.state;
    numChoices = b.numChoices;
    paidSlots = b.paidSlots;
  }

  function getProposal(uint256 _id)
  public view returns (address contributor, string memory title,
      string memory documentsLink, bytes memory documentsHash,
      uint256 budgetPeriodLen, uint256 remainingPeriods,
      uint256 budgetPerPeriod, uint256 nextPaymentDate,
      Proposals.ProposalState state, uint256 initialEndDate,
      uint256 contestEndDate) {
    Proposals.Proposal memory p = proposalsData.proposals[_id];

    contributor = p.contributor;
    title = p.title;
    documentsLink = p.documentsLink;
    documentsHash = p.documentsHash;
    budgetPeriodLen = p.budgetPeriodLen;
    remainingPeriods = p.remainingPeriods;
    budgetPerPeriod = p.budgetPerPeriod;
    nextPaymentDate = p.nextPaymentDate;
    state = p.state;
    initialEndDate = p.initialEndDate;
    contestEndDate = p.contestEndDate;
  }

  function createBallotBox(uint256 _choices) public returns (uint256) {
    return proposalsData.createBallotBox(_choices, numSlots);
  }

  function createProposal(
    address payable contributor, string calldata title, string calldata documents_link,
    bytes calldata documents_hash, uint256 budget_period_len, uint256 num_periods, uint256 budget_per_period
  ) external returns (uint256 _id) {
    require(budget_period_len <= minPeriodLen);
    require(num_periods * budget_per_period <= proposalsData.treasuryLimitPercentage * balances[treasuryAddress] / 100);
    _id = proposalsData.createProposal(
        numSlots,
        contributor,
        title,
        documents_link,
        documents_hash,
        budget_period_len,
        num_periods,
        budget_per_period
    );
    transferInternal(treasuryAddress, address(_id), num_periods * budget_per_period);
  }

  function unpaidSlots(uint256 _id) public view returns (uint256) {
    Proposals.BallotBox memory b = proposalsData.ballotBoxes[_id];
    require(b.state != Proposals.BallotBoxState.Uninitialized, "Invalid ballot box ID!");
    return numSlots - b.paidSlots;
  }

  // Pay for multiple slots at once, 32 seems to be a reasonable amount.
  function payForGas(uint256 _id, uint256 _slotsToPay) public {
    proposalsData.payForGas(_id, _slotsToPay);
  }

  function calcVoteDifference(uint256 _id) view public returns (int256) {
    return proposalsData.calcVoteDifference(_id);
  }

  function castVote(uint256 _id, uint256 _slot, uint8 _choice) public slotOwner(_slot) {
    updateProposalState(_id);
    proposalsData.castVote(_id, _slot, _choice);
  }

  function getVote(uint256 _id, uint256 _slot) public view returns (uint) {
    Proposals.BallotBox storage b = proposalsData.ballotBoxes[_id];
    require(b.state != Proposals.BallotBoxState.Uninitialized, "Invalid ballot box ID!");
    uint256 bitsPerVote = Util.msb(b.numChoices) + 1;
    uint256 votesPerWord = 255 / bitsPerVote;
    uint256 index = _slot / votesPerWord;
    uint256 offset = (_slot % votesPerWord) * bitsPerVote;
    uint256 mask = ((1 << bitsPerVote) - 1) << offset;
    return (b.votes[index] & mask) >> offset;
  }

  function getVoteCount(uint256 _id, uint256 _choice) public view validBallotBoxID(_id) returns(uint256) {
    return proposalsData.ballotBoxes[_id].voteCount[_choice] & ALL_BUT_MSB;
  }

  function completeProposal(uint256 _id) private {
    updateProposalState(_id);
    proposalsData.completeProposal(_id);
  }

  function updateProposalState(uint256 _id) public validBallotBoxID(_id) {
    bool _return_to_treasury = proposalsData.updateProposalState(_id);
    if (_return_to_treasury) {
      transferInternal(address(_id), treasuryAddress, balances[address(_id)]);
    }
  }

  // In case the contributor is inactive anyone could call the function
  // AFTER all periods are passed and the funds locked
  // in the proposal address will be returned to treasury.
  // Rejecting the proposal will have the same effect.
  function claimReward(uint256 _id, uint256 _budget) public validBallotBoxID(_id) {
    updateProposalState(_id);
    (bool _is_transfer_allowed, uint256 _return_to_treasury) = proposalsData.claimReward(_id, _budget);
    if (_is_transfer_allowed) {
      transferInternal(address(_id), proposalsData.proposals[_id].contributor, _budget);
    }
    if (_return_to_treasury > 0) {
      transferInternal(address(_id), treasuryAddress, _return_to_treasury);
    }
  }

  // Test functions, to be deleted

  function setOwner(uint256 _slot, address new_owner) public debugOnly {
    slots[_slot].owner = new_owner;
  }

  function setOwnerAllSlots() public debugOnly {
    for (uint256 i = 0; i < numSlots; ++i) {
      slots[i].owner = msg.sender;
    }
  }

  function castVotesForApproval(uint256 _id) public debugOnly returns(uint256){
    uint256 minNumYesVotes =
        uint256((int256(numSlots) * (proposalsData.approvalPercentage + 100) + 199) / 200);
    for (uint256 i = 0; i < minNumYesVotes; ++i) {
      castVote(_id, i, 1);
    }
    return minNumYesVotes;
  }

  function castVotesForRejection(uint256 _id) public debugOnly returns(uint256){
    uint256 minNumNoVotes =
        uint256((int256(numSlots) * (100 - proposalsData.contestPercentage) + 199) / 200);
    for (uint256 i = 0; i < minNumNoVotes; ++i) {
      castVote(_id, i, 2);
    }
    return minNumNoVotes;
  }

  function getVoteWord(uint256 _id, uint256 _idx) public view returns(uint256) {
    return proposalsData.ballotBoxes[_id].votes[_idx] & ALL_BUT_MSB;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Mining
  //////////////////////////////////////////////////////////////////////////////////////////////////
  uint256 private rewardPerSlotPerSecond;

  function initMining(uint256 nSlots,
                      uint256 minDifficultyBits,
                      uint256 predefinedMask,
                      uint256 initialDailySupply) public {
    require(nSlots > 0);
    setMinDifficulty(minDifficultyBits);
    if (predefinedMask == 0) {
      // Prevents premining with a known predefined mask.
      setMask(uint256(keccak256(abi.encodePacked(now, msg.sender))));
    } else {
      // Setup predefined mask, useful for testing purposes.
      setMask(predefinedMask);
    }
    rewardPerSlotPerSecond = (1 ether * initialDailySupply) / 1 days / nSlots;
  }

  function slotAcquired(uint256 id) internal override {
    require(totalSupply < maxSupply, "Cap reached");
    ValidatorSlot memory slot = slots[id];
    if (slot.last_claim_time != 0) {
      uint256 k = (1 << 128) - (((totalSupply * totalSupply) / maxSupply) << 128) / maxSupply;
      uint256 _value = ((now - slot.last_claim_time) * rewardPerSlotPerSecond * k) >> 128;
      mint(slot.owner, _value);
    } else {
      // Reward first time validators as if they held the slot for 1 hour.
      mint(msg.sender, (3600) * rewardPerSlotPerSecond);
    }
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

  DEX.Data public dexData;

  uint256 public minOrderETH = 1 ether / 10;
  uint256 public minOrderAUTO = 1000 ether;

  function withdraw(uint256 _value) external {
    require(_value <= dexData.balanceETH[msg.sender]);
    dexData.balanceETH[msg.sender] -= _value;
    (bool success, ) = msg.sender.call.value(_value)("");
    require(success);
  }

  function getBalanceETH(address _user) public view returns (uint256) {
    return dexData.balanceETH[_user];
  }

  function getOrdersLength() public view returns (uint256) {
    return dexData.ids;
  }

  function getOrder(uint256 _id) public view
  returns (uint256 AUTO, uint256 ETH, address owner, DEX.OrderType orderType) {
    DEX.Order memory o = dexData.orders[_id];
    AUTO = o.AUTO;
    ETH = o.ETH;
    owner = o.owner;
    orderType = o.orderType;
  }

  function buy(uint256 _AUTO) public payable returns (uint256) {
    require(msg.value >= minOrderETH, "Minimum ETH requirement not met");
    require(_AUTO >= minOrderAUTO, "Minimum AUTO requirement not met");
    return dexData.addOrder(_AUTO, msg.value, msg.sender, DEX.OrderType.Buy);
  }

  function sellNow(uint256 _id, uint256 _AUTO, uint256 _ETH) public {
    address owner = dexData.orders[_id].owner;
    dexData.sellNow(_id, _AUTO, _ETH);
    transfer(owner, _AUTO);
  }

  function sell(uint256 _AUTO, uint256 _ETH) public returns (uint256 _id){
    require(_AUTO >= minOrderAUTO, "Minimum AUTO requirement not met");
    require(_ETH >= minOrderETH, "Minimum ETH requirement not met");
    transfer(DEXAddress, _AUTO);
    return dexData.addOrder(_AUTO, _ETH, msg.sender, DEX.OrderType.Sell);
  }

  function buyNow(uint256 _id, uint256 _AUTO) public payable {
    dexData.buyNow(_id, _AUTO, msg.value);
    transferInternal(DEXAddress, msg.sender, _AUTO);
  }

  function cancelOrder(uint256 _id) public {
    DEX.Order memory o = dexData.orders[_id];
    require(o.owner == msg.sender);
    dexData.removeOrder(_id);

    if (o.orderType == DEX.OrderType.Buy) {
      uint256 balance = dexData.balanceETH[msg.sender];
      require(balance + o.ETH > balance);
      dexData.balanceETH[msg.sender] += o.ETH;
    } else if (o.orderType == DEX.OrderType.Sell) {
      transferInternal(DEXAddress, msg.sender, o.AUTO);
    }
  }
}
