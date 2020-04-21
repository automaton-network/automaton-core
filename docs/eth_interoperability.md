## Calling ethereum functions and sending transactions

```
/**
  @param[in] fname function name/alias as given in register_contract
  @param[in] params
    case 1: sending raw transaction -> params is signed transaction
    case 2: params is list in json format where 'bytes' and 'address' are given in hex, integers in decimal,
    'string' is string, 'boolean' is "true" or "false" (without quotes)
    ! 'fixed' numbers are not yet supported !
  @param[in] private_key in hex (without 0x prefix)
  @param[in] gas_price in hex (without 0x prefix)
  @param[in] gas_limit in hex (without 0x prefix)
  @param[in] value in hex (without 0x prefix)

  @returns status code:
    1) if function call was successful status code is 'OK' and status msg contains the function result:
    transaction receipt if the function is a transaction, decoded function result in json format, otherwise
    2) if function call failed status code shows error type and status msg contains the error
*/
common::status call(const std::string& fname, const std::string& params,
    const std::string& private_key = "", const std::string& value = "",
    const std::string& gas_price = "", const std::string& gas_limit = "");
```

### Ethereum functions that do not change the state

Example usage:

```
status s = call("getSlotOwner", "[2]");
```

Example result stored in ```s.msg```:

```
["603CB0D1C8AB86E72BEB3C7DF564A36D7B85ECD2"]
```

### Ethereum functions that change the state (transactions)

Transaction receipt is returned in ```s.msg```.
#### Sending transaction

```
s = contract->call("createProposal",
"[\"603CB0d1c8ab86E72beb3c7DF564A36D7B85ecD2\", \"title\",\"documents_link\",\"\",259200,3,20]",
<PRIVATE_KEY>);
```

#### Sending raw transaction

The second parameter is the signed transaction

```
s = contract->call("claimSlot", "6B2C8C48FFFF28CAC197730D84B5....");
```

* Using eth_transaction class

```
std::stringstream claim_slot_data;
claim_slot_data
    // claimSlot signature
    << "6b2c8c48"
    // pubKeyX
    << bin2hex(pub_key.substr(0, 32))
    // pubKeyY
    << bin2hex(pub_key.substr(32, 32))
    // v
    << std::string(62, '0') << bin2hex(sig.substr(64, 1))
    // r
    << bin2hex(sig.substr(0, 32))
    // s
    << bin2hex(sig.substr(32, 32));

eth_transaction t;
t.nonce = "1a";
t.gas_price = "1388";  // 5 000
t.gas_limit = "5B8D80";  // 6M
t.to = "4B7c78aB094Ab856eAdA523Fa847236567e1750E";
t.value = "";
t.data = claim_slot_data.str();
t.chain_id = "01";

s = contract->call("claimSlot", t.sign_tx(<PRIVATE_KEY>));
```
