function interface(IN, OUT)
    IN.timer = Type:Int64()
    IN.someInt = Type:Int64()
    IN.Angle = Type:Float()

    OUT.timer = Type:Int64()
    OUT.someInt = Type:Int64()
    OUT.Rotation = Type:Vec3f()
end


function run(IN, OUT)
    OUT.timer = IN.timer
    OUT.someInt = IN.someInt

    local t = IN.timer * 0.0000001
    OUT.Rotation = {0.0, IN.Angle * t, 0.0}
end
