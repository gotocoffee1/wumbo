-- break exits the innermost loop; code after break is unreachable
print("1")   -- 1
while true do
    print("2")   -- 2
    break
    print("3")   -- not reached
end
print("4")   -- 4

repeat
    print("5")   -- 5
    while true do
            print("6")   -- 6
            break        -- exits inner while only
            print("7")   -- not reached
    end
    print("8")   -- 8
    break        -- exits outer repeat
    print("9")   -- not reached
until false
print("10")   -- 10
 