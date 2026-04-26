-- Calling the return value of a function call immediately
local function a()
    return function() print(1) end
end

a()()   -- 1  (calls the returned function)

-- Indexing the return value of a function call
local function b()
    return {d = 2}
end

print(b().d)   -- 2
b().d = 3      -- assignment to field of returned table (no observable effect here)
print(4)       -- 4

-- Method call on the return value of a function call  (: passes table as first arg)
local function c()
    return { d = function() print(5) end }
end

c():d()   -- 5
