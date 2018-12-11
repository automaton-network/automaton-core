#include "automaton/core/script/engine.h"

#include "automaton/core/crypto/cryptopp/Keccak_256_cryptopp.h"
#include "automaton/core/crypto/cryptopp/RIPEMD160_cryptopp.h"
#include "automaton/core/crypto/cryptopp/secp256k1_cryptopp.h"
#include "automaton/core/crypto/cryptopp/secure_random_cryptopp.h"
#include "automaton/core/crypto/cryptopp/SHA256_cryptopp.h"
#include "automaton/core/crypto/cryptopp/SHA3_256_cryptopp.h"
#include "automaton/core/crypto/cryptopp/SHA512_cryptopp.h"
#include "automaton/core/crypto/ed25519_orlp/ed25519_orlp.h"
#include "automaton/core/io/io.h"

using automaton::core::crypto::cryptopp::Keccak_256_cryptopp;
using automaton::core::crypto::cryptopp::RIPEMD160_cryptopp;
using automaton::core::crypto::cryptopp::secure_random_cryptopp;
using automaton::core::crypto::cryptopp::SHA256_cryptopp;
using automaton::core::crypto::cryptopp::SHA512_cryptopp;
using automaton::core::crypto::cryptopp::SHA3_256_cryptopp;
using automaton::core::crypto::cryptopp::secp256k1_cryptopp;
using automaton::core::io::hex2bin;

namespace automaton {
namespace core {
namespace script {

void engine::bind_crypto() {
  ripemd160.reset(new RIPEMD160_cryptopp());
  sha512.reset(new SHA512_cryptopp());
  sha256.reset(new SHA256_cryptopp());
  sha3.reset(new SHA3_256_cryptopp());
  keccak256.reset(new Keccak_256_cryptopp());
  secp256k1.reset(new secp256k1_cryptopp());
  random.reset(new secure_random_cryptopp());

  set_function("rand", [&](size_t size) {
    CHECK_LT(size, 1024);
    uint8_t buf[1024];
    random->block(&buf[0], size);
    return std::string(reinterpret_cast<char*>(buf), size);
  });

  set_function("ripemd160", [&](const std::string& s) -> std::string {
    uint8_t digest[20];
    ripemd160->calculate_digest(reinterpret_cast<const uint8_t*>(s.data()), s.size(), digest);
    return std::string(reinterpret_cast<char*>(digest), 20);
  });

  set_function("sha512", [&](const std::string& s) -> std::string {
    uint8_t digest[64];
    sha512->calculate_digest(reinterpret_cast<const uint8_t*>(s.data()), s.size(), digest);
    return std::string(reinterpret_cast<char*>(digest), 64);
  });

  set_function("sha256", [&](const std::string& s) -> std::string {
    uint8_t digest[32];
    sha256->calculate_digest(reinterpret_cast<const uint8_t*>(s.data()), s.size(), digest);
    return std::string(reinterpret_cast<char*>(digest), 32);
  });

  set_function("sha3", [&](const std::string& s) -> std::string {
    uint8_t digest[32];
    sha3->calculate_digest(reinterpret_cast<const uint8_t*>(s.data()), s.size(), digest);
    return std::string(reinterpret_cast<char*>(digest), 32);
  });

  set_function("keccak256", [&](const std::string& s) -> std::string {
    uint8_t digest[32];
    keccak256->calculate_digest(reinterpret_cast<const uint8_t*>(s.data()), s.size(), digest);
    return std::string(reinterpret_cast<char*>(digest), 32);
  });

  // ECDSA functions
  set_function("secp256k1_sign", [&](const std::string& pr_key,
                const std::string& msg) -> std::string {
    uint8_t signature[64];
    secp256k1->sign(reinterpret_cast<const uint8_t*>(pr_key.data()),
                    reinterpret_cast<const uint8_t*>(msg.data()),
                    msg.size(),
                    signature);
    return std::string(reinterpret_cast<char*>(signature), secp256k1->signature_size());
  });

  set_function("secp256k1_gen_public_key", [&](const std::string& pr_key) -> std::string {
    uint8_t public_key[33];
    secp256k1->gen_public_key(reinterpret_cast<const uint8_t*>(pr_key.data()), public_key);
    return std::string(reinterpret_cast<char*>(public_key), secp256k1->public_key_size());
  });

  set_function("secp256k1_verify", [&](const std::string& pub_key, const std::string& msg,
              const std::string& signature) -> bool {
    return secp256k1->verify(reinterpret_cast<const uint8_t*>(pub_key.data()),
                             reinterpret_cast<const uint8_t*>(msg.data()),
                             msg.size(),
                             reinterpret_cast<const uint8_t*>(signature.data()));
  });
}

}  // namespace script
}  // namespace core
}  // namespace automaton
