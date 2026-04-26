-- Table library (table.*)

-- table.insert: append
local t = {1, 2, 3}
table.insert(t, 4)
print(#t)      -- 4
print(t[4])    -- 4

-- table.insert: at position
table.insert(t, 2, 99)
print(#t)      -- 5
print(t[1])    -- 1
print(t[2])    -- 99
print(t[3])    -- 2

-- table.remove: from end
local t2 = {10, 20, 30, 40}
local removed = table.remove(t2)
print(removed)   -- 40
print(#t2)       -- 3

-- table.remove: at position
local t3 = {10, 20, 30, 40}
local removed2 = table.remove(t3, 2)
print(removed2)  -- 20
print(t3[1])     -- 10
print(t3[2])     -- 30
print(t3[3])     -- 40
print(#t3)       -- 3

-- table.concat
local t4 = {"hello", "world", "foo"}
print(table.concat(t4))          -- helloworldfoo
print(table.concat(t4, ", "))    -- hello, world, foo
print(table.concat(t4, "-", 2))  -- world-foo
print(table.concat(t4, "|", 1, 2))  -- hello|world

-- table.sort
local t5 = {5, 3, 1, 4, 2}
table.sort(t5)
for i = 1, #t5 do print(t5[i]) end
-- 1 2 3 4 5

-- table.sort with comparator
local t6 = {5, 3, 1, 4, 2}
table.sort(t6, function(a, b) return a > b end)
for i = 1, #t6 do print(t6[i]) end
-- 5 4 3 2 1

-- table.sort: strings
local t7 = {"banana", "apple", "cherry", "date"}
table.sort(t7)
for i = 1, #t7 do print(t7[i]) end
-- apple banana cherry date

-- table.move (Lua 5.3)
local src = {1, 2, 3, 4, 5}
local dst = {10, 20, 30}
table.move(src, 2, 4, 2, dst)
print(dst[1])   -- 10
print(dst[2])   -- 2
print(dst[3])   -- 3
print(dst[4])   -- 4

-- table.unpack (was unpack in Lua 5.1)
local t8 = {10, 20, 30, 40, 50}
print(table.unpack(t8))           -- 10  20  30  40  50
print(table.unpack(t8, 2, 4))    -- 20  30  40

-- table.pack (Lua 5.2+)
local packed = table.pack(10, 20, 30)
print(packed.n)     -- 3
print(packed[1])    -- 10
print(packed[2])    -- 20
print(packed[3])    -- 30
