function interface(IN,OUT)
    IN.colorDoors = Type:Vec4f()
    IN.colorSteeringWheel = Type:Vec4f()
    IN.colorSeat = Type:Vec4f()

    OUT.colorDoors = Type:Vec3f()
    OUT.colorSteeringWheel = Type:Vec4f()
    OUT.colorSeat = Type:Vec4f()
end

function run(IN,OUT)
    OUT.colorDoors = IN.colorDoors
    OUT.colorSteeringWheel = IN.colorSteeringWheel
    OUT.colorSeat = IN.colorSeat

end
