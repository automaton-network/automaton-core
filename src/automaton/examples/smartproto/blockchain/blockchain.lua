-- blockchain.lua

-- Valid block is a block that:
-- 1. Is a new block, with valid hash and height >= 1
-- 2. Either prev_hash is in blocks or the block is with height #1 and prev_hash is GENESIS_HASH
-- 3. Has height difference of one with the blocks[prev_hash].height
--    or it is block#1 with prev_hash equal to GENESIS_HASH
function validate(block)
  if block == nil then
    log("validate", "validate called with block=nil")
    return BLOCK.INVALID
  end
  -- get the block hash and target difficulty
  local hash = block_hash(block)
  local target = get_target()
  -- Check if we already have the block
  if blocks[hash] ~= nil then
    log_block("validate", block, "DUPLICATE")
    return BLOCK.DUPLICATE
  -- Check difficulty
  elseif hash > target then
    log_block("validate", block, "INVALID target > hash")
    return BLOCK.INVALID
  -- Check block height is a positive integer
  elseif block.height < 1 then
    log_block("validate", block, "INVALID height < 1")
    return BLOCK.INVALID
  -- The block should have its predecessor in blocks unless it is the first block
  elseif block.prev_hash ~= GENESIS_HASH and blocks[block.prev_hash] == nil then
    log_block("validate", block, "NO_PARENT")
    return BLOCK.NO_PARENT
  -- 1. If this is the first block, it needs to have GENESIS_HASH as prev_hash.
  -- 2. If it is not the first block, check if the height of
  --    the block with hash prev_hash is the height of this
  elseif block.height == 1 and block.prev_hash ~= GENESIS_HASH
      or block.height > 1 and blocks[block.prev_hash].height ~= block.height-1 then
    log_block("validate", block, "INVALID height")
    return BLOCK.INVALID
  else
    log_block("validate", block, "VALID")
    return BLOCK.VALID
  end
end

function on_Block(peer_id, block)
  -- Validate, save and broadcast
  local block_validity = validate(block)
  local hash = block_hash(block)
  log(pid(peer_id), "RECV | " .. hex(hash))
  if block_validity == BLOCK.VALID  then
    -- Block is valid, store it
    blocks[hash] = {
      miner = block.miner,
      prev_hash = block.prev_hash,
      height = block.height,
      nonce = block.nonce
    }
    -- Check if we get a longer chain. Does not matter if it is the main or alternative.
    if block.height == #blockchain+1 then
      -- We are sure that this is the head of the longest chain.
      blockchain[#blockchain+1] = hash
      -- Check if blocks[block.prev_hash] is part of the main chain and replace if necessary.
      local block_index = (#blockchain)-1
      local longest_chain_hash = block.prev_hash
      while block_index >= 1 and (blockchain[block_index] ~= longest_chain_hash) do
        blockchain[block_index] = longest_chain_hash
        longest_chain_hash = blocks[longest_chain_hash].prev_hash
        block_index = block_index - 1
      end
      gossip(peer_id, block_index + 1)
    end
  end
end
