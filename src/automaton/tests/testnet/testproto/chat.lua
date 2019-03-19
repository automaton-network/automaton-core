heard_of = {}
math.randomseed(os.time())

function gossip(peer_id, msg)
  for k, v in pairs(peers) do
    if k ~= 0 and k ~= peer_id then
      send(k, msg, 0)
    end
  end
end

function on_Msg(peer_id, m)
  if heard_of[m.author] == nil then
    heard_of[m.author] = true
  end
  gossip(peer_id, m)
end

function update(timestamp)
  local m = Msg()
  m.author = nodeid
  on_Msg(0, m)
end

function get_peers()
  local response = Peers()
  for k,v in pairs(peers) do
    response:set_repeated_blob(1, v.name, -1)
  end
  return response:serialize()
end

function get_heard_of()
  local response = Peers()
  for k,_ in pairs(heard_of) do
    response:set_repeated_blob(1, k, -1)
  end
  return response:serialize()
end

function sent(peer_id, msg_id, success)
end
