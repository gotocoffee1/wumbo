local z = 1
local y = 1
local test
do
	test = function()
		z = z + 1
		print(z)
	end
end

do
	local function test2()
		y = y + 1
	end
end

z = 50

print(z)
print(y)
test()
