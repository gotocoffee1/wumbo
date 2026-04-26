-- if / elseif / else
local function classify(n)
    if n < 0 then
        print("negative")
    elseif n == 0 then
        print("zero")
    elseif n < 10 then
        print("small")
    elseif n < 100 then
        print("medium")
    else
        print("large")
    end
end

classify(-5)   -- negative
classify(0)    -- zero
classify(7)    -- small
classify(50)   -- medium
classify(200)  -- large

-- Truthy/falsy: only nil and false are falsy
if 0 then print("0 is truthy") end        -- 0 is truthy
if "" then print("empty string truthy") end  -- empty string truthy
if not nil then print("nil is falsy") end    -- nil is falsy
if not false then print("false is falsy") end  -- false is falsy

-- while loop
local i = 1
local sum = 0
while i <= 10 do
    sum = sum + i
    i = i + 1
end
print(sum)   -- 55

-- while with break
local j = 0
while true do
    j = j + 1
    if j == 5 then break end
end
print(j)   -- 5

-- repeat/until
local k = 0
repeat
    k = k + 1
until k >= 5
print(k)   -- 5

-- repeat/until: condition can see locals declared in body
repeat
    local x = 42
until x == 42   -- x is visible here
print("repeat done")   -- repeat done

-- Numeric for: basic
local total = 0
for n = 1, 10 do
    total = total + n
end
print(total)   -- 55

-- Numeric for: with step
local vals = {}
local idx = 1
for n = 0, 20, 3 do
    vals[idx] = n
    idx = idx + 1
end
for i = 1, #vals do print(vals[i]) end
-- 0 3 6 9 12 15 18

-- Numeric for: negative step (count down)
for n = 5, 1, -1 do
    print(n)
end
-- 5 4 3 2 1

-- Numeric for: float step
local fvals = {}
local fi = 1
for n = 0.0, 1.0, 0.25 do
    fvals[fi] = n
    fi = fi + 1
end
for i = 1, #fvals do print(fvals[i]) end
-- 0.0  0.25  0.5  0.75  1.0

-- Nested loops with break only breaks inner
local found_i, found_j = 0, 0
for i = 1, 5 do
    for j = 1, 5 do
        if i * j == 12 then
            found_i = i
            found_j = j
            break
        end
    end
    if found_i ~= 0 then break end
end
print(found_i, found_j)   -- 3   4

-- do...end block (creates new scope)
local outer_val = "outer"
do
    local outer_val = "inner"
    print(outer_val)   -- inner
end
print(outer_val)   -- outer

-- Nested if
local function sign(x)
    if x ~= 0 then
        if x > 0 then
            return 1
        else
            return -1
        end
    else
        return 0
    end
end
print(sign(-10))  -- -1
print(sign(0))    -- 0
print(sign(10))   -- 1
