

function interface()
    IN.in_float = FLOAT
    OUT.out_float = FLOAT
end

function run()
    local i = 1
    local v = 100
    while i < 10000 do
        v = v * v * v;
        i = i + 1
    end
    OUT.out_float = IN.in_float
end
