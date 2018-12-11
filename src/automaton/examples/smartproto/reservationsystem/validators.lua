-- validators.lua

validators = {}
validators_sorted = {}

-- initialize our validator key
private_key = sha256(nodeid)
public_key = secp256k1_gen_public_key(private_key)
validators[public_key] = nodeid

function validators_msg()
  local m = Hello()
  m.name = nodeid
  for k in pairs(validators) do
    local v = Validator()
    v.public_key = k
    v.name = validators[k]
    m.validators = v;
  end
  return m
end

function on_Hello(peer_id, msg)
  peers[peer_id].name = msg.name

  local new_validator = false
  for k,v in pairs(msg.validators) do
    if validators[v.public_key] == nil then
      log("on_Hello", nodeid .. " RECEIVED " .. hex(v.public_key) .. " aka " .. v.name)
      validators[v.public_key] = v.name
      new_validator = true
    end
  end
  if new_validator then
    gossip(peer_id, validators_msg())
    if table_length(validators) == TOTAL_VALIDATORS then
      for k, v in pairs(validators) do
        table.insert(validators_sorted, k)
      end
      table.sort(validators_sorted)
      --log("total_validators ", validators_sorted)
      for i,n in ipairs(validators_sorted) do
        log("validators", tostring(i) .. ": " .. hex(n) .. " a.k.a. " .. validators[n])
        if public_key == n then
          our_slot = i
          log("validators", tostring(i) .. " IS OUR SLOT!")
        end
      end
    end
  end
end
