ZERO_BYTES = 3
DIFFICULTY = 500000

-- Smart protocol logic

Block = {}
Block_mt = { __index = Block }

function Block:create()
  local o = {}
  setmetatable(o, Block_mt)
  return o
end

Peer = {}
Peer_mt = { __index = Peer }

function Peer:create()
  local o = {}
  setmetatable(o, Peer_mt)
  return o
end

Node = {}
Node_mt = { __index = Node }

function Node:create()
  local o = {
    state = {},
    peers = {},
    pk = rand(32),
  }

  print("Node created " .. hex(o.pk))
  
  setmetatable(o, Node_mt)
  return o
end

nodes = {}

NODES = 1

for i = 1,NODES do
  nodes[i] = Node.create()
end

function f(x, i)
  return x ^ i
end
