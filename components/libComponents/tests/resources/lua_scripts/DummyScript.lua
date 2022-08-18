function interface(IN,OUT)
    IN.dummyPropBool = Type:Bool()
    OUT.dummyPropBool = Type:Bool()
end

function run(IN,OUT)
    OUT.dummyPropBool = IN.dummyPropBool
end
