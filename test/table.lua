-- Table field assignment and access
local t = {}

t.first = 1

print(t.first)   -- 1

t.first = nil          -- setting to nil removes the field
print(t.first)   -- nil

t.first = { second = print, third = "test" }

t.first.second(t.first.third)   -- test  (calls print with "test")
