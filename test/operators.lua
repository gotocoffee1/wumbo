-- Arithmetic operators
print(10 + 3)    -- 13
print(10 - 3)    -- 7
print(10 * 3)    -- 30
print(10 / 3)    -- 3.3333333333333 (float division always)
print(10 // 3)   -- 3 (floor division, integer result)
print(10 % 3)    -- 1
print(2 ^ 10)    -- 1024.0 (exponentiation always float)
print(-5)        -- -5
print(-(-3))     -- 3

-- Floor division rounds toward -inf
print(7 // 2)    -- 3
print(-7 // 2)   -- -4
print(7 // -2)   -- -4
print(7.0 // 2)  -- 3.0 (float operand keeps float result)

-- Modulo follows floor division sign
print(10 % 3)    -- 1
print(-1 % 3)    -- 2
print(1 % -3)    -- -2

-- Bitwise operators (Lua 5.3, integers only)
print(6 & 3)     -- 2
print(6 | 3)     -- 7
print(6 ~ 3)     -- 5  (XOR)
print(~0)        -- -1 (bitwise NOT)
print(~5)        -- -6
print(1 << 4)    -- 16
print(16 >> 4)   -- 1
print(1 << 63)   -- -9223372036854775808 (wraps to min int64)

-- Comparison operators
print(1 == 1)    -- true
print(1 ~= 2)    -- true
print(1 < 2)     -- true
print(2 > 1)     -- true
print(1 <= 1)    -- true
print(2 >= 2)    -- true
print(1 == 1.0)  -- true (integer/float cross-comparison)
print(1 ~= 2.0)  -- true
print("abc" == "abc")  -- true
print("abc" ~= "def")  -- true
print("abc" < "abd")   -- true
print("z" > "a")       -- true
print("abc" <= "abc")  -- true

-- Logical operators (return one of their operands, not booleans)
print(true and false)    -- false
print(true or false)     -- true
print(not true)          -- false
print(not false)         -- true
print(not nil)           -- true
print(not 0)             -- false  (0 is truthy in Lua!)
print(not "")            -- false  (empty string is truthy)
print(1 and 2)           -- 2
print(false and 2)       -- false
print(nil and 2)         -- nil
print(1 or 2)            -- 1
print(false or 2)        -- 2
print(nil or false)      -- false
print(nil or "default")  -- default
print(false or nil)      -- nil

-- Length operator
print(#"")           -- 0
print(#"hello")      -- 5
print(#{1, 2, 3})    -- 3
print(#{})           -- 0
