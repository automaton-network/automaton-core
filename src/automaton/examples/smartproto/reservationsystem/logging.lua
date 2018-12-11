-- logging.lua

function log_CreateReservation(r)
  return "  RESERVE ROOM(S) [" .. table.concat(r.room_id, ", ")
    .. "] DATES: " .. tostring(r.start_day) .. ".." .. tostring(r.end_day)
    .. " CLIENT: " .. hex(r.client_public_key)
    .. " SIG: " .. hex(r.client_signature)
end

function log_CancelReservation(r)
  return "  CANCEL ROOM(S) [" .. table.concat(r.room_id, ", ")
    .. "] DATES: " .. tostring(r.start_day) .. ".." .. tostring(r.end_day)
    .. " CLIENT: " .. hex(r.client_public_key)
    .. " SIG: " .. hex(r.client_signature)
end

function log_StateTransition(st)
  out = {}
  local reservations = st.reservations
  local cancellations = st.cancellations
  table.insert(out, "EPOCH: " .. tostring(st.epoch)
    .. " RESERVATIONS: " .. #reservations
    .. " CANCELLATIONS: " .. #cancellations)

  for _, r in pairs(reservations) do
    table.insert(out, log_CreateReservation(r))
  end

  for _, r in pairs(cancellations) do
    table.insert(out, log_CancelReservation(r))
  end

  return table.concat(out, "\n");
end
