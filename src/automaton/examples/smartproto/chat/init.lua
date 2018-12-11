-- chat init.lua
history_add("testnet(localhost, chat_node, 5, 1, \"logs/chat/\")")
history_add("nodes = get_nodes(\"logs/chat/\")")
history_add("p = nodes[2]:process_cmd(\"get_peers\", \"\")")
history_add("m = nodes[2]:process_cmd(\"get_messages\", \"\")")

function chat_node(id)
  local n = node(id, "automaton/examples/smartproto/chat/")

  _G[id] = {
    node_type = "chat",
  }

  return n
end
