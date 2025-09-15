local function a()
    return function() print(1) end
end

a()()

local function b()
    return {d = 2}
end

print(b().d)
b().d = 3
print(4)

local function c() 
    return { d = function() print(5) end }
end

c():d()
