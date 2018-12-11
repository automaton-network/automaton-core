-- blockchain init.lua
history_add("testnet(localhost, blockchain_node, 5, 1, \"logs/blockchain/\")")
history_add("start_mining(get_nodes(\"logs/blockchain/\"))")

function blockchain_node(id)
  local n = node(id, "automaton/examples/smartproto/blockchain/")

  -- print(id)
  _G[id] = {
    node_type = "blockchain",

    set_mining_power = function(x)
      n:script("MINE_ATTEMPTS=" .. x .. " return ''")
    end,

    get_mining_power = function()
      n:script("return tostring(MINE_ATTEMPTS)");
    end,

    get_stats = function()
      return n:script("return node_stats()")
    end,

    disconnect_all = function()
      n:call("disconnect_all()")
    end,

    connect = function(peer_id)
      n:call("connect("..tostring(peer_id)..")")
    end,
  }

  return n
end

-- test network functions

function set_mining_power(n, nodes)
  if nodes == nil then
    return
  end
  for i in pairs(nodes) do
    nodes[i]:call("MINE_ATTEMPTS=" .. tostring(n))
  end
end

function get_mining_power(nodes)
  if nodes == nil then
    return
  end
  s = {}
  for i in pairs(nodes) do
    pwr = nodes[i]:script("return tostring(MINE_ATTEMPTS)")
    table.insert(s, pwr)
  end
  print(table.concat(s, ", "))
end

function start_mining(nodes)
  set_mining_power(10, nodes)
end
