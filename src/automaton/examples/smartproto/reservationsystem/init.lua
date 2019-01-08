-- reservation system init.lua
history_add("testnet(localhost, reservation_system_node, 5, 1, \"logs/hotel/\")")
history_add("start_random_reservations(get_nodes_from_testnet(\"logs/hotel/\"))")
history_add("stop_random_reservations(get_nodes_from_testnet(\"logs/hotel/\"))")

function reservation_system_node(id)
  local n = node(id, "automaton/examples/smartproto/reservationsystem/")

  -- print(id)
  _G[id] = {
    node_type = "reservation_system_node",

    connect = function(peer_id)
      n:call("connect("..tostring(peer_id)..")")
    end,

    reserve = function(room_id, start_day, end_day)
      n:call("reserve(" .. tostring(room_id) .. ","
        .. tostring(start_day) .. ","
        .. tostring(end_day) .. ")")
    end,

    cancel = function(room_id, start_day, end_day)
      n:call("cancel(" .. tostring(room_id) .. ","
        .. tostring(start_day) .. ","
        .. tostring(end_day) .. ")")
    end
  }
  return n
end

-- test network functions

function start_random_reservations(nodes)
  if nodes == nil then
    return
  end
  for i in pairs(nodes) do
    nodes[i]:call("random_reservations = true")
  end
end

function stop_random_reservations(nodes)
  if nodes == nil then
    return
  end
  for i in pairs(nodes) do
    nodes[i]:call("random_reservations = false")
  end
end
