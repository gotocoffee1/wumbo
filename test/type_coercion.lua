-- type() function returns a string naming the type

print(type(nil))             -- nil
print(type(true))            -- boolean
print(type(false))           -- boolean
print(type(42))              -- number
print(type(42.0))            -- number
print(type("hello"))         -- string
print(type({}))              -- table
print(type(print))           -- function
print(type(type))            -- function

-- type of results
print(type(1 + 1))           -- number
print(type(1 + 1.0))         -- number
print(type(1 < 2))           -- boolean
print(type(nil or false))    -- boolean
print(type(1 and 2))         -- number

-- Implicit coercions: strings to numbers in arithmetic
print("10" + 5)      -- 15
print("3.5" + 1.5)   -- 5.0
print("10" * 2)      -- 20
print("8" / "2")     -- 4.0

-- tonumber
print(tonumber(42))        -- 42
print(tonumber(42.5))      -- 42.5
print(tonumber("100"))     -- 100
print(tonumber("  42 "))   -- 42   (trims whitespace)
print(tonumber("0xFF"))    -- 255
print(tonumber("1e3"))     -- 1000.0
print(tonumber(true))      -- nil  (boolean not convertible)
print(tonumber(nil))       -- nil
print(tonumber({}))        -- nil  (table not convertible)

-- tostring
print(tostring(nil))       -- nil
print(tostring(true))      -- true
print(tostring(false))     -- false
print(tostring(42))        -- 42
print(tostring(42.0))      -- 42.0
print(tostring(0.0))       -- 0.0

-- tostring on table: shows type (unless __tostring metamethod)
print(type(tostring({})))    -- string  (implementation-defined format)
print(type(tostring(print))) -- string

-- Arithmetic coercion failures (caught by pcall)
local ok, err = pcall(function() return {} + 1 end)
print(ok)   -- false

local ok2, err2 = pcall(function() return "abc" + 1 end)
print(ok2)  -- false

local ok3, err3 = pcall(function() return nil + 1 end)
print(ok3)  -- false
