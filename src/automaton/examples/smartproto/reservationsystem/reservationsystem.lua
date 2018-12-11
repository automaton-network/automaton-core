-- reservationsystem.lua

-- create reservtion timer
cr_timer = math.random(100, 1000)

random_reservations = false

function update(time)
  if table_length(validators) == TOTAL_VALIDATORS then
    update_state(time)
  end

  cr_timer = cr_timer - 1
  if cr_timer <= 0 then
    if random_reservations then
      create_random_reservation()
    end
    cr_timer = math.random(200, 1000)
  end
end

function create_random_reservation()
  local start_day = math.random(1, DAYS)
  local end_day = math.min(DAYS, start_day + math.random(0, 4))
  tx = create_tx("create",
      { math.random(1, ROOM_COUNT) },
      start_day,
      end_day,
      public_key)
  on_CreateReservation(0, tx)
end

function reserve(room_id, start_day, end_day)
  tx = create_tx("create",
      { room_id },
      start_day,
      end_day,
      public_key)
  on_CreateReservation(0, tx)
end

function cancel(room_id, start_day, end_day)
  tx = create_tx("cancel",
      { room_id },
      start_day,
      end_day,
      public_key)
  on_CancelReservation(0, tx)
end

function sent(peer_id, msg_id, success)
end

function on_StateTransition(peer_id, state_transition)
  log("on_StateTransition", "SENT FROM: " .. peers[peer_id].name .. "\n"
    .. log_StateTransition(state_transition)) -- log the state transition
  if valid_transition(state_transition) then
    log("on_StateTransition", "VALID!")
    apply_transition(state_transition)
    gossip(peer_id, state_transition)
  else
    log("on_StateTransition", "INVALID!")
  end
end

function apply_transition(st)
  epoch = epoch + 1

  for _,r in ipairs(st.reservations) do
    for _,v in ipairs(r.room_id) do
      for i = r.start_day, r.end_day do
        rooms[v][i] = r.client_public_key
        pending_reservations[r.client_signature] = nil
      end
    end
  end

  for _,c in ipairs(st.cancellations) do
    for _,v in ipairs(c.room_id) do
      for i = c.start_day, c.end_day do
        rooms[v][i] = nil
        pending_cancellations[c.client_signature] = nil
      end
    end
  end

end

function valid_transition(st)
  if st.epoch ~= epoch + 1 then
    return false
  end
  for i = 1, ROOM_COUNT do
    rooms_local[i] = {}
  end
  for _,r in ipairs(st.reservations) do
    if not is_valid_signature(r, "create") then
      return false
    end
    if not conflicting_reservation(r) then
      for _,v in pairs(r.room_id) do
        for i = r.start_day, r.end_day do
          rooms_local[v][i] = r.client_public_key
        end
      end
    else
      return false
    end
  end

  for _,c in ipairs(st.cancellations) do
    if not is_valid_signature(c, "cancel") then
      return false
    end
    if not conflicting_cancellation(c) then
      for _,v in pairs(c.room_id) do
        for i = c.start_day, c.end_day do
          rooms_local[v][i] = nil
        end
      end
    else
      return false
    end
  end

  local signature = st.signature
  st.signature = ""
  local serialized = st:serialize()
  local state_valid = secp256k1_verify(validators_sorted[epoch % TOTAL_VALIDATORS + 1], serialized, signature)
  st.signature = signature
  return state_valid
end

function on_CreateReservation(peer_id, reservation)
  if pending_reservations[reservation.client_signature] or
      (not is_valid_signature(reservation, "create")) then
    return
  end
  for _,v in pairs(reservation.room_id) do
    if rooms[v] == nil then
      return
    end
    -- Check if there is conflicting reservation in pending_reservations
    -- for i = reservation.start_day, reservation.end_day do
    --   if rooms_local[v][i] then
    --     return
    --   end
    -- end
  end
  -- Mark pending reservation
  -- for _,v in pairs(reservation.room_id) do
  --   for i = reservation.start_day, reservation.end_day do
  --     rooms_local[v][i] = reservation.client_public_key
  --   end
  -- end

  log("on_CreateReservation", tostring(peer_id) .. reservation:to_json())
  pending_reservations[reservation.client_signature] = reservation
  gossip(peer_id, reservation)
end

function on_CancelReservation(peer_id, cancellation)
  if pending_cancellations[cancellation.client_signature] or
      (not is_valid_signature(cancellation, "cancel")) then
    return
  end
  for _,v in pairs(cancellation.room_id) do
    if rooms[v] == nil then
      return
    end
    -- for i = cancellation.start_day, cancellation.end_day do
    --   if rooms_local[v][i] ~= cancellation.client_public_key then
    --     return
    --   end
    -- end
  end
  -- Check if there is conflicting reservation in pending reservations
  -- for _,v in pairs(cancellation.room_id) do
  --   for i = cancellation.start_day, cancellation.end_day do
  --     rooms_local[v][i] = nil
  --   end
  -- end

  log("on_CancelReservation", tostring(peer_id) .. cancellation:to_json())
  pending_cancellations[cancellation.client_signature] = cancellation
  gossip(peer_id, cancellation)
end

function create_tx(tx_type, rooms, start_day, end_day, client_public_key)
  local msg = 0
  if tx_type == "create" then
    msg = CreateReservation()
  elseif tx_type == "cancel" then
    msg = CancelReservation()
  else
    return
  end

  for _,v in pairs(rooms) do
    msg.room_id = v
  end
  msg.start_day = start_day
  msg.end_day = end_day
  msg.client_public_key = client_public_key
  local to_sign = table.concat(rooms, "")
  to_sign = tx_type .. to_sign .. tostring(start_day) .. tostring(end_day)
  msg.client_signature = secp256k1_sign(private_key, to_sign)
  return msg
end
