-- core.lua

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
