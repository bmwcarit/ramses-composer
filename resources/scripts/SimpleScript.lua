

function interface(IN,OUT)
    IN.in_float = Type:Float()
    OUT.out_float = Type:Float()
end

function run(IN,OUT)
    OUT.out_float = IN.in_float
end
