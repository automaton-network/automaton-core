peers = {}

function connected(peer_id)
  peers[peer_id] = { name = "N/A" }
  hi = Hello()
  hi.name = nodeid
  send(peer_id, hi, 1)
end

function disconnected(peer_id)
  peers[peer_id] = nil
end

function on_Hello(peer_id, m)
  peers[peer_id].name = m.name
end
