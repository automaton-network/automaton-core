let catchRevert            = require("./exceptions.js").catchRevert;
let catchOutOfGas          = require("./exceptions.js").catchOutOfGas;
let catchInvalidJump       = require("./exceptions.js").catchInvalidJump;
let catchInvalidOpcode     = require("./exceptions.js").catchInvalidOpcode;
let catchStackOverflow     = require("./exceptions.js").catchStackOverflow;
let catchStackUnderflow    = require("./exceptions.js").catchStackUnderflow;
let catchStaticStateChange = require("./exceptions.js").catchStaticStateChange;

let utils = require('./utils.js');
let increaseTime = utils.increaseTime;

const VOTE_YES = 1;
const VOTE_NO = 2;

describe('TestKingAutomatonProposals1', async() => {
  const KingAutomaton = artifacts.require("KingAutomaton");

  beforeEach(async() => {
    accounts = await web3.eth.getAccounts();
    account = accounts[0];
    slots = 4;
    koh = await KingAutomaton.new(slots, 16, "0x010000", "406080000", 10, -10);
    id = await koh.createProposal.call(account, "", "", "0x");
    await koh.createProposal(account, "", "", "0x");
    proposal_start_period = 90;
    contest_period = 90;
  });

  it("correct proposal creation", async() => {
    assert.equal(id, 2, "Incorrect ID!");
    assert.exists(account, "Account doesn't exist!");
    assert.exists(koh.address, "Contract wasn't deployed!");
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 1, "Ballot box state is not PrepayingGas!");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 1, "Proposal state is not Started!");
    let approval_percentage = await koh.approval_percentage();
    assert.equal(approval_percentage, 10, "Approval % in incorrect!");
    let contest_percentage = await koh.contest_percentage();
    assert.equal(contest_percentage, -10, "Contest % in incorrect!");
  });

  it("cast vote invalid slot owner", async() => {
    await koh.payForGas(id, slots - 1);
    await catchRevert(koh.castVote(id, 0, VOTE_YES), "Invalid slot owner!");
  });

  it("cast vote invalid ballot box id", async() => {
    await koh.setOwnerAllSlots();
    await catchRevert(koh.castVote(200, 0, VOTE_YES), "Invalid ballot box ID!");
  });

  it("cast vote before gas is paid", async() => {
    await koh.setOwnerAllSlots();
    await catchRevert(koh.castVote(id, 0, VOTE_YES), "Ballot is not active!");
  });

  it("pay gas for more slots", async() => {
    await catchRevert(koh.payForGas(id, 20), "Too many slots!");
  });

  it("vote change", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, slots - 1);
    // Test initial vote values
    let vote = await koh.getVote(id, 0);
    let pos_vote_count = await koh.getVoteCount(id, 1);
    let neg_vote_count = await koh.getVoteCount(id, 2);
    assert.equal(vote, 0, "Incorrect initial vote!");
    assert.equal(pos_vote_count, 0, "Incorrect initial positive vote count!");
    assert.equal(neg_vote_count, 0, "Incorrect initial negative vote count!");
    // Cast positive vote
    await koh.castVote(id, 0, VOTE_YES);
    vote = await koh.getVote(id, 0);
    pos_vote_count = await koh.getVoteCount(id, VOTE_YES);
    assert.equal(vote, VOTE_YES, "Incorrect vote!");
    assert.equal(pos_vote_count, 1, "Incorrect positive vote count!");
    // Change vote
    await koh.castVote(id, 0, VOTE_NO);
    vote = await koh.getVote(id, 0);
    assert.equal(vote, VOTE_NO, "Incorrect vote change!");
  });

  // 1 negative 0 positive / -25% approval < rejection %
  it("rejection during initial voting", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, slots - 1);
    // Check if states are correct
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (0)");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 1, "Proposal state is not Started! (0)");
    // Cast negative vote
    await koh.castVote(id, 0, VOTE_NO);
    await koh.updateProposalState(id);
    // States shouldn't change during initial time
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (1)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 1, "Proposal state is not Started! (1)");
    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive! (2)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected! (2)");
    let vote_difference = await koh.calcVoteDifference(id);
    assert.equal(vote_difference, -25, "Vote difference is incorrect!");
  });

  // 0 votes / 0% approval < approval %
  it("no voting rejection during initial voting", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, slots - 1);
    await koh.updateProposalState(id);
    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive!");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected!");
    let vote_difference = await koh.calcVoteDifference(id);
    assert.equal(vote_difference, 0, "Vote difference is incorrect!");
  });

  it("approval then rejection during initial voting", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, slots - 1);

    await koh.castVotesForApproval(id);
    await koh.castVotesForRejection(id);

    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive!");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected!");
  });

  it("contested then rejection", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, slots - 1);

    await koh.castVotesForApproval(id);
    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (0)");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 2, "Proposal state is not Accepted! (0)");

    // Vote negative to enter contested state
    await koh.castVotesForRejection(id);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (1)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 4, "Proposal state is not Contested! (1)");

    // Wait for contest to end
    increaseTime(contest_period);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive! (2)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected! (2)");
  });

  it("contested then approval", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, slots - 1);

    // Vote positive for approval
    await koh.castVotesForApproval(id);
    await koh.updateProposalState(id);
    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (0)");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 2, "Proposal state is not Accepted! (0)");

    // Vote negative to enter contested state
    await koh.castVotesForRejection(id);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (1)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 4, "Proposal state is not Contested! (1)");

    // Vote positive
    await koh.castVotesForApproval(id);
    await koh.updateProposalState(id);
    // States shouldn't change until the end of contest
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (2)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 4, "Proposal state is not Contested! (2)");

    // Wait for contest to end
    increaseTime(contest_period);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (3)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 2, "Proposal state is not Accepted! (3)");
  });
});


