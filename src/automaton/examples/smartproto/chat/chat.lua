-- chat.lua

-- initialize protocol
msgs = {}
msg_index = 0
global_seq = 0
math.randomseed(os.time())

-- gossip - sends to everyone but the specified peer_id
function gossip(peer_id, msg)
  for k, v in pairs(peers) do
    if k ~= 0 and k ~= peer_id then
      log("sending", peers[peer_id].name .. " -> " .. msg.msg)
      global_seq = global_seq + 1
      send(k, msg, global_seq)
    end
  end
end

function sent(peer_id, msg_id, success)
  -- log("sent",
  --   string.format("Sent to %s (%d), success: %s", peers[peer_id].name, msg_id, tostring(success))
  -- )
end

function on_Msg(peer_id, m)
  local hash = hex(sha3(m.author .. m.msg))
  local msg = string.format("<%s>: %s [FROM %s] [%s]",
      m.author, m.msg, peers[peer_id].name, hash)
  if msgs[hash] == nil then
    msgs[hash] = m.msg
    log("CHAT", msg)
    gossip(peer_id, m)
  else
    log("IGNORED", "[Already received] " .. msg)
  end
end

-- wait a bit between generating and sending chat messages
wait = math.random(200,1000)
function update(timestamp)
  wait = wait - 1
  if wait <= 0 then
    wait = math.random(200,1000)
    local m = Msg()
    msg_index = msg_index + 1
    local idx = ((msg_index - 1) % #msg_contents) + 1

    m.sequence = msg_index
    m.author = nodeid
    m.msg = msg_contents[idx] .. " (" .. tostring(msg_index) .. ")"
    on_Msg(0, m)
  end
end

function get_peers()
  local response = Peers()
  for k,v in pairs(peers) do
    response:set_repeated_blob(1, v.name, -1)
  end
  return response:serialize()
end

function get_messages()
  local response = Messages()
  for k,v in pairs(msgs) do
    local m = ReceivedMsg()
    m.hash = k
    m.data = v
    response:set_repeated_msg(1, m, -1)
  end
  return response:serialize()
end
