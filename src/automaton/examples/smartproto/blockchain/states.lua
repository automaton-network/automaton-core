-- states
-- STATE = {}
-- STATE.HANDSHAKE = 1
-- STATE.BEHIND = 2
-- STATE.AHEAD = 3
-- STATE.DISCONECTED = 4
-- STATE.NOT_CONNECTED = 5
-- STATE.IN_CONSENSUS = 6

-- block validation results
BLOCK = {}
BLOCK.VALID = 1
BLOCK.INVALID = 2
BLOCK.DUPLICATE = 3
BLOCK.NO_PARENT = 4

GENESIS_HASH = sha3("automaton")

peers = {}
blocks = {}
blockchain = {}
balances = {}

function node_stats()
  local hashes = {}
  for k,b in pairs(blockchain) do
    table.insert(hashes, hex(b))
  end
  return table.concat(hashes, "\n")
end

function collect_balances()
  balances = {}
  for i = 1, #blockchain do
    local block = get_block(blockchain[i])
    balances[block.miner] = (balances[block.miner] or 0 ) + 1
  end
end

function cur_hash()
  return blockchain[#blockchain] or GENESIS_HASH
end

function get_block(hash)
  if blocks[hash] ~= nil then
    return blocks[hash]
  elseif hash == GENESIS_HASH then
    return {
      height = 0,
      miner = "",
      prev_hash = "",
      nonce = ""
    }
  end
  return
end

function log_block(identifer, block, extra)
  log(identifer,
    string.format("%s | %4d | %12s | %s",
      hex(block_hash(block)),
      block.height,
      block.miner,
      extra))
end
