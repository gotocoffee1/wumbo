-- Coroutines (coroutine.*)

-- Basic coroutine: create and resume
local co = coroutine.create(function(a, b)
    print("start", a, b)   -- start  10  20
    local c = coroutine.yield(a + b)
    print("resumed", c)    -- resumed  100
    return "done"
end)

print(coroutine.status(co))   -- suspended

local ok, val = coroutine.resume(co, 10, 20)
print(ok, val)   -- true  30

print(coroutine.status(co))   -- suspended

local ok2, val2 = coroutine.resume(co, 100)
print(ok2, val2)   -- true  done

print(coroutine.status(co))   -- dead

-- Resuming a dead coroutine returns false
local ok3, err3 = coroutine.resume(co)
print(ok3)   -- false

-- coroutine.wrap: simpler interface
local gen = coroutine.wrap(function()
    for i = 1, 3 do
        coroutine.yield(i)
    end
end)

print(gen())   -- 1
print(gen())   -- 2
print(gen())   -- 3

-- Producer-consumer pattern
local function producer()
    local items = {"a", "b", "c"}
    for i = 1, #items do
        coroutine.yield(items[i])
    end
end

local co2 = coroutine.create(producer)
local running = true
while running do
    local ok, val = coroutine.resume(co2)
    if ok and val ~= nil then
        print(val)
    else
        running = false
    end
end
-- a  b  c

-- Passing values back into coroutine via resume
local co3 = coroutine.create(function()
    local x = coroutine.yield(1)
    local y = coroutine.yield(2)
    return x + y
end)

local _, v1 = coroutine.resume(co3)
print(v1)   -- 1
local _, v2 = coroutine.resume(co3, 10)
print(v2)   -- 2
local _, v3 = coroutine.resume(co3, 20)
print(v3)   -- 30

-- Error handling in coroutines
local co4 = coroutine.create(function()
    error("coroutine error", 0)
end)
local ok4, err4 = coroutine.resume(co4)
print(ok4)    -- false
print(err4)   -- coroutine error

-- coroutine.running
local main_co, is_main = coroutine.running()
print(is_main)   -- true  (we are in the main thread)

local co5 = coroutine.create(function()
    local running_co, is_main2 = coroutine.running()
    print(is_main2)   -- false
    print(running_co ~= nil)  -- true
end)
coroutine.resume(co5)

-- coroutine.isyieldable
print(coroutine.isyieldable())   -- false  (main thread not yieldable)
local co6 = coroutine.create(function()
    print(coroutine.isyieldable())   -- true
end)
coroutine.resume(co6)
