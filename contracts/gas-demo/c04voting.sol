pragma solidity ^0.6.2;

// contract OptimusPrime04a {
//   address[] public owners;
//   bytes public votes;
//   uint256[] public voteCount;
//
//   constructor(uint256 _slots, uint256 _numChoices) public {
//     owners.length = _slots;
//     votes.length = _slots;
//     voteCount.length = _numChoices + 1;
//   }
//
//   function setOwner(uint256 _slot) public {
//     owners[_slot] = msg.sender;
//   }
//
//   function castVote(uint256 _slot, uint8 _choice) public {
//     require(msg.sender == owners[_slot], "Invalid slot owner");
//
//     // Modify vote selection.
//     byte oldVote = votes[_slot];
//     if (oldVote > 0) {
//       voteCount[uint8(oldVote)]--;
//     }
//
//     votes[_slot] = byte(_choice);
//     if (_choice != 0x00) {
//       voteCount[_choice]++;
//     }
//   }
//
//   function getVote(uint256 _slot) public view returns (byte) {
//     return votes[_slot];
//   }
// }
//
// contract OptimusPrime04b {
//   address[] public owners;
//
//   uint256[] public votes;
//   uint256[] public voteCount;
//
//   constructor(uint256 _slots, uint256 _numChoices) public {
//     owners.length = _slots;
//     votes.length = _slots;
//     voteCount.length = _numChoices + 1;
//   }
//
//   function setOwner(uint256 _slot) public {
//     owners[_slot] = msg.sender;
//   }
//
//   function castVote(uint256 _slot, uint8 _choice) public {
//     require(msg.sender == owners[_slot], "Invalid slot owner");
//
//     // Modify vote selection.
//     uint256 oldVote = votes[_slot];
//     if (oldVote > 0) {
//       voteCount[oldVote]--;
//     }
//
//     votes[_slot] = _choice;
//     if (_choice != 0x00) {
//       voteCount[_choice]++;
//     }
//   }
//
//   function getVote(uint256 _slot) public view returns (uint256) {
//     return votes[_slot];
//   }
// }

contract OptimusPrime04c {
  address[65536] public owners;
  byte[65536] public votes;
  uint256[256] public voteCount;

  constructor() public {
  }

  function setOwner(uint256 _slot) public {
    owners[_slot] = msg.sender;
  }

  function castVote(uint256 _slot, uint8 _choice) public {
    require(msg.sender == owners[_slot], "Invalid slot owner");

    // Modify vote selection.
    byte oldVote = votes[_slot];
    if (oldVote > 0) {
      voteCount[uint8(oldVote)]--;
    }

    votes[_slot] = byte(_choice);
    if (_choice != 0x00) {
      voteCount[_choice]++;
    }
  }

  function getVote(uint256 _slot) public view returns (byte) {
    return votes[_slot];
  }
}


contract OptimusPrime04d {
  address[65536] public owners;

  uint256[65536] public votes;
  uint256[256] public voteCount;

  constructor() public {
  }

  function setOwner(uint256 _slot) public {
    owners[_slot] = msg.sender;
  }

  function castVote(uint256 _slot, uint8 _choice) public {
    require(msg.sender == owners[_slot], "Invalid slot owner");

    // Modify vote selection.
    uint256 oldVote = votes[_slot];
    if (oldVote > 0) {
      voteCount[oldVote]--;
    }

    votes[_slot] = _choice;
    if (_choice != 0x00) {
      voteCount[_choice]++;
    }
  }

  function getVote(uint256 _slot) public view returns (uint256) {
    return votes[_slot];
  }
}

contract Ballot {
  struct Voter {
    uint weight; // weight > 0 => can vote
    bool voted;  // if true, that person already voted
    uint vote;   // index of the voted proposal
  }

  struct Proposal {
    bytes32 name;   // short name (up to 32 bytes)
    uint voteCount; // number of accumulated votes
  }

  address public chairperson;

  mapping(address => Voter) public voters;
  Proposal[] public proposals;

  // Create a new ballot to choose one of `proposalNames`.
  constructor() public {
    chairperson = msg.sender;
    voters[chairperson].weight = 1;

    for (uint i = 0; i < 5; i++) {
      proposals.push(Proposal({
          name: bytes32(i),
          voteCount: 0
      }));
    }
  }

  function vote(uint proposal) public {
    Voter storage sender = voters[msg.sender];
    require(sender.weight != 0, "Has no right to vote");
    require(!sender.voted, "Already voted.");
    sender.voted = true;
    sender.vote = proposal;

    proposals[proposal].voteCount += sender.weight;
  }

  function winningProposal() public view returns (uint winningProposal_) {
    uint winningVoteCount = 0;
    for (uint p = 0; p < proposals.length; p++) {
      if (proposals[p].voteCount > winningVoteCount) {
        winningVoteCount = proposals[p].voteCount;
        winningProposal_ = p;
      }
    }
  }

  function winnerName() public view returns (bytes32 winnerName_) {
      winnerName_ = proposals[winningProposal()].name;
  }
}
