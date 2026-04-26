-- Multiple return values
local function multi()
    return 1, 2, 3
end

local a, b, c = multi()
print(a, b, c)   -- 1   2   3

-- Extra values are discarded
local x, y = multi()
print(x, y)      -- 1   2

-- Missing values become nil
local p, q, r, s = multi()
print(p, q, r, s)  -- 1   2   3   nil

-- Multiple returns in expression list (only last call expands)
print(multi())        -- 1   2   3
print(multi(), 10)    -- 1   10  (call not in tail position)
print(10, multi())    -- 10  1   2   3

-- Variadic functions (...)
local function sum(...)
    local total = 0
    local args = {...}
    for i = 1, #args do
        total = total + args[i]
    end
    return total
end

print(sum(1, 2, 3))       -- 6
print(sum(10, 20, 30))    -- 60
print(sum())              -- 0

-- Passing varargs through
local function wrap(...)
    return ...
end
print(wrap(4, 5, 6))   -- 4   5   6

-- Recursive functions
local function fact(n)
    if n <= 1 then return 1 end
    return n * fact(n - 1)
end
print(fact(1))   -- 1
print(fact(5))   -- 120
print(fact(10))  -- 3628800

-- Fibonacci
local function fib(n)
    if n <= 1 then return n end
    return fib(n-1) + fib(n-2)
end
print(fib(0))   -- 0
print(fib(1))   -- 1
print(fib(10))  -- 55

-- Anonymous functions assigned to variables
local double = function(x) return x * 2 end
print(double(7))   -- 14

-- Functions as first-class values
local function apply(f, x)
    return f(x)
end
print(apply(double, 5))    -- 10
print(apply(fact, 6))      -- 720

-- Functions returning functions (higher-order)
local function make_adder(n)
    return function(x) return x + n end
end
local add5 = make_adder(5)
local add10 = make_adder(10)
print(add5(3))    -- 8
print(add10(3))   -- 13
print(add5(add10(1)))  -- 16

-- Nested functions
local function outer(x)
    local function inner(y)
        return x + y
    end
    return inner(x * 2)
end
print(outer(3))   -- 9  (3 + 3*2)

-- Method syntax: f:method(args) == f.method(f, args)
local obj = {value = 42}
function obj:get()
    return self.value
end
function obj:set(v)
    self.value = v
end
print(obj:get())   -- 42
obj:set(100)
print(obj:get())   -- 100

-- Tail calls (should not grow the stack)
local function count_down(n)
    if n == 0 then return "done" end
    return count_down(n - 1)
end
print(count_down(1000))   -- done
