-- Math library (math.*)

-- Constants
print(math.pi)                -- 3.1415926535898
print(math.huge)              -- inf
print(math.maxinteger)        -- 9223372036854775807
print(math.mininteger)        -- -9223372036854775808

-- Basic functions
print(math.abs(-5))           -- 5
print(math.abs(5))            -- 5
print(math.abs(-3.14))        -- 3.14

print(math.floor(3.7))        -- 3
print(math.floor(-3.7))       -- -4
print(math.floor(3.0))        -- 3

print(math.ceil(3.2))         -- 4
print(math.ceil(-3.2))        -- -3
print(math.ceil(3.0))         -- 3

print(math.sqrt(4))           -- 2.0
print(math.sqrt(2))           -- 1.4142135623731

-- min / max
print(math.max(1, 5, 3, 2, 4))    -- 5
print(math.min(1, 5, 3, 2, 4))    -- 1
print(math.max(1))                 -- 1
print(math.min(-3, -1, -2))        -- -3

-- Trigonometry
print(math.sin(0))                 -- 0.0
print(math.cos(0))                 -- 1.0
print(math.tan(0))                 -- 0.0
print(math.sin(math.pi / 2))       -- 1.0
print(math.cos(math.pi))           -- -1.0

-- Inverse trig
print(math.asin(1))                -- 1.5707963267949
print(math.acos(1))                -- 0.0
print(math.atan(1))                -- 0.78539816339745
print(math.atan(1, 0))             -- 1.5707963267949  (atan2)

-- Logarithm / exponentiation
print(math.exp(0))                 -- 1.0
print(math.exp(1))                 -- 2.718281828459
print(math.log(1))                 -- 0.0
print(math.log(math.exp(1)))       -- 1.0
print(math.log(8, 2))              -- 3.0

-- Type checking (Lua 5.3)
print(math.type(1))                -- integer
print(math.type(1.0))             -- float
print(math.type("x"))             -- fail (not a number)

-- Integer conversion
print(math.tointeger(5.0))        -- 5
print(math.tointeger(5.5))        -- nil
print(math.tointeger(5))          -- 5

-- modf: returns integer and fractional parts
print(math.modf(3.7))             -- 3   0.7
print(math.modf(-3.7))            -- -3  -0.7

-- fmod
print(math.fmod(7, 3))            -- 1.0
print(math.fmod(-7, 3))           -- -1.0  (C fmod, not Lua %)

-- Degrees / radians
print(math.deg(math.pi))          -- 180.0
print(math.rad(180))              -- 3.1415926535898
