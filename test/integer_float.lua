-- Lua 5.3 introduced a proper integer subtype distinct from floats.
-- Integers and floats are both "number" type but behave differently.

-- Integers print without decimal point
print(1)       -- 1
print(100)     -- 100
print(-42)     -- -42

-- Floats always print with decimal point or exponent
print(1.0)     -- 1.0
print(100.0)   -- 100.0
print(-42.0)   -- -42.0
print(0.5)     -- 0.5
print(1.5)     -- 1.5

-- Both are "number" type
print(type(1))    -- number
print(type(1.0))  -- number

-- Division always produces float
print(10 / 2)   -- 5.0
print(6 / 3)    -- 2.0
print(1 / 3)    -- 0.33333333333333

-- Exponentiation always produces float
print(2 ^ 0)    -- 1.0
print(2 ^ 3)    -- 8.0
print(4 ^ 0.5)  -- 2.0

-- Floor division preserves type
print(7 // 2)     -- 3      (int // int = int)
print(7.0 // 2)   -- 3.0    (float // int = float)
print(7 // 2.0)   -- 3.0    (int // float = float)
print(7.0 // 2.0) -- 3.0    (float // float = float)

-- Arithmetic with mixed types promotes to float
print(1 + 1)     -- 2
print(1 + 1.0)   -- 2.0
print(1.0 + 1)   -- 2.0
print(1 * 2)     -- 2
print(1 * 2.0)   -- 2.0

-- Large integers stay exact
print(2^53)         -- 9.007199254741e+15 (float, loses precision)
print(9007199254740992)  -- 9007199254740992 (integer, exact)

-- Integer overflow wraps
print(9223372036854775807 + 1)   -- -9223372036854775808 (wraps)
print(-9223372036854775808 - 1)  -- 9223372036854775807 (wraps)

-- tostring reveals the type distinction
print(tostring(1))    -- 1
print(tostring(1.0))  -- 1.0
