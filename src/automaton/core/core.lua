-- core.lua

-- add rpc/core commands to cli history/hints
history_add("list_supported_protocols()")
hints_add("list_supported_protocols()")
history_add("list_running_protocols()")
hints_add("list_running_protocols()")
history_add("get_protocols()")
hints_add("get_protocols()")
history_add("load_protocol()")
hints_add("load_protocol()")

history_add("launch_node()")
hints_add("launch_node()")
history_add("list_nodes()")
hints_add("list_nodes()")
history_add("get_node()")
hints_add("get_node()")

history_add("add_peers()")
hints_add("add_peers()")
history_add("remove_peers()")
hints_add("remove_peers()")
history_add("list_known_peers()")
hints_add("list_known_peers()")
history_add("list_connected_peers()")
hints_add("list_connected_peers()")
history_add("get_peers()")
hints_add("get_peers()")
history_add("connect()")
hints_add("connect()")
history_add("disconnect()")
hints_add("disconnect()")
history_add("process_cmd()")
hints_add("process_cmd()")

history_add("testnet_create()")
hints_add("testnet_create()")
history_add("testnet_get_node_id()")
hints_add("testnet_get_node_id()")
history_add("testnet_destroy()")
hints_add("testnet_destroy()")

history_add("start_testnet()")
hints_add("start_testnet()")

history_add("history_add()")
hints_add("history_add()")
history_add("hints_add()")
hints_add("hints_add()")
history_add("hints_clear()")
hints_add("hints_clear()")

-- PROTOCOLS RPC --

function list_supported_protocols()
  local m = ProtocolIDsList()
  for k,_ in pairs(get_core_supported_protocols()) do
    m.protocol_ids = k
  end
  return m:serialize()
end

function rpc_list_supported_protocols()
  return list_supported_protocols()
end

function get_protocols(ids)
  local protocols = get_core_supported_protocols()
  local response = ProtocolsList()
  for _,id in ipairs(ids) do
    local p = protocols[id] -- p -> std::vector<std::pair<std::string, std::string> >
    if p ~= nil then
      local m = Protocol()
      m.protocol_id = id
      for k,v in pairs(p) do
        m.file_names = k
        m.files = v
      end
      response.protocols = m
    end
  end
  return response:serialize()
end

function rpc_get_protocols(m)
  local request = ProtocolIDsList()
  request:deserialize(m)
  return get_protocols(request.protocol_ids)
end

function list_running_protocols()
  -- local response = ListProtocolsResponse()
  -- need to store and get info
  print("This function is not yet supported")
  return ""
  -- return response:serialize()
end

function rpc_list_running_protocols()
  return list_running_protocols()
end

function load_protocols(protocol_list)
  for pid, path in pairs(protocol_list) do
    load_protocol(pid, path)
  end
  return ""
end

function rpc_load_protocols(m)
  local request = ProtocolsList()
  request:deserialize(m)
  local protocol_list = {}
  for _,p in ipairs(request.protocols) do
    protocol_list[p.protocol_id] = p.path
  end
  return load_protocols(protocol_list)
end

-- NODE RPC COMMON --

function rpc_launch_node(m)
  local msg = Node()
  msg:deserialize(m)
  return launch_node(msg.id, msg.protocol_id, msg.address)  -- implemented in core.cc
end

function list_nodes()
  local res = NodeIdsList()
  for _,id in ipairs(list_nodes_as_table()) do
    res.node_ids = id
  end
  return res:serialize()
end

function rpc_list_nodes()
  return list_nodes()
end

function get_nodes(node_ids)
  local response = NodesList()
  for _,id in ipairs(node_ids) do
    local node = get_node(id)
    if (node ~= nil) then
      local n = Node()
      n.id = node:get_id()
      n.protocol_id = node:get_protocol_id()
      n.address = node:get_address()
      print("node " .. n.id .. " -> " .. n.protocol_id .. " -> " .. n.address)
      response.nodes = n
    end
  end
  return response:serialize()
end

function rpc_get_nodes(m)
  local request = NodeIdsList()
  request:deserialize(m)
  return get_nodes(request.node_ids)
end

function remove_nodes(node_ids)

end

function rpc_remove_nodes(m)

end

-- NODE RPC --

function add_peers(node_id, peer_addresses)
  local node = get_node(node_id)
  if node == nil then
    return ""
  end
  local response = PeersList()
  response.node_id = node_id
  for _,addr in ipairs(peer_addresses) do
    local p = Peer()
    p.id = node:add_peer(addr)
    print("added " .. tostring(p.id))
    p.address = addr
    response.peers = p
  end
  return response:serialize()
