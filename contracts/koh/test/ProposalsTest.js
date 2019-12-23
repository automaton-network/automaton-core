let catchRevert            = require("./exceptions.js").catchRevert;
let catchOutOfGas          = require("./exceptions.js").catchOutOfGas;
let catchInvalidJump       = require("./exceptions.js").catchInvalidJump;
let catchInvalidOpcode     = require("./exceptions.js").catchInvalidOpcode;
let catchStackOverflow     = require("./exceptions.js").catchStackOverflow;
let catchStackUnderflow    = require("./exceptions.js").catchStackUnderflow;
let catchStaticStateChange = require("./exceptions.js").catchStaticStateChange;


describe('TestKingAutomatonProposals1', async() => {
  const KingAutomaton = artifacts.require("KingAutomaton");

  beforeEach(async() => {
    accounts = await web3.eth.getAccounts();
    account = accounts[0];
    koh = await KingAutomaton.new(4, 16, "0x010000", "406080000");
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
  });

  it("cast vote invalid slot owner", async() => {
    await koh.payForGas(id, 3);
    await catchRevert(koh.castVote(id, 0, 1), "Invalid slot owner!");
  });

  it("cast vote invalid ballot box id", async() => {
    await koh.setOwnerAllSlots();
    await catchRevert(koh.castVote(200, 0, 1), "Invalid ballot box ID!");
  });

  it("cast vote before gas is paid", async() => {
    await koh.setOwnerAllSlots();
    await catchRevert(koh.castVote(id, 0, 1), "Ballot is not active!");
  });

  it("pay gas for more slots", async() => {
    await catchRevert(koh.payForGas(id, 20), "Too many slots!");
  });

  it("vote change", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, 3);
    // Test initial vote values
    let vote = await koh.getVote(id, 0);
    let pos_vote_count = await koh.getVoteCount(id, 1);
    let neg_vote_count = await koh.getVoteCount(id, 2);
    assert.equal(vote, 0, "Incorrect initial vote!");
    assert.equal(pos_vote_count, 0, "Incorrect initial positive vote count!");
    assert.equal(neg_vote_count, 0, "Incorrect initial negative vote count!");
    // Cast positive vote
    await koh.castVote(id, 0, 1);
    vote = await koh.getVote(id, 0);
    pos_vote_count = await koh.getVoteCount(id, 1);
    assert.equal(vote, 1, "Incorrect vote!");
    assert.equal(pos_vote_count, 1, "Incorrect positive vote count!");
    // Change vote
    await koh.castVote(id, 0, 2);
    vote = await koh.getVote(id, 0);
    assert.equal(vote, 2, "Incorrect vote change!");
  });

  // 1 negative 0 positive / -25% approval < rejection %
  it("rejection during initial voting", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, 3);
    // Check if states are correct
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active!");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 1, "Proposal state is not Started!");
    // Cast negative vote
    await koh.castVote(id, 0, 2);
    await koh.updateProposalState(id);
    // States shouldn't change during initial time
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active!");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 1, "Proposal state is not Started!");
    await await web3.currentProvider.send(
        {jsonrpc: "2.0", method: "evm_increaseTime", params: [proposal_start_period], id: 0}, (err, result) => {
        if (err) {
          console.log(err);
        }
      });
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive!");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected!");
    let vote_difference = await koh.calcVoteDifference(id);
    assert.equal(vote_difference, -25, "Vote difference is incorrect!");
  });

  // 0 votes / 0% approval < approval %
  it("no voting rejection during initial voting", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, 3);
    await koh.updateProposalState(id);

    await await web3.currentProvider.send(
        {jsonrpc: "2.0", method: "evm_increaseTime", params: [proposal_start_period], id: 0}, (err, result) => {
        if (err) {
          console.log(err);
        }
      });
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

  // 2 positive 1 negative // 25% approval
  it("approval then refection during initial voting", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, 3);

    // Cast 2 positive votes
    await koh.castVote(id, 0, 1);
    await koh.castVote(id, 1, 1);
    // Cast negative vote
    await koh.castVote(id, 2, 2);

    await await web3.currentProvider.send(
        {jsonrpc: "2.0", method: "evm_increaseTime", params: [proposal_start_period], id: 0}, (err, result) => {
        if (err) {
          console.log(err);
        }
      });
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive!");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected!");
    let vote_difference = await koh.calcVoteDifference(id);
    assert.equal(vote_difference, 25, "Vote difference is incorrect!");
  });

  it("contested then rejection", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, 3);

    // Cast 2 positive votes
    await koh.castVote(id, 0, 1);
    await koh.castVote(id, 1, 1);
    await await web3.currentProvider.send(
        {jsonrpc: "2.0", method: "evm_increaseTime", params: [proposal_start_period], id: 0}, (err, result) => {
        if (err) {
          console.log(err);
        }
      });
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active!");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 2, "Proposal state is not Accepted!");

    // Vote negative to enter contested state
    await koh.castVotesForRejection(id);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active!");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 4, "Proposal state is not Contested!");

    // Wait for contest to end
    await await web3.currentProvider.send(
        {jsonrpc: "2.0", method: "evm_increaseTime", params: [contest_period], id: 0}, (err, result) => {
        if (err) {
          console.log(err);
        }
      });
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 3, "Ballot box state is not Inactive!");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 3, "Proposal state is not Rejected!");
  });

  it("contested then approval", async() => {
    await koh.setOwnerAllSlots();
    await koh.payForGas(id, 3);

    // Cast 2 positive votes
    await koh.castVote(id, 0, 1);
    await koh.castVote(id, 1, 1);
    await koh.updateProposalState(id);
    await await web3.currentProvider.send(
        {jsonrpc: "2.0", method: "evm_increaseTime", params: [proposal_start_period], id: 0}, (err, result) => {
        if (err) {
          console.log(err);
        }
      });
    await koh.updateProposalState(id);
    let ballot = await koh.ballot_boxes(id);
    let ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active!");
    let proposal = await koh.proposals(id)
    let proposal_state = proposal.state;
    assert.equal(proposal_state, 2, "Proposal state is not Accepted!");

    // Vote negative to enter contested state
    await koh.castVotesForRejection(id);
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active!");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 4, "Proposal state is not Contested!");

    // Vote positive
    await koh.castVotesForApproval(id);
    await koh.updateProposalState(id);
    // States shouldn't change until the end of contest
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active!");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 4, "Proposal state is not Contested!");


    // Wait for contest to end
    await await web3.currentProvider.send(
        {jsonrpc: "2.0", method: "evm_increaseTime", params: [contest_period], id: 0}, (err, result) => {
        if (err) {
          console.log(err);
        }
      });
    await koh.updateProposalState(id);
    ballot = await koh.ballot_boxes(id);
    ballot_state = ballot.state;
    assert.equal(ballot_state, 2, "Ballot box state is not Active!");
    proposal = await koh.proposals(id)
    proposal_state = proposal.state;
    assert.equal(proposal_state, 2, "Proposal state is not Accepted!");
  });
});
