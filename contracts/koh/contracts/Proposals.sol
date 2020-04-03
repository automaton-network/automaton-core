pragma solidity ^0.6.2;

import "./Util.sol";

library Proposals {
  enum BallotBoxState {Uninitialized, PrepayingGas, Active, Inactive}
  enum ProposalState {Uninitialized, Started, Accepted, Rejected, Contested, Completed}

  uint256 public constant PROPOSAL_START_PERIOD = 90 seconds; // 1 weeks;
  uint256 public constant CONTEST_PERIOD = 90 seconds;  //

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
    uint256 numSlots;
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
    uint256 ballotBoxIDs;

    mapping (uint256 => BallotBox) ballotBoxes;
    mapping (uint256 => Proposal) proposals;
  }

  modifier validBallotBoxID(Data storage self, uint256 id) {
    require(self.ballotBoxes[id].state != BallotBoxState.Uninitialized, "Invalid ballot box ID!");
    _;
  }

  function createBallotBox(Data storage self, uint256 _choices, uint256 _numSlots) public returns (uint256 id) {
    require(_choices > 1, "Number of choices must be bigger than 1!");
    id = ++self.ballotBoxIDs;
    BallotBox storage b = self.ballotBoxes[id];
    b.numSlots = _numSlots;
    b.numChoices = _choices;
    b.state = BallotBoxState.PrepayingGas;
    for (uint256 i = 0; i <= _choices; i++) {
      b.voteCount[i] = MSB_SET;
    }
    payForGas(self, id, 1);
  }

  function createProposal(
      Data storage self,
      uint256 num_slots,
      address payable contributor,
      string memory title,
      string memory documents_link,
      bytes memory documents_hash,
      uint256 budget_period_len,
      uint256 num_periods,
      uint256 budget_per_period) public returns (uint256 id) {
    id = createBallotBox(self, 2, num_slots);
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
  function payForGas(Data storage self, uint256 _id, uint256 _slotsToPay) public validBallotBoxID(self, _id) {
    BallotBox storage b = self.ballotBoxes[_id];
    uint256 _numSlots = b.numSlots;
    uint256 _paidSlots = b.paidSlots;
    require((_numSlots - _paidSlots) >= _slotsToPay, "Too many slots!");
    uint256 _newLength = _paidSlots + _slotsToPay;
    uint256 votesPerWord = 255 / (Util.msb(b.numChoices) + 1);
    uint256 votesWords = (_numSlots + votesPerWord - 1) / votesPerWord;
    for (uint256 i = 1; i <= _slotsToPay; i++) {
      uint256 idx = _newLength - i;
      b.payGas1[idx] = b.payGas2[idx] = MSB_SET;
      if (_newLength - i <= votesWords) {
        b.votes[idx] = MSB_SET;
      }
    }
    b.paidSlots = _newLength;
    if (_newLength == _numSlots) {
      b.state = BallotBoxState.Active;
    }
  }

  function calcVoteDifference(Data storage self, uint256 _id) view public validBallotBoxID(self, _id) returns (int256) {
    BallotBox storage b = self.ballotBoxes[_id];
    int256 yes = int256(b.voteCount[1]);
    int256 no = int256(b.voteCount[2]);
    return (yes - no) * 100 / int256(b.numSlots);
  }

  function castVote(Data storage self, uint256 _id, uint256 _slot, uint8 _choice) public validBallotBoxID(self, _id) {
    BallotBox storage box = self.ballotBoxes[_id];
    uint256 numChoices = box.numChoices;
    require(_choice <= numChoices, "Invalid choice");
    require(box.state == BallotBoxState.Active, "Ballot is not active!");

    uint256 bitsPerVote = Util.msb(numChoices) + 1;
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

  function getVote(
    Data storage self,
    uint256 _id,
    uint256 _slot
  ) public view validBallotBoxID(self, _id) returns (uint256) {
    uint256 numChoices = self.ballotBoxes[_id].numChoices;
    uint256 bitsPerVote = Util.msb(numChoices) + 1;
    uint256 votesPerWord = 255 / bitsPerVote;

    // Calculate masks.
    uint256 index = _slot / votesPerWord;
    uint256 offset = (_slot % votesPerWord) * bitsPerVote;
    uint256 mask = ((1 << bitsPerVote) - 1) << offset;

    // Get vote
    return (self.ballotBoxes[_id].votes[index] & mask) >> offset;
  }

  function updateProposalState(
    Data storage self, uint256 _id
  ) public validBallotBoxID(self, _id) returns (bool _return_to_treasury) {
    Proposal storage p = self.proposals[_id];
    ProposalState p_state = p.state;
    uint256 _initialEndDate = p.initialEndDate;
    if (p_state == ProposalState.Started) {
      BallotBox storage b = self.ballotBoxes[_id];
      if (b.state == BallotBoxState.Active) {  // Gas is paid
        if (_initialEndDate != 0) {
          if (now >= _initialEndDate) {
            int256 vote_diff = calcVoteDifference(self, _id);
            if (vote_diff >= self.approvalPercentage) {
              p.state = ProposalState.Accepted;
              p.nextPaymentDate = now;
            } else {
              p.state = ProposalState.Rejected;
              b.state = BallotBoxState.Inactive;
              _return_to_treasury = true;
            }
          }
        } else {
          // Either gas has been just paid and time hasn't been set
          // or the initial time hasn't passed
          p.initialEndDate = now + PROPOSAL_START_PERIOD;
        }
      }
    } else if (p_state == ProposalState.Accepted) {
      int256 vote_diff = calcVoteDifference(self, _id);
      if (vote_diff <= self.contestPercentage) {
        p.state = ProposalState.Contested;
        p.contestEndDate = now + CONTEST_PERIOD;
      }
    } else if (p_state == ProposalState.Contested) {
      if (now >= p.contestEndDate) {
        int256 vote_diff = calcVoteDifference(self, _id);
        if (vote_diff >= self.approvalPercentage) {
          p.state = ProposalState.Accepted;
        } else {
          p.state = ProposalState.Rejected;
          self.ballotBoxes[_id].state = BallotBoxState.Inactive;
          _return_to_treasury = true;
        }
      }
    }
  }

  // In case the contributor is inactive anyone could call the function
  // AFTER all periods are passed and the funds locked
  // in the proposal address will be returned to treasury.
  // Rejecting the proposal will have the same effect.
  function claimReward(
    Data storage self, uint256 _id, uint256 _budget
  ) public validBallotBoxID(self, _id)
  returns (bool _is_sender_transfer_allowed, uint256 _return_to_treasury) {
    Proposal storage p = self.proposals[_id];
    require(p.state == ProposalState.Accepted || p.state == ProposalState.Contested,
        "Incorrect proposal state!");
    uint256 paymentDate = p.nextPaymentDate;
    uint256 remainingPeriods = p.remainingPeriods;
    uint256 periodLen = p.budgetPeriodLen;
    uint256 budgetPerPeriod = p.budgetPerPeriod;

    require(_budget <= budgetPerPeriod, "Budget exceeded!");

    if (paymentDate > 0) {
      if (paymentDate <= now) {
        if (paymentDate + periodLen < now) {
          uint256 missedPeriods = (now - paymentDate) / periodLen;
          _return_to_treasury += missedPeriods * p.budgetPerPeriod;
          remainingPeriods -= missedPeriods;
          paymentDate += periodLen * missedPeriods;
        }
        if (remainingPeriods > 1) {
          require(p.contributor == msg.sender, "Invalid contributor!");
          _is_sender_transfer_allowed = true;
          if (budgetPerPeriod > _budget) {
            _return_to_treasury +=  budgetPerPeriod - _budget;
          }
          paymentDate += periodLen;
          remainingPeriods--;
        } else {
          if (remainingPeriods == 1) {
            require(p.contributor == msg.sender, "Invalid contributor!");
            _is_sender_transfer_allowed = true;
            if (budgetPerPeriod > _budget) {
              _return_to_treasury +=  budgetPerPeriod - _budget;
            }
          }
          p.nextPaymentDate = 0;
          remainingPeriods = 0;
          p.state = ProposalState.Completed;
          self.ballotBoxes[_id].state = BallotBoxState.Inactive;
        }
        p.remainingPeriods = remainingPeriods;
        p.nextPaymentDate = paymentDate;
      }
    }
  }

  function completeProposal(Data storage self, uint256 _id) public {
    require(self.ballotBoxes[_id].state == BallotBoxState.Active, "Ballot is not active!");
    Proposal storage p = self.proposals[_id];
    ProposalState p_state = p.state;
    require(p_state == ProposalState.Started || p_state == ProposalState.Contested,
        "Invalid proposal state!");
    p.state = ProposalState.Completed;
    self.ballotBoxes[_id].state = BallotBoxState.Inactive;
  }
}
