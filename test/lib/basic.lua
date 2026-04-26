-- Basic library functions not covered by other test files.
-- Excluded: assert, error, pcall, xpcall  → test/pcall_error.lua
--           print                          → used throughout all tests
--           setmetatable, getmetatable     → test/metatables.lua, test/oop.lua
--           tonumber, tostring, type       → test/type_coercion.lua

-- _VERSION
print(_VERSION)   -- Lua 5.3

-- _G is the global environment table
print(type(_G))                -- table
print(_G.print == print)       -- true
print(_G.type == type)         -- true
print(_G.tostring == tostring) -- true

-- collectgarbage: no-op in this implementation, must not error
collectgarbage()
collectgarbage("collect")
collectgarbage("count")
print("gc ok")   -- gc ok

-- select('#', ...): returns total count of arguments
local function count(...)
    return select('#', ...)
end
print(count())            -- 0
print(count(1))           -- 1
print(count(1, 2, 3))    -- 3
print(count(nil, nil))   -- 2  (nil args still count)

-- select(n, ...): returns all args from position n onward
local function from(n, ...)
    return select(n, ...)
end
print(from(1, 10, 20, 30))   -- 10  20  30
print(from(2, 10, 20, 30))   -- 20  30
print(from(3, 10, 20, 30))   -- 30

-- select with negative index: from end
print(select(-1, 10, 20, 30))   -- 30  (last one)
print(select(-2, 10, 20, 30))   -- 20  30

-- ipairs: iterates the sequence part of a table (1..n)
local arr = {10, 20, 30, 40, 50}
for i, v in ipairs(arr) do
    print(i, v)
end
-- 1   10
-- 2   20
-- 3   30
-- 4   40
-- 5   50

-- ipairs stops at the first nil hole
local sparse = {1, 2, nil, 4}
local count2 = 0
for i, v in ipairs(sparse) do
    count2 = count2 + 1
end
print(count2)   -- 2  (stops before the nil)

-- ipairs on empty table
local empty_count = 0
for i, v in ipairs({}) do
    empty_count = empty_count + 1
end
print(empty_count)   -- 0

-- pairs: iterates all key-value pairs (sequence tables have defined order)
local seq = {10, 20, 30}
for i, v in pairs(seq) do
    print(i, v)
end
-- 1   10
-- 2   20
-- 3   30

-- pairs visits every key; count entries in a mixed table
local mixed = {a = 1, b = 2, c = 3}
local n = 0
for k, v in pairs(mixed) do
    n = n + 1
end
print(n)   -- 3

-- pairs on empty table
local n2 = 0
for k, v in pairs({}) do
    n2 = n2 + 1
end
print(n2)   -- 0

-- next: advances to the next key in a table
-- next(t, nil) returns some first key-value pair (or nil for empty table)
local t1 = {}
print(next(t1))   -- nil  (empty table)

local t2 = {100}
local k, v = next(t2, nil)
print(k, v)   -- 1   100
-- next after the last key returns nil
print(next(t2, k))   -- nil

-- rawequal: equality without __eq metamethod
local mt = {__eq = function() return true end}
local a = setmetatable({}, mt)
local b = setmetatable({}, mt)
print(a == b)          -- true   (uses __eq)
print(rawequal(a, b))  -- false  (different tables)
print(rawequal(a, a))  -- true   (same reference)
print(rawequal(1, 1))  -- true
print(rawequal(1, 1.0))-- false  (different types: integer vs float)
print(rawequal("x","x")) -- true

-- rawget: read a field bypassing __index
local proxy = {}
local mt2 = {__index = function(t, k) return 999 end}
local obj = setmetatable(proxy, mt2)
obj.real = 42
print(obj.real)           -- 42   (raw value)
print(obj.missing)        -- 999  (via __index)
print(rawget(obj, "real"))    -- 42   (bypasses __index, key exists)
print(rawget(obj, "missing")) -- nil  (bypasses __index)

-- rawset: write a field bypassing __newindex
local log = {}
local mt3 = {__newindex = function(t, k, v) log[k] = v end}
local guarded = setmetatable({}, mt3)
guarded.x = 1        -- goes through __newindex → log
rawset(guarded, "y", 2)  -- bypasses __newindex → set directly
print(log.x)             -- 1   (captured by __newindex)
print(log.y)             -- nil (rawset bypassed __newindex)
print(rawget(guarded, "y"))  -- 2   (was set directly)

-- rawlen: length bypassing __len metamethod
local mt4 = {__len = function() return 999 end}
local tlen = setmetatable({10, 20, 30}, mt4)
print(#tlen)           -- 999  (uses __len)
print(rawlen(tlen))    -- 3    (bypasses __len)
print(rawlen("hello")) -- 5    (works on strings too)

-- load: compile and run a string as Lua code
local f = load("return 1 + 2")
print(f())   -- 3

local f2 = load("return ...")
print(f2(42))    -- 42
print(f2(1, 2))  -- 1   2

local f3 = load("local x = 10; local y = 20; return x + y")
print(f3())   -- 30

-- load with a syntax error returns nil + error message
local bad, err = load("this is not valid lua @@@")
print(bad)          -- nil
print(type(err))    -- string
