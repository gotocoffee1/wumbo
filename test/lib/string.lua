-- String library (string.*)
-- Note: string methods can also be called on string values via the string metatable

-- string.len
print(string.len("hello"))    -- 5
print(string.len(""))         -- 0
print(("hello"):len())        -- 5  (method syntax)

-- string.sub
print(string.sub("hello", 1, 3))    -- hel
print(string.sub("hello", 2))       -- ello
print(string.sub("hello", -3))      -- llo
print(string.sub("hello", 2, -2))   -- ell
print(string.sub("hello", 10))      -- "" (out of range)

-- string.rep
print(string.rep("ab", 3))          -- ababab
print(string.rep("x", 0))           -- ""  (empty)
print(string.rep("ha", 3, "-"))     -- ha-ha-ha  (with separator)

-- string.reverse
print(string.reverse("hello"))      -- olleh
print(string.reverse(""))           -- ""

-- string.upper / string.lower
print(string.upper("hello"))   -- HELLO
print(string.lower("WORLD"))   -- world
print(string.upper("Hello World"))  -- HELLO WORLD

-- string.byte / string.char
print(string.byte("A"))          -- 65
print(string.byte("ABC", 2))     -- 66
print(string.byte("ABC", 1, 3))  -- 65  66  67
print(string.char(72, 101, 108, 108, 111))  -- Hello

-- string.format
print(string.format("%d", 42))          -- 42
print(string.format("%05d", 42))        -- 00042
print(string.format("%f", 3.14))        -- 3.140000
print(string.format("%.2f", 3.14159))  -- 3.14
print(string.format("%s", "hello"))     -- hello
print(string.format("%q", 'say "hi"'))  -- "say \"hi\""
print(string.format("%x", 255))         -- ff
print(string.format("%X", 255))         -- FF
print(string.format("%o", 8))           -- 10
print(string.format("%10s", "hi"))      --         hi
print(string.format("%-10s|", "hi"))   -- hi        |
print(string.format("%e", 314.16e-2))  -- 3.141600e+00
print(string.format("%i", -42))        -- -42

-- string.find (plain)
print(string.find("hello world", "world"))       -- 7   11
print(string.find("hello world", "world", 1, true))  -- 7  11
print(string.find("hello", "xyz"))               -- nil

-- string.find (pattern)
print(string.find("hello123", "%d+"))    -- 6   8
print(string.find("   hello", "^%s*"))  -- 1   3

-- string.match
print(string.match("hello123", "%d+"))      -- 123
print(string.match("2023-04-15", "(%d+)-(%d+)-(%d+)"))  -- 2023  04  15
print(string.match("hello", "xyz"))         -- nil

-- string.gmatch
local words = {}
for w in string.gmatch("one two three", "%a+") do
    words[#words + 1] = w
end
print(#words)       -- 3
print(words[1])     -- one
print(words[2])     -- two
print(words[3])     -- three

-- string.gsub
print(string.gsub("hello world", "o", "0"))         -- hell0 w0rld  2
print(string.gsub("aaa", "a", "b", 2))              -- bba  2
print(string.gsub("hello", "(%w+)", "[%1]"))        -- [hello]  1
