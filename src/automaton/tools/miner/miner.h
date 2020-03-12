#ifndef AUTOMATON_TOOLS_MINER_MINER_H_
#define AUTOMATON_TOOLS_MINER_MINER_H_

#include <assert.h>
#include <secp256k1.h>
#include <secp256k1_recovery.h>

#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>


// will need tests, so we need to make it library,
// but we also need it to be executable printing the information
// in the correct format

namespace automaton {
namespace tools {
namespace miner {

// Generate random keys untill a public key ^
// Precondition mask == 32 bytes
//              difficulty == 32 bytes
//              pr_key == 32 bytes
// IN:  mask:    the mask that to apply to the public key before comparing it to difficulty
//      difficulty:   the public key should be greater than the difficulty after applying the mask.
// OUT: pr_key:   mined private key
// @returns total number of keys generated
unsigned int mine_key(unsigned char* mask, unsigned char* difficulty, unsigned char* pr_key, int max_attempts = 1000);

std::string sign(const unsigned char* priv_key, const unsigned char* msg_hash);

std::string gen_pub_key(const unsigned char* priv_key);

}  // namespace miner
}  // namespace tools
}  // namespace automaton

#endif  //  AUTOMATON_TOOLS_MINER_MINER_H_
