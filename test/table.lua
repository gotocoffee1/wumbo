local t = {}

t.first = 1

print(t.first)

t.first = nil
print(t.first)

t.first = { second = print, third = "test" }

t.first.second(t.first.third)
