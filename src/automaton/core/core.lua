-- core.lua

history_add("m = get_protocols({\"automaton/examples/smartproto/chat/\",\"automaton/examples/smartproto/blockchain/\",\"automaton/examples/smartproto/reservationsystem/\"})")
history_add("m = list_supported_protocols()")
history_add("msg = ProtocolsList()")
history_add("msg = ProtocolIDsList()")
history_add("msg:deserialize(m)")
history_add("print(msg:to_string())")

-- for test purposes message serialisation and deserialisation is included
function testprint(text)
  local m = PrintRequest()
  m.data = text
  local str = m:serialize()
  local m2 = PrintRequest()
  m2:deserialize(str)
  print(m2.data)
end
-- this function is an example only and is not used right now
function rpc_testprint(serialized_message)
  local m = PrintRequest()
  m:deserialize(serialized_message)
  testprint(m.data)
end

-- PROTOCOLS RPC --

function list_supported_protocols()
  local m = ProtocolIDsList()
  for k,_ in pairs(get_core_supported_protocols()) do
    m.protocol_ids = k
    print(k)
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

function list_running_protocols ()
  -- local response = ListProtocolsResponse()
  -- need to store and get info
  print("This function is not yet supported")
  -- return response:serialize()
end

function rpc_list_running_protocols ()
  return list_running_protocols()
end
-- function load_protocols ()

-- end
-- function rpc_load_protocols ()

-- end
