#include "automaton/core/crypto/cryptopp/secp256k1_cryptopp.h"

#include <cryptlib.h>
#include <eccrypto.h>
#include <integer.h>
#include <oids.h>
#include <osrng.h>
#include <randpool.h>
#include <iostream>
#include <string>

#include "automaton/core/crypto/digital_signature.h"

namespace automaton {
namespace core {
namespace crypto {
namespace cryptopp {

/*
TODO(Samir): set domain params from the following example
https://stackoverflow.com/a/45796422
OID curve = ASN1::secp256r1();
ECDH<ECP>::Domain domain(curve);
====================================================
// To get the size of the private and public keys:
size_t len = domain.PrivateKeyLength()
====================================================
SecByteBlock privKey(domain.PrivateKeyLength());
SecByteBlock pubKey(domain.PublicKeyLength());
*/

size_t secp256k1_cryptopp::public_key_size() {
  return 33;
}

size_t secp256k1_cryptopp::private_key_size() {
  return 32;
}

size_t secp256k1_cryptopp::signature_size() {
  return 64;
}

// TODO(Samir): max input k size?
size_t secp256k1_cryptopp::k_size() {
  return 0;
}

bool secp256k1_cryptopp::has_deterministic_signatures() {
  return false;
}

void secp256k1_cryptopp::gen_public_key(const uint8_t * private_key, uint8_t * public_key) {
  // Create private key object from exponent
  const CryptoPP::Integer privateExponent(private_key, private_key_size());
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
  privateKey.Initialize(CryptoPP::ASN1::secp256k1(), privateExponent);

  // Create the public key object
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;
  privateKey.MakePublicKey(publicKey);

  // Save the public key in public_key
  publicKey.GetGroupParameters().GetCurve().EncodePoint(public_key,
      publicKey.GetPublicElement(), true);
}

void secp256k1_cryptopp::sign(const uint8_t * private_key,
                        const uint8_t * message,
                        const size_t msg_len,
                        uint8_t * signature) {
  CryptoPP::AutoSeededRandomPool prng;

  std::string str_signature;
  // Create private key object from exponent
  const CryptoPP::Integer privateExponent(private_key, private_key_size());
  // TODO(Samir): Need to use identity hash intead of SHA256
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
  privateKey.Initialize(CryptoPP::ASN1::secp256k1(), privateExponent);

  memset(signature, 0, signature_size());
  CryptoPP::StringSource(message, msg_len, true,
    new CryptoPP::SignerFilter(prng, CryptoPP::ECDSA<CryptoPP::ECP,
        CryptoPP::SHA256>::Signer(privateKey),
            new CryptoPP::ArraySink(signature, signature_size())));
  // std::cout << "signature length: " << str_signature.length() << std::endl;
  // std::cout << "signature: " << str_signature << std::endl;
}

void secp256k1_cryptopp::sign_deterministic(
    const uint8_t * private_key,
    const uint8_t * message,
    const size_t msg_len,
    const uint8_t * k,
    uint8_t * signature) {
  throw;
}

bool secp256k1_cryptopp::verify(
    const uint8_t * public_key,
    const uint8_t * message,
    const size_t msg_len,
    const uint8_t * signature) {
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;

  std::string input(reinterpret_cast<const char*>(public_key),
      public_key_size());
  CryptoPP::ECP::Point p;
  publicKey.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256k1());
  publicKey.GetGroupParameters().GetCurve().DecodePoint(p, public_key,
      public_key_size());
  publicKey.SetPublicElement(p);
  // TODO(Samir): Change SHA256 to identity hash
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Verifier
      verifier(publicKey);
  return verifier.VerifyMessage(message, msg_len, signature,
      signature_size());
}

}  // namespace cryptopp
}  // namespace crypto
}  // namespace core
}  // namespace automaton
