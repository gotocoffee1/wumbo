-- Multiple assignment

-- Basic multiple assignment
local a, b = 1, 2
print(a, b)   -- 1   2

-- Swap without a temp variable
a, b = b, a
print(a, b)   -- 2   1

-- More variables than values: extras become nil
local x, y, z = 10, 20
print(x, y, z)   -- 10  20  nil

-- More values than variables: extras are discarded
local p, q = 1, 2, 3, 4
print(p, q)   -- 1   2

-- Multiple assignment with function calls
local function duo() return 100, 200 end
local m, n = duo()
print(m, n)   -- 100  200

-- Function call not in last position: only first value used
local r, s = duo(), 999
print(r, s)   -- 100  999

-- Function call in last position expands
local t1, t2, t3 = 0, duo()
print(t1, t2, t3)   -- 0   100  200

-- Global multiple assignment
x1, x2, x3 = 10, 20, 30
print(x1, x2, x3)   -- 10  20  30
x1, x2 = x2, x1
print(x1, x2)   -- 20  10

-- Table field multiple assignment
local tbl = {}
tbl.a, tbl.b = 5, 6
print(tbl.a, tbl.b)   -- 5   6
tbl.a, tbl.b = tbl.b, tbl.a
print(tbl.a, tbl.b)   -- 6   5

-- Indexed multiple assignment
local arr = {0, 0, 0}
arr[1], arr[2], arr[3] = 10, 20, 30
print(arr[1], arr[2], arr[3])   -- 10  20  30
arr[1], arr[3] = arr[3], arr[1]
print(arr[1], arr[2], arr[3])   -- 30  20  10

-- All right-hand sides evaluated before assignment
local i = 1
local t = {10, 20, 30}
i, t[i] = i + 1, i + 1
-- right side: i+1=2, i+1=2; then assign i=2, t[1]=2
print(i)       -- 2
print(t[1])    -- 2
print(t[2])    -- 20
