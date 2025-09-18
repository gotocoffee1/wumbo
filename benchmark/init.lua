local filename = arg[1]
local mode = arg[2] == "run"
local iterations = tonumber(arg[3])

if mode then
    local f = assert(loadfile(filename))
    for i=1, iterations do
        f()
    end
else
    local file = assert(io.open(filename, "rb"))
    local content = file:read("*all")
    file:close()
    file = nil

    for i=1, iterations do
        local f = assert(load(content))
    end
end