describe('TestKingAutomatonProposals2', async() => {
  const KingAutomaton = artifacts.require("KingAutomaton");

  beforeEach(async() => {
    accounts = await web3.eth.getAccounts();
    account = accounts[0];
    slots = 256;
    koh = await KingAutomaton.new(slots, 16, "0x010000", "406080000", 10, -10);
    id = await koh.createProposal.call(account, "", "", "0x");
    await koh.createProposal(account, "", "", "0x");
    proposal_start_period = 90;
    contest_period = 90;
  });

  it("rejection during initial voting", async() => {
    await koh.setOwnerAllSlots();

    let slots_to_pay = await koh.unpaidSlots(id);
    while (slots_to_pay >= 64) {
      await koh.payForGas(id, 64);
      slots_to_pay = await koh.unpaidSlots(id);
    }
    if (slots_to_pay > 0) {
      await koh.payForGas(id, slots_to_pay);
    }
    // Check if states are correct
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (0)");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 1, "Proposal state is not Started! (0)");

    await koh.castVotesForRejection(id);
    await koh.updateProposalState(id);
    // States shouldn't change during initial time
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (1)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 1, "Proposal state is not Started! (1)");
    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive! (2)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected! (2)");
  });

  it("no voting rejection during initial voting", async() => {
    await koh.setOwnerAllSlots();
    let slots_to_pay = await koh.unpaidSlots(id);
    while (slots_to_pay >= 64) {
      await koh.payForGas(id, 64);
      slots_to_pay = await koh.unpaidSlots(id);
    }
    if (slots_to_pay > 0) {
      await koh.payForGas(id, slots_to_pay);
    }
    await koh.updateProposalState(id);
    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive!");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected!");
  });

  it("approval then rejection during initial voting", async() => {
    await koh.setOwnerAllSlots();
    let slots_to_pay = await koh.unpaidSlots(id);
    while (slots_to_pay >= 64) {
      await koh.payForGas(id, 64);
      slots_to_pay = await koh.unpaidSlots(id);
    }
    if (slots_to_pay > 0) {
      await koh.payForGas(id, slots_to_pay);
    }
    await koh.castVotesForApproval(id);
    await koh.castVotesForRejection(id);

    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive!");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected!");
  });

  it("correct voting", async() => {
    await koh.setOwnerAllSlots();
    let slots_to_pay = await koh.unpaidSlots(id);
    while (slots_to_pay >= 64) {
      await koh.payForGas(id, 64);
      slots_to_pay = await koh.unpaidSlots(id);
    }
    if (slots_to_pay > 0) {
      await koh.payForGas(id, slots_to_pay);
    }

    let votes_num = await koh.castVotesForApproval.call(id);
    await koh.castVotesForApproval(id);
    for (i = 0; i < votes_num; ++i) {
      let vote = await koh.getVote(id, i);
      assert.equal(vote, VOTE_YES, "Incorrect vote! Vote must be positive");
    }

    votes_num = await koh.castVotesForRejection.call(id);
    await koh.castVotesForRejection(id);
    for (i = 0; i < votes_num; ++i) {
      let vote = await koh.getVote(id, i);
      assert.equal(vote, VOTE_NO, "Incorrect vote! Vote must be negative");
    }
  });

  it("contested then rejection", async() => {
    await koh.setOwnerAllSlots();
    let slots_to_pay = await koh.unpaidSlots(id);
    while (slots_to_pay >= 64) {
      await koh.payForGas(id, 64);
      slots_to_pay = await koh.unpaidSlots(id);
    }
    if (slots_to_pay > 0) {
      await koh.payForGas(id, slots_to_pay);
    }

    await koh.castVotesForApproval(id);
    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (0)");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 2, "Proposal state is not Accepted! (0)");

    // Vote negative to enter contested state
    await koh.castVotesForRejection(id);

    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (1)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 4, "Proposal state is not Contested! (1)");

    // Wait for contest to end
    increaseTime(contest_period);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive! (2)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected! (2)");
  });

  it("contested then approval", async() => {
    await koh.setOwnerAllSlots();
    let slots_to_pay = await koh.unpaidSlots(id);
    while (slots_to_pay >= 64) {
      await koh.payForGas(id, 64);
      slots_to_pay = await koh.unpaidSlots(id);
    }
    if (slots_to_pay > 0) {
      await koh.payForGas(id, slots_to_pay);
    }

    await koh.castVotesForApproval(id);
    await koh.updateProposalState(id);
    increaseTime(proposal_start_period);
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (0)");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 2, "Proposal state is not Accepted! (0)");

    // Vote negative to enter contested state
    await koh.castVotesForRejection(id);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (1)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 4, "Proposal state is not Contested! (1)");

    // Vote positive
    await koh.castVotesForApproval(id);
    await koh.updateProposalState(id);
    // States shouldn't change until the end of contest
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (2)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 4, "Proposal state is not Contested! (2)");
    vote_difference = await koh.calcVoteDifference(id);

    // Wait for contest to end
    increaseTime(contest_period);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active! (3)");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 2, "Proposal state is not Accepted! (3)");
  });
});
