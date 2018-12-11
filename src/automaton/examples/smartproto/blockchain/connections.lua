-- connections.lua

-- Keeps track of the currently connected peers

conn = {}
conn[0] = { name="ME!" }

function pid(peer_id)
  if conn[peer_id] ~= nil then
    return string.format("CONN%d[%s]", peer_id, conn[peer_id].name)
  end
  return "N/A"
end

function peer_connected(peer_id)
  -- Send all current blocks, so that we have consensus
  send_blocks(peer_id, 1)
end

function connected(peer_id)
  log("connections", "CONNECTED TO " .. tostring(peer_id))
  conn[peer_id] = { name = "N/A" }
  hi = Hello()
  hi.name = nodeid
  send(peer_id, hi, 1)
end

function disconnected(peer_id)
  log("connections", "DISCONNECTED FROM " .. tostring(peer_id))
  conn[peer_id] = nil
end

function disconnect_all()
  for k, v in pairs(conn) do
    disconnect(k)
  end
end

function sent(peer_id, msg_id)
end

MIN_LAG = 0
MAX_LAG = 0
lag = 0
d_blocks = {}
current_message_id = 0

function process_delayed_blocks()
  if lag > 0 then
    -- log("lag", "waiting... LAG:" .. lag .. " BLOCKS: " .. #d_blocks)
    lag = lag - 1
    return
  end
  lag = math.random(MIN_LAG, MAX_LAG)
  for k,v in pairs(d_blocks) do
    local block = v.b
    -- Convert block to Block protocol message and send it
    local block_msg = Block()
    block_msg.miner = block.miner
    block_msg.prev_hash = block.prev_hash
    block_msg.height = block.height
    block_msg.nonce = block.nonce
    current_message_id = current_message_id + 1
    send(v.pid, block_msg, current_message_id)
  end
  d_blocks = {}
end

function send_block(peer_id, hash)
  log(pid(peer_id), "SEND | " .. hex(hash))

  -- Get block by hash and check if it's valid
  local block = get_block(hash)
  if block == nil then
    log(pid(peer_id), "Trying to send invalid block with hash " .. hex(hash))
    return
  end

  table.insert(d_blocks, {pid = peer_id, d = math.random(MIN_LAG, MAX_LAG), b = block})
  -- process_delayed_blocks()
end

function send_blocks(peer_id, start_block)
  log(pid(peer_id), "Sending blocks " .. start_block .. ".." .. #blockchain)
  for i = start_block, #blockchain do
    send_block(peer_id, blockchain[i])
  end
end

function gossip(from, start_block)
  for k, v in pairs(conn) do
    if (k ~= from) and (k ~= 0) then
      send_blocks(k, start_block)
    end
  end
end

function on_Hello(peer_id, m)
  log("HELLO", "Hello from peer " .. tostring(peer_id) .. " name: " .. m.name)
  conn[peer_id].name = m.name
  peer_connected(peer_id)
end
