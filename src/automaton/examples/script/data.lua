b = BlockHeader()
blocks = Blocks()

print(getmetatable(b))
print(getmetatable(blocks))

b = nil
collectgarbage()

b = BlockHeader()
b:set_blob(1, "test\0\1")
b:set_bool(4, false)
b:set_enum(5, 2)

print(b:get_enum(5))
print("B: " .. b:to_json())

bin = b:serialize()
print("SERIALIZED: " .. hex(bin))

b2 = BlockHeader()
b2:deserialize(bin)
print("B2: " .. b2:to_json())

-- blocks = Blocks()
