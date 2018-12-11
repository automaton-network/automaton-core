current_message_id = 1
function connected(peer_id)
  peers[peer_id] = { status = "connected", name = "N/A" .. tostring(peer_id) }
  send(peer_id, validators_msg(), current_message_id)
  current_message_id = current_message_id + 1
end

function disconnected(peer_id)
  peers[peer_id] = nil
end

function gossip(peer_id, msg)
  for k,v in pairs(peers) do
    if k ~= 0 and k ~= peer_id and v.status == "connected" then
      send(k, msg, current_message_id)
      current_message_id = current_message_id + 1
    end
  end
end
