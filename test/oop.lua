-- Object-Oriented Programming with metatables

-- Class definition pattern
local Animal = {}
Animal.__index = Animal

function Animal.new(name, sound)
    local self = setmetatable({}, Animal)
    self.name = name
    self.sound = sound
    return self
end

function Animal:speak()
    print(self.name)
    print(self.sound)
end

function Animal:getName()
    return self.name
end

local dog = Animal.new("Rex", "Woof")
local cat = Animal.new("Whiskers", "Meow")
dog:speak()        -- Rex (newline) Woof
cat:speak()        -- Whiskers (newline) Meow
print(dog:getName())   -- Rex

-- Inheritance
local Dog = setmetatable({}, {__index = Animal})
Dog.__index = Dog

function Dog.new(name)
    local self = Animal.new(name, "Woof")
    return setmetatable(self, Dog)
end

function Dog:fetch(item)
    print(self.name)
    print(item)
end

local buddy = Dog.new("Buddy")
buddy:speak()         -- Buddy (newline) Woof (inherited)
buddy:fetch("ball")   -- Buddy (newline) ball

-- Overriding methods
function Dog:speak()
    print("Dog says: Woof!")
end
buddy:speak()   -- Dog says: Woof!

-- instanceof check via metatable
print(getmetatable(buddy) == Dog)     -- true
print(getmetatable(buddy) == Animal)  -- false

-- Class with private-like state via closures
local function Counter(initial)
    local count = initial or 0
    local self = {}
    function self.increment()
        count = count + 1
    end
    function self.decrement()
        count = count - 1
    end
    function self.get()
        return count
    end
    return self
end

local c = Counter(10)
print(c.get())       -- 10
c.increment()
c.increment()
print(c.get())       -- 12
c.decrement()
print(c.get())       -- 11

-- Multiple inheritance via __index function
local Flyable = {
    fly = function(self)
        print("flying")
    end
}
local Swimmable = {
    swim = function(self)
        print("swimming")
    end
}

local Duck = {}
Duck.__index = function(t, k)
    return Duck[k] or Flyable[k] or Swimmable[k]
end

function Duck.new()
    return setmetatable({}, Duck)
end

local duck = Duck.new()
duck:fly()    -- flying
duck:swim()   -- swimming

-- __tostring for objects
local Point = {}
Point.__index = Point
Point.__tostring = function(self)
    return tostring(self.x) .. "," .. tostring(self.y)
end

function Point.new(x, y)
    return setmetatable({x=x, y=y}, Point)
end

Point.__add = function(a, b)
    return Point.new(a.x + b.x, a.y + b.y)
end

local p1 = Point.new(1, 2)
local p2 = Point.new(3, 4)
local p3 = p1 + p2
print(p3.x, p3.y)   -- 4   6
print(tostring(p1))  -- 1,2
