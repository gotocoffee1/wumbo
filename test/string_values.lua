-- String literals: single quotes, double quotes, long strings
print("hello")           -- hello
print('world')           -- world
print([[long string]])   -- long string
print([=[another]=])     -- another

-- Escape sequences
print("newline:\nend")   -- newline: (newline) end
print("tab:\there")      -- tab:     here
print("quote:\"end\"")   -- quote:"end"
print("backslash:\\end") -- backslash:\end
print("null:\0end")      -- null: (null byte) end  [length 9]
print("\65\66\67")        -- ABC  (decimal escapes)
print("\x41\x42\x43")    -- ABC  (hex escapes)

-- String length
print(#"")         -- 0
print(#"hello")    -- 5
print(#"ab\0cd")   -- 5  (null byte counts)

-- String comparison (lexicographic by byte value)
print("abc" == "abc")   -- true
print("abc" ~= "def")   -- true
print("abc" < "abd")    -- true
print("abc" < "abcd")   -- true
print("b" > "a")        -- true
print("" < "a")         -- true
print("Z" < "a")        -- true  (uppercase < lowercase in ASCII)
print("abc" <= "abc")   -- true
print("abc" >= "abc")   -- true

-- Strings are not equal to numbers even if they look the same
print("1" == 1)    -- false
print(1 == "1")    -- false

-- tostring/tonumber conversions
print(tostring(42))      -- 42
print(tostring(42.5))    -- 42.5
print(tostring(true))    -- true
print(tostring(false))   -- false
print(tostring(nil))     -- nil
print(tonumber("42"))    -- 42
print(tonumber("42.5"))  -- 42.5
print(tonumber("0xff"))  -- 255
print(tonumber("10", 2)) -- 2  (binary)
print(tonumber("ff", 16))-- 255 (hex)
print(tonumber("hello")) -- nil (not a number)
