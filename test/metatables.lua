-- __index as function: called when key not found in table
local t = setmetatable({existing = 1}, {
    __index = function(table, key)
        return 99
    end
})
print(t.existing)   -- 1   (raw value, __index not called)
print(t.missing)    -- 99  (__index called)

-- __index as table: prototype-based lookup
local proto = {x = 10, y = 20}
local obj = setmetatable({z = 30}, {__index = proto})
print(obj.x)    -- 10  (from proto)
print(obj.y)    -- 20  (from proto)
print(obj.z)    -- 30  (own value)

-- __newindex: called when setting a new key
local proxy = {}
local logged = setmetatable({}, {
    __newindex = function(t, k, v)
        proxy[k] = v
    end
})
logged.a = 42
logged.b = 99
print(proxy.a)   -- 42
print(proxy.b)   -- 99

-- __add
local mt_add = {
    __add = function(a, b)
        return setmetatable({n = a.n + b.n}, getmetatable(a))
    end
}
mt_add.__index = mt_add
local a = setmetatable({n = 10}, mt_add)
local b = setmetatable({n = 5}, mt_add)
local c = a + b
print(c.n)    -- 15

-- __sub
local mt_sub = {
    __sub = function(a, b)
        return setmetatable({n = a.n - b.n}, getmetatable(a))
    end
}
local x = setmetatable({n = 10}, mt_sub)
local y = setmetatable({n = 3}, mt_sub)
print((x - y).n)   -- 7

-- __mul
local mt_mul = {
    __mul = function(a, b)
        return setmetatable({n = a.n * b.n}, getmetatable(a))
    end
}
local p = setmetatable({n = 4}, mt_mul)
local q = setmetatable({n = 3}, mt_mul)
print((p * q).n)   -- 12

-- __div
local mt_div = {
    __div = function(a, b)
        return setmetatable({n = a.n / b.n}, getmetatable(a))
    end
}
local u = setmetatable({n = 10}, mt_div)
local v = setmetatable({n = 4}, mt_div)
print((u / v).n)   -- 2.5

-- __unm (unary minus)
local mt_unm = {
    __unm = function(a)
        return setmetatable({n = -a.n}, getmetatable(a))
    end
}
local r = setmetatable({n = 7}, mt_unm)
print((-r).n)   -- -7

-- __mod
local mt_mod = {
    __mod = function(a, b)
        return setmetatable({n = a.n % b.n}, getmetatable(a))
    end
}
local m1 = setmetatable({n = 10}, mt_mod)
local m2 = setmetatable({n = 3}, mt_mod)
print((m1 % m2).n)   -- 1

-- __pow
local mt_pow = {
    __pow = function(a, b)
        return setmetatable({n = a.n ^ b.n}, getmetatable(a))
    end
}
local e1 = setmetatable({n = 2}, mt_pow)
local e2 = setmetatable({n = 8}, mt_pow)
print((e1 ^ e2).n)   -- 256.0

-- __idiv (floor division)
local mt_idiv = {
    __idiv = function(a, b)
        return setmetatable({n = a.n // b.n}, getmetatable(a))
    end
}
local d1 = setmetatable({n = 10}, mt_idiv)
local d2 = setmetatable({n = 3}, mt_idiv)
print((d1 // d2).n)   -- 3

-- __call: makes a table callable
local callable = setmetatable({}, {
    __call = function(self, a, b)
        return a + b
    end
})
print(callable(3, 4))    -- 7
print(callable(10, 20))  -- 30

-- __len: custom length
local custom_len = setmetatable({}, {
    __len = function(self)
        return 42
    end
})
print(#custom_len)   -- 42

-- __tostring: custom string representation
local obj2 = setmetatable({value = 123}, {
    __tostring = function(self)
        return tostring(self.value)
    end
})
print(tostring(obj2))   -- 123

-- __eq: equality metamethod (called when operands share the metamethod)
local mt_eq = {
    __eq = function(a, b) return a.val == b.val end
}
local e1 = setmetatable({val = 5}, mt_eq)
local e2 = setmetatable({val = 5}, mt_eq)
local e3 = setmetatable({val = 6}, mt_eq)
print(e1 == e2)   -- true
print(e1 == e3)   -- false

-- __lt and __le: comparison metamethods
local mt_cmp = {
    __lt = function(a, b) return a.v < b.v end,
    __le = function(a, b) return a.v <= b.v end,
}
local n1 = setmetatable({v = 1}, mt_cmp)
local n2 = setmetatable({v = 2}, mt_cmp)
print(n1 < n2)    -- true
print(n2 < n1)    -- false
print(n1 <= n1)   -- true
print(n2 <= n1)   -- false

-- Chained __index (inheritance chain)
local base = {base_method = function(self) return "base" end}
base.__index = base
local derived = setmetatable({}, base)
derived.__index = derived
local instance = setmetatable({}, derived)
print(instance:base_method())   -- base
