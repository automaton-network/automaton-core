-- states.lua

ROOM_COUNT = 4
DAYS = 30
TOTAL_VALIDATORS = 20

rooms = {}
-- room_local is representation of the reserved rooms in the mempool
-- not including the reserved rooms in the state
rooms_local = {}
mempool = {}
peers = {}
peers[0] = { status = "disconnected", name = "Us" }
msg_hash = {}

for i = 1, ROOM_COUNT do
  rooms[i] = {}
  rooms_local[i] = {}
end

-- Check if reservation or cancelation TX has a valid signature
function is_valid_signature(tx, type)
  local msg = table.concat(tx.room_id, "")
  msg = type .. msg .. tostring(tx.start_day) .. tostring(tx.end_day)
  return secp256k1_verify(tx.client_public_key, msg, tx.client_signature)
end

function table_length(t)
  local count = 0
  for _ in pairs(t) do
    count = count + 1
  end
  return count
end

function is_table_empty(t)
  return next(t) == nil
end

function table_to_json(t)
  -- TODO: ints should not be saved as strings
  local json = "{"
  for k,v in pairs(t) do
    json = json .. '"' .. k .. '"' .. ":"
    if type(v) == "table" then
      json = json .. table_to_json(v) .. ","
    else
      json = json .. '"' .. v .. '",'
    end
  end
  json = json:sub(1, -2)
  json = json .. "}"
  return json
end
