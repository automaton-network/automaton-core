-- consensus.lua
epoch = 0

-- Minimum number of transactions in a state transition
MIN_TRANSACTIONS = 1

pending_reservations = {}
pending_cancellations = {}

function broadcast(msg)
  gossip(0, msg)
end

function update_state(time)
  local modified = 0
  if not (is_table_empty(pending_reservations) and is_table_empty(pending_cancellations)) then
    if our_slot == (epoch % TOTAL_VALIDATORS + 1) then
      for i = 1, ROOM_COUNT do
        rooms_local[i] = {}
      end
      --epoch = epoch + 1;
      st = StateTransition()
      st.epoch = epoch+1;
      for _, r in pairs(pending_reservations) do
        if not conflicting_reservation(r) then
          modified = modified + 1
          st.reservations = r
          for _,v in pairs(r.room_id) do
            for i = r.start_day, r.end_day do
              rooms_local[v][i] = r.client_public_key
            end
          end
        end
      end

      for _, c in pairs(pending_cancellations) do
        if not conflicting_cancellation(c) then
          modified = modified + 1
          st.cancellations = c
          for _,v in pairs(c.room_id) do
            for i = c.start_day, c.end_day do
              rooms_local[v][i] = "cancelled"
            end
          end
        end
      end

      if modified >= MIN_TRANSACTIONS then
        -- Reset mempool
        pending_reservations = {}
        pending_cancellations = {}
        to_sign = st:serialize()
        st.signature = secp256k1_sign(private_key, to_sign)

        on_StateTransition(0, st)
      end
    end
  end
end

function conflicting_reservation(reservation)
  for _,v in pairs(reservation.room_id) do
    if rooms[v] == nil then
      return true
    end
    for i = reservation.start_day, reservation.end_day do
      if rooms[v][i] or rooms_local[v][i] then
        return true
      end
    end
  end
  return false
end

function conflicting_cancellation(cancellation)
  for _,v in pairs(cancellation.room_id) do
    if rooms[v] == nil then
      return true
    end
    for i = cancellation.start_day, cancellation.end_day do
      local key = cancellation.client_public_key
      if not ((rooms[v][i] == key and rooms_local[v][i] == nil)
              or (rooms_local[v][i] == key)) then
           return true
      end
      -- if local_rooms[v][i] == key then
      --   -- do nothing / continue
      -- elseif rooms[v][i] == key and canceled then
      --   return true
      -- end      

      -- if (local_rooms[v][i] != key and rooms[v][i] ~= cancellation.client_public_key)
      --   or rooms_local[v][i] == "cancelled" then
      --   return true
      -- end
    end
  end
  return false
end
