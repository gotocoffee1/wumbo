-- Numeric for: explicit step of 11
for i = 1, 100, 11 do print(i) end
-- 1 12 23 34 45 56 67 78 89 100

-- Numeric for: default step of 1
for i = 1, 100 do print(i) end
-- 1 2 3 ... 100

-- Nested numeric for loops
for i = 1, 20 do
    for j = 1, 10 do print(i, j) end
end
-- prints all (i, j) pairs: i in [1,20], j in [1,10]
