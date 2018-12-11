#include "automaton/core/crypto/ed25519_orlp/ed25519_orlp.h"
#include <ed25519.h>
#include <string>
#include <iostream>

namespace automaton {
namespace core {
namespace crypto {
namespace ed25519_orlp {

size_t ed25519_orlp::public_key_size() {
  return 32;
}
// This is actually the seed
size_t ed25519_orlp::private_key_size() {
  return 32;
}
size_t ed25519_orlp::signature_size() {
  return 64;
}
// TODO(Samir): max input k size?
size_t ed25519_orlp::k_size() {
  return 0;
}

bool ed25519_orlp::has_deterministic_signatures() {
  return true;
}

void ed25519_orlp::gen_public_key(const uint8_t * private_key,
                                      uint8_t * public_key) {
  uint8_t private_from_seed[64];
  ed25519_create_keypair(public_key, private_from_seed, private_key);
}

// We can have faster implementation if we pass the public key aswell
void ed25519_orlp::sign(const uint8_t * private_key,
                        const uint8_t * message,
                        const size_t msg_len,
                        uint8_t * signature) {
  uint8_t public_key[32];
  uint8_t private_from_seed[64];
  ed25519_create_keypair(public_key, private_from_seed, private_key);

  ed25519_sign(signature, message, msg_len, public_key, private_from_seed);
}

void ed25519_orlp::sign_deterministic(const uint8_t * private_key,
                                      const uint8_t * message,
                                      const size_t msg_len,
                                      const uint8_t * k,
                                      uint8_t * signature) {
  sign(private_key, message, msg_len, signature);
}

bool ed25519_orlp::verify(const uint8_t * public_key,
                          const uint8_t * message,
                          const size_t msg_len,
                          const uint8_t * signature) {
  return ed25519_verify(signature, message, msg_len, public_key);
}

}  // namespace ed25519_orlp
}  // namespace crypto
}  // namespace core
}  // namespace automaton
