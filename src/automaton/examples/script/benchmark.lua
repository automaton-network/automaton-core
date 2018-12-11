ITERATIONS = 10000

function benchmark_msg()
  t = os.clock()
  s = 0
  for i = 1,ITERATIONS do
    s = add(s,i)
  end
  t = os.clock() - t
  print(string.format("add [%.3f M/s]", ITERATIONS / t / 1000000))
end

benchmark_msg()

function benchmark_msg2()
  t = os.clock()
  b = BlockHeader()
  for i = 1,ITERATIONS do
    rand(32)
  end
  print(string.format("rand(32) [%.3f M/s]", ITERATIONS / t / 1000000))

  t = os.clock()
  b = BlockHeader()
  for i = 1,ITERATIONS do
    b.hash = rand(32)
  end
  print(string.format("b.hash = rand(32) [%.3f M/s]", ITERATIONS / t / 1000000))

  t = os.clock()
  b = BlockHeader()
  for i = 1,ITERATIONS do
    b.hash = "ABC"
  end
  print(string.format("b.hash = 'abc' [%.3f M/s]", ITERATIONS / t / 1000000))

  t = os.clock()
  b = BlockHeader()
  s = "abc"
  for i = 1,ITERATIONS do
    b:set_blob(2,s)
  end
  print(string.format("b.set_blob(2,s) [%.3f M/s]", ITERATIONS / t / 1000000))

  t = os.clock()
  for i = 1,ITERATIONS do s = b.hash end
  print(string.format("s = b.hash [%.3f M/s]", ITERATIONS / t / 1000000))

  b = nil
  collectgarbage()
end

benchmark_msg2()

hashes = {
  {"SHA2-256", sha256},
  {"SHA2-512", sha512},
  {"Keccak-256", keccak256},
  {"SHA3-256", sha3},
  {"RIPEMD-160", ripemd160},
}

function benchmark_hash(hash)
  name = hash[1]
  f = hash[2]
  t = os.clock()

  s = ""
  for i = 1, ITERATIONS do
    s = f(s)
  end

  t = os.clock() - t
  return ITERATIONS / t / 1000000
end

function benchmark_batch(name)
  t = os.clock()

  fromcpp(name, ITERATIONS)

  t = os.clock() - t
  return ITERATIONS / t / 1000000
end

print("=== BENCHMARK HASH FUNCTIONS ===")

for i = 1, #hashes do
  speed1 = benchmark_hash(hashes[i])
  speed2 = benchmark_batch(hashes[i][1])
  print(string.format("%-12s [Lua %6.3f Mh/s] [C++ %6.3f Mh/s - %.1fx faster]",
        name, speed1, speed2, speed2 / speed1))
end

print("================================")
print("")
