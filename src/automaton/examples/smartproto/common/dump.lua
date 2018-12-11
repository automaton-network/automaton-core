-- dump.lua

function dump_logs(path)
  if networks[path] == nil then
    print("NO SUCH PATH in NETWORKS " .. path)
    return
  end
  dump_connections_graph(path)
  for i in pairs(networks[path]["nodes"]) do
    networks[path]["nodes"][i]:dump_logs(string.format("%sN%03d-%s.html", path, i, names[i]))
  end
  collect_states(path)
end

function dump_connections_graph(path)
  if networks[path]["graph"] == nil then
    return
  end
  local file = io.open(path .. "connections_graph.html", "w+")
  if file == nil then
    print("Could not open " .. path .. "connections_graph.html")
  end
  file:write(create_graph_html(networks[path]["graph"]))
  file:close()
end
