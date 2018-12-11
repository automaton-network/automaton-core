-- Keeps track of the currently connected peers

peers = {}
peers[0] = { name = "ME!" }

function connected(peer_id)
  log("connections", "CONNECTED TO " .. tostring(peer_id))
  peers[peer_id] = { name = "N/A" }
  hi = Hello()
  hi.name = nodeid
  send(peer_id, hi, 1)
end

function disconnected(peer_id)
  log("connections", "DISCONNECTED FROM " .. tostring(peer_id))
  peers[peer_id] = nil
end

function on_Hello(peer_id, m)
  log("on_Hello", "Hello from " .. m.name)
  peers[peer_id].name = m.name
end
