

function interface(IN,OUT)
    IN.in_float = Type:Float()
    OUT.out_float = Type:Float()
end

function run(IN,OUT)
    local i = 1
    local v = 100
    while i < 10000 do
        v = v * v * v;
        i = i + 1
    end
    OUT.out_float = IN.in_float
end
