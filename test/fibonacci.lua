local function fibonacci(k)
	if k < 2 then
		return k
	end
	return fibonacci(k - 1) + fibonacci(k - 2)
end
print(fibonacci(40))
