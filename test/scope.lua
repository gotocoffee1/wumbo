-- Local variable scoping: inner do-block does not affect outer variable
local i = 1

do
    local i = 6   -- shadows outer i, but only within this block
end
print(i)   -- 1
