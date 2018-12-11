# Automaton crypto module reference

## Hash Interface


## DSA

### DSA Interface

**dsa.h**

```cpp
class dsa {
public:
  size_t public_key_size() = 0;
  size_t private_key_size() = 0;
  size_t message_size() = 0;
  size_t signature_size() = 0;
  size_t k_size();

  bool has_deterministic_signatures() = 0;

  void gen_public_key(const byte * private_key, byte * public_key);
  void sign(const byte * private_key, const byte * message, byte * signature);
  void sign_deterministic(const byte * private_key,
                          const byte * message,
                          const byte * k,
                          byte * signature);
  void verify(const byte * public_key, const byte * message, byte * signature);

  // algorithm
  // secp256k1
  // secp256r1
  // ed25519
  static dsa * create(string algorithm);
}
```

### DSA Examples

```

```