end

function rpc_add_peers(m)
  local request = PeerAddressesList()
  request:deserialize(m)
  return add_peers(request.node_id, request.peer_addresses)
end

function remove_peers(node_id, peer_ids)
  local node = get_node(node_id)
  if node == nil then
    return ""
  end
  for _,id in ipairs(peer_ids) do
    node:remove_peer(id)
    print("removed " .. tostring(id))
  end
    return ""
end

function rpc_remove_peers(m)
  local request = PeerIdsList()
  request:deserialize(m)
  return remove_peers(request.node_id, request.peer_ids)
end

function list_known_peers(node_id)
  local node = get_node(node_id)
  if node == nil then
    return ""
  end
  local response = PeerIdsList()
  local list = node:known_peers()
  print("known peers to " .. node_id .. " ::")
  for _,id in ipairs(list) do
    response.peer_ids = id
    print(id)
  end
  return response:serialize()
end

function rpc_list_known_peers(m)
  local request = NodeId()
  request:deserialize(m)
  return list_known_peers(request.node_id)
end

function list_connected_peers(node_id)
  local node = get_node(node_id)
  if node == nil then
    return ""
  end
  local response = PeerIdsList()
  local list = node:peers()
  print("connected peers to " .. node_id .. " ::")
  for _,id in ipairs(list) do
    response.peer_ids = id
    print(id)
  end
  return response:serialize()
end

function rpc_list_connected_peers(m)
  local request = NodeId()
  request:deserialize(m)
  return list_connected_peers(request.node_id)
end

function get_peers(node_id, peer_ids)
  local node = get_node(node_id)
  if node == nil then
    return ""
  end
  local response = PeersList()
  response.node_id = node_id
  for _,id in ipairs(peer_ids) do
    local p = Peer()
    p.id = id
    p.address = node:get_peer_address(id)
    response.peers = p
  end
  return response:serialize()
end

function rpc_get_peers(m)
  local request = PeerIdsList()
  request:deserialize(m)
  return get_peers(request.node_id, request.peer_ids)
end

function connect(node_id, peer_ids)
  print("getting node " .. node_id)
  local node = get_node(node_id)
  if node == nil then
    return ""
  end
  for _,id in ipairs(peer_ids) do
    node:connect(id)
  end
  return ""
end

function rpc_connect(m)
  local request = PeerIdsList()
  request:deserialize(m)
  return connect(request.node_id, request.peer_ids)
end

function disconnect(node_id, peer_ids)
  local node = get_node(node_id)
  if node == nil then
    return ""
  end
  for _,id in ipairs(peer_ids) do
    node:disconnect(id)
  end
    return ""
end

function rpc_disconnect(m)
  local request = PeerIdsList()
  request:deserialize(m)
  return disconnect(request.node_id, request.peer_ids)
end

function process_cmd(node_id, cmd, params)
  local node = get_node(node_id)
  if node == nil then
    return ""
  end
  local response = NodeCmdResponse()
  response.response = node:process_cmd(cmd, params)
  return response:serialize()
end

function rpc_process_cmd(m)
  local request = NodeCmdRequest()
  request:deserialize(m)
  return process_cmd(request.node_id, request.cmd, request.params)
end

-- TESTNET --

function rpc_testnet_create(m)
  local request = TestNetCreate()
  request:deserialize(m)
  local peers_list = {}
  for _,v in pairs(request.topology) do
    peers_list[v.from_node] = v.to_node
  end
  testnet_create(request.testnet_id, request.protocol_id, request.network_type, request.number_nodes, peers_list)
  return ""
end

function rpc_testnet_destroy(m)
  local request = TestNetID()
  request:deserialize(m)
  testnet_destroy(request.testnet_id)
  return ""
end

function rpc_testnet_get_node_id(m)
  local request = TestNetGetNodeID()
  request:deserialize(m)
  local response = NodeID()
  response.node_id = get_testnet_node_id(request.testnet_id, request.node_index)
  return response:serialize()
end
-----------------

function rpc_start_testnet(m)
  local request = Network()
  request:deserialize(m)
  if (request.is_localhost) then
    testnet(localhost, _G[request.protocol_id], request.number_nodes, request.number_peers_per_node, request.logging_path)
  else
    testnet(simulation, _G[request.protocol_id], request.number_nodes, request.number_peers_per_node, request.logging_path)
  end
  return ""
end
