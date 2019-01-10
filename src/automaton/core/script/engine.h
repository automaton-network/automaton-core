#ifndef AUTOMATON_CORE_SCRIPT_ENGINE_H_
#define AUTOMATON_CORE_SCRIPT_ENGINE_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "automaton/core/crypto/digital_signature.h"
#include "automaton/core/crypto/hash_transformation.h"
#include "automaton/core/crypto/secure_random.h"
#include "automaton/core/data/factory.h"

#include "sol.hpp"

namespace automaton {
namespace core {
namespace script {

/**
  Lua script engine wrapper and Autoamaton's module bridge.
*/
class engine : public sol::state {
 public:
  engine();
  explicit engine(std::shared_ptr<data::factory> data_factory);
  ~engine();

  void bind_core() {
    bind_crypto();
    bind_data();
    bind_io();
    bind_log();
    bind_network();
    bind_state();
  }

  void bind_crypto();

  void bind_data();

  void bind_io();

  void bind_log();

  void bind_network();

  void bind_state();

  void import_schema(data::schema* msg_schema);

  void set_factory(std::shared_ptr<data::factory> factory);

  std::shared_ptr<data::factory> get_factory() {
    return data_factory;
  }

 private:
  std::shared_ptr<data::factory> data_factory;

  // Crypto hash functions
  std::unique_ptr<crypto::hash_transformation> ripemd160;
  std::unique_ptr<crypto::hash_transformation> sha512;
  std::unique_ptr<crypto::hash_transformation> sha256;
  std::unique_ptr<crypto::hash_transformation> sha3;
  std::unique_ptr<crypto::hash_transformation> keccak256;

  // Crypto ECDSA functions
  std::unique_ptr<crypto::digital_signature> secp256k1;

  // Secure random
  std::unique_ptr<crypto::secure_random> random;
};

}  // namespace script
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_SCRIPT_ENGINE_H_
