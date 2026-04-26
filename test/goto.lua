-- goto statement (introduced in Lua 5.2, present in 5.3)

-- Basic goto: jump forward
do
    goto skip
    print("this should not print")
    ::skip::
    print("after skip")   -- after skip
end

-- goto to implement continue in a loop
local evens = {}
local idx = 1
for i = 1, 10 do
    if i % 2 ~= 0 then goto continue end
    evens[idx] = i
    idx = idx + 1
    ::continue::
end
for i = 1, #evens do print(evens[i]) end
-- 2 4 6 8 10

-- goto used to exit nested loops (like a labeled break)
local result = 0
for i = 1, 10 do
    for j = 1, 10 do
        if i + j > 15 then
            goto done
        end
        result = result + 1
    end
end
::done::
print(result)   -- 105  (all pairs where i+j <= 15)

-- goto can jump to a label in the enclosing scope
local count = 0
::again::
count = count + 1
if count < 3 then goto again end
print(count)   -- 3

-- goto with local variable scoping (cannot jump into a local's scope)
-- This tests that goto respects variable lifetimes
local flag = false
goto check
::set_flag::
flag = true
goto check
::check::
if not flag then
    goto set_flag
end
print(flag)   -- true
