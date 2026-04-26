-- Upvalues: closures capture variables by reference, not by value
local z = 1
local y = 1
local test
do
	test = function()
		z = z + 1   -- captures z from outer scope
		print(z)
	end
end

do
	local function test2()
		y = y + 1   -- also captures y, but test2 is not accessible outside this block
	end
end

z = 50   -- modifies z; the closure inside test sees this change

print(z)    -- 50
print(y)    -- 1  (test2 was never called so y was never incremented)
test()      -- 51 (z was 50, +1 = 51)
