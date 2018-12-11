print ("==================================== BLOCKCHAIN DEMO ====================================")

MINERS = 100
nonce = {0}

function inc_nonce(n)
  for i = 1, #n do
    if n[i] < 255 then
      n[i] = n[i] + 1
      break
    else
      n[i] = 0
      if i == #n then
        table.insert(n, 1)
        return
      end
    end
  end
end

function nonce_str(n)
  s = {}
  for i = 1, #n do
    table.insert(s, string.char(n[i]))
  end
  return table.concat(s)
end

t = os.clock()

for i = 1, ITERATIONS do
  inc_nonce(nonce)
end
print(hex(nonce_str(nonce)))

t = os.clock() - t
print(string.format("nonce inc [%.3f M/s]", ITERATIONS / t / 1000000))

function miner_update()
  inc_nonce(nonce)
  for m = 1, MINERS do
    
  end
end

print ("=========================================================================================")
