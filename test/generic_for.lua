-- Generic for loop (for k, v in iter, state, init do)
-- Tests the generic for loop construct with custom iterators

-- Stateless iterator: iterates array indices 1..n
local function array_iter(t, i)
    i = i + 1
    local v = t[i]
    if v ~= nil then
        return i, v
    end
end

local fruits = {"apple", "banana", "cherry"}
for i, v in array_iter, fruits, 0 do
    print(i, v)
end
-- 1   apple
-- 2   banana
-- 3   cherry

-- Iterator that counts down
local function countdown(from, current)
    if current <= 0 then return nil end
    return current - 1, current
end

for state, val in countdown, 5, 5 do
    print(val)
end
-- 5 4 3 2 1

-- Custom stateful iterator using closures
local function range(n)
    local i = 0
    return function()
        i = i + 1
        if i <= n then return i end
    end
end

for v in range(5) do
    print(v)
end
-- 1 2 3 4 5

-- Multiple values from iterator
local function entries(t, keys)
    local i = 0
    return function()
        i = i + 1
        local k = keys[i]
        if k ~= nil then
            return k, t[k]
        end
    end
end

local data = {x = 10, y = 20, z = 30}
local key_order = {"x", "y", "z"}
for k, v in entries(data, key_order) do
    print(k, v)
end
-- x   10
-- y   20
-- z   30

-- break in generic for
local found = nil
for i, v in array_iter, {"a", "b", "c", "d"}, 0 do
    if v == "c" then
        found = i
        break
    end
end
print(found)   -- 3

-- Nested generic for loops
local matrix = {{1,2},{3,4},{5,6}}
for i, row in array_iter, matrix, 0 do
    for j, val in array_iter, row, 0 do
        print(i, j, val)
    end
end
-- 1   1   1
-- 1   2   2
-- 2   1   3
-- 2   2   4
-- 3   1   5
-- 3   2   6
