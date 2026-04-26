-- Error handling with pcall, xpcall, error

-- pcall: returns true + results on success
local ok, val = pcall(function() return 42 end)
print(ok, val)    -- true  42

-- pcall: returns false + error message on failure
local ok2, err = pcall(function() error("something went wrong") end)
print(ok2)        -- false
print(type(err))  -- string

-- error with a table value
local ok3, err3 = pcall(function()
    error({code = 404, msg = "not found"})
end)
print(ok3)          -- false
print(err3.code)    -- 404
print(err3.msg)     -- not found

-- error with level 0 (no location info added)
local ok4, err4 = pcall(function()
    error("raw error", 0)
end)
print(ok4)    -- false
print(err4)   -- raw error

-- Nested pcall
local ok5, result5 = pcall(function()
    local ok_inner, err_inner = pcall(function()
        error("inner error", 0)
    end)
    print(ok_inner)     -- false
    print(err_inner)    -- inner error
    return "outer ok"
end)
print(ok5, result5)   -- true  outer ok

-- pcall with arguments
local ok6, r6 = pcall(function(a, b) return a + b end, 10, 20)
print(ok6, r6)   -- true  30

-- xpcall: like pcall but with a message handler
local function handler(err)
    return "handled: " .. tostring(err)
end

local ok7, msg7 = xpcall(function()
    error("test error", 0)
end, handler)
print(ok7)    -- false
print(msg7)   -- handled: test error

-- xpcall success
local ok8, val8 = xpcall(function() return 99 end, handler)
print(ok8, val8)   -- true  99

-- assert: passes through values on success
local v1, v2 = assert(10, "msg")
print(v1, v2)   -- 10  msg

-- assert: raises error on nil/false
local ok9, err9 = pcall(function()
    assert(false, "assertion message")
end)
print(ok9)   -- false

local ok10, err10 = pcall(function()
    assert(nil)
end)
print(ok10)   -- false

-- Re-raising errors
local ok11, err11 = pcall(function()
    local ok_inner, err_inner = pcall(function()
        error("original", 0)
    end)
    if not ok_inner then
        error(err_inner, 0)
    end
end)
print(ok11)    -- false
print(err11)   -- original

-- pcall with multiple return values
local ok12, a12, b12, c12 = pcall(function() return 1, 2, 3 end)
print(ok12, a12, b12, c12)   -- true  1  2  3
