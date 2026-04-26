-- String concatenation with ..
print("hello" .. " " .. "world")   -- hello world
print("foo" .. "bar")              -- foobar

-- Numbers are coerced to strings for concatenation
print(1 .. 2)          -- 12
print(1.5 .. "x")     -- 1.5x
print(10 .. "!")       -- 10!

-- Building strings in a loop
local s = ""
for i = 1, 5 do
    s = s .. i
end
print(s)    -- 12345

-- Concatenation is right-associative
-- "a" .. "b" .. "c"  ==  "a" .. ("b" .. "c")
local a = "x"
local b = "y"
local c = "z"
print(a .. b .. c)    -- xyz

-- Long string concatenation
local result = "start"
result = result .. "-middle"
result = result .. "-end"
print(result)         -- start-middle-end

-- Concat with escaped characters
print("line1\n" .. "line2")   -- line1 (newline) line2
