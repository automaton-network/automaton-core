-- network.lua
networks = {}
ring_order = true

function random_topology()
  ring_order = false
end

function ring_order_topology()
  ring_order = true
end

-- NETWORK SIMULATION DISCOVERY

function sim_bind(i)
  return string.format("sim://100:10000:%d", i)
end

function sim_addr(i)
  return string.format("sim://1:20:10000:%d", i)
end

function simulation(node_factory, NODES, PEERS, path)
  networks[path] = {}
  networks[path]["nodes"] = {}
  networks[path]["graph"] = {}
  for i = 1, NODES do
    -- print(sim_bind(i))
    networks[path]["nodes"][i] = add_node(node_factory, i, networks[path]["graph"])
    networks[path]["nodes"][i]:listen(sim_bind(i))
  end

  for i = 1, NODES do
    for j = 1, PEERS do
      if ring_order then
        a = (i + j - 1) % NODES + 1
      else
        repeat
          a = math.random(NODES)
        until (a ~= i) and (r_peers[a] == nil)
        r_peers[a] = true
      end
      -- print(sim_addr(a))
      peer_id = add_peer(networks[path]["nodes"], i, sim_addr(a), a, networks[path]["graph"])
      networks[path]["nodes"][i]:connect(peer_id)
    end
  end
end

-- LOCALHOST DISCOVERY SETUP

math.randomseed(os.time())
START_PORT = 5000 + math.random(60000)

function tcp_addr(i)
  return "tcp://127.0.0.1:" .. tostring(START_PORT + i)
end

function localhost(node_factory, NODES, PEERS, path)
  -- create nodes and start listening
  networks[path] = {}
  networks[path]["nodes"] = {}
  networks[path]["graph"] = {}
  for i = 1, NODES do
    networks[path]["nodes"][i] = add_node(node_factory, i, networks[path]["graph"])
    networks[path]["nodes"][i]:listen(tcp_addr(i))
  end

  -- connect to peers
  for i = 1, NODES do
    local r_peers = {}
    for j = 1, PEERS do
      if ring_order then
        a = (i + j - 1) % NODES + 1
      else
        repeat
          a = math.random(NODES)
        until (a ~= i) and (r_peers[a] == nil)
        r_peers[a] = true
      end
      peer_id = add_peer(networks[path]["nodes"], i, tcp_addr(a), a, networks[path]["graph"])
      networks[path]["nodes"][i]:connect(peer_id)
    end
  end
end

-- MANUAL SETUP

manual_listeners = {}
remote_peers = {}

function set_listeners(address, s, e)
  manual_listeners = {}
  for port = s, e do
    table.insert(manual_listeners, string.format("tcp://%s:%d", address, port))
  end
end

function add_peers(address, s, e)
  for port = s, e do
    table.insert(remote_peers, string.format("tcp://%s:%d", address, port))
  end
end

function manual(node_factory, nn, PEERS, path)
  local NODES = #manual_listeners
  networks[path] = {}
  networks[path]["nodes"] = {}
  networks[path]["graph"] = {}
  -- create nodes and start listening
  for i = 1, #manual_listeners do
    networks[path]["nodes"][nIndex + i] = add_node(node_factory, nIndex + i, networks[path]["graph"])
    networks[path]["nodes"][nIndex + i]:listen(manual_listeners[i])
  end

  -- connect to peers
  for i = 1, NODES do
    local r_peers = {}
    for j = 1, PEERS do
      a = (i + j - 1) % NODES + 1
      --[[
      repeat
        a = math.random(#remote_peers)
      until r_peers[a] == nil
      r_peers[a] = true
      ]]

      peer_id = add_peer(networks[path]["nodes"], nIndex + i, remote_peers[a], a, networks[path]["graph"])
      networks[path]["nodes"][nIndex + i]:connect(peer_id)
    end
  end
end

function testnet(discovery, node_factory, num_nodes, num_peers, path)
  return discovery(node_factory, num_nodes, num_peers, path)
end

function add_node(node_factory, node_id, graph)
  local name = names[node_id]
  local new_node = node_factory(name)
  local s = string.format("{id: '%s', shape: 'box', label: '%s'}", node_id, name)
  if graph ~= nil then
    if graph["nodes"] == nil then
      graph["nodes"] = {}
    end
    table.insert(graph["nodes"], s)
  end
  return new_node
end

function add_peer(nodes, node_id, address, pid, graph)
  local peer_id = nodes[node_id]:add_peer(address)
  local s = string.format("{from: '%s', to: '%s', arrows:'to'}", node_id, pid)
  if graph ~= nil then
    if graph.edges == nil then
      graph.edges = {}
    end
    table.insert(graph.edges, s)
  end
  return peer_id
end

function set_lag(min_lag, max_lag)
  for i in pairs(nodes) do
    nodes[i]:call(
      "MIN_LAG=" .. tostring(min_lag) ..
      "; MAX_LAG=" .. tostring(max_lag))
  end
end

function get_nodes_from_testnet(testnet_id)
  if networks[testnet_id] ~= nil then
    return networks[testnet_id]["nodes"]
  end
  print("NO NODES @ PATH " .. testnet_id)
  return nil
end
