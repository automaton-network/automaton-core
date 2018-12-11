Node/ Protocol set up:
  * As a beginning everyone is a FULL node. Difficulty doesn't change. New block will consist of
  hash, hash of the previous block, height, the address of the peer that found it and nonce.
  Everybody is mining and when a new hash is found (this could also be rpc call: process(*mine new
    block*)) it sends it to all other peers.
  * There is no disconnect and storage of state for use when connect again.
  * There is no peer discovery. A new peer joins the network by connecting to peers from a list.
  * A state storing <hash, block> is used by every peer to represent the blockchain.

Block validation and state synchronization:
 1. Check if hash satisfies difficulty. If it doesn't, ignore it.
 2. Check if hash is in the tree, if it is, ignore it.
 3. Check if prev hash is present in the chain/ tree.
  1. If prev hash is the top, add the new block to the top. See if there are orphan blocks that are
  children to this one. Do recursively.
  2. If prev hash is in the tree but not the top, split. See if there are orphan blocks that are
  children to this one. Do recursively.
  3. If prev hash is not in the tree, add the new block to orphan blocks set and request prev
  hash block
 4. Send the top of the chain.

Implementation:
  **No block deletion**
  1. Peer finds a new hash.
  2. Creates a block, containing the hash, the hash of its previous chain top, the new height and
  his id (for now the id of the first acceptor). Creates a *block* message.
  3. Adds the new block to the global state -> key is block hash, value is the serialized message.
  4. Change its chain top to point to the new block
  5. Sends the new block to everybody in a message containing the block hash and the serialized
  *block* message.
  6. Peer receives the message and checks if he has the block hash in his blockchain version or in
  his set of orphan blocks which means it is an already seen/ received block. If it is, ignores it.
  7. Validate that the hash of this block is the received hash.
  9. Checks if the previous block is in the tree. If it is not, inserts the new block into the
  set of orphan blocks and sends a block request for its previous block.
  10. (If the previous block **is** in the tree) Checks if the previous block height + 1 = new
  block height. Adds it to the tree and checks if it is the new chain top. Checks if there is an
  orphan block that could be added to the tree. When there are no more orphan blocks that could be
  added, sends the new chain top.


Main functions:
  * void mine(const std::string& hash) -> It acts like it has mined a new hash. // ...
  * on_message_received
  * handle_block
  * check_orphans
  * create_send_blocks_message
  * create_request_blocks_message

```
syntax = "proto3";

message blocks_request {
/*
  Peer A send this when requesting blocks to show what is its top of the chain so if he is many
  blocks behind peer B will send him more blocks. If connects for the first time, this will be zero.
*/
  bytes top_block_hash = 1;

  // Peer A sends the hash/es of the block/s he requests from B
  repeated bytes block_hash_requested = 2;
}

message block {
  bytes hash = 1;
  bytes prev_hash = 2;
  uint32 height = 3;
  bytes discovered_by = 4;
  bytes nonce = 5;
}

message blocks_response {
  // Peer B sets the hash/es of the block/s he sends to A
  repeated bytes block_hash = 1;
   // Peer B sends the whole block/s (serialized block messages) to A
  repeated bytes whole_block = 2;
}

message data {
  blocks_request msg_type_1 = 1;
  blocks_response msg_type_2 = 2;
}
```
