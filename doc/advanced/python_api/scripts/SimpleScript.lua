

function interface(IN,OUT)
    IN.input = Type:Vec3f()
    OUT.output = Type:Vec3f()
end

function run(IN,OUT)
    OUT.output = IN.input
end
