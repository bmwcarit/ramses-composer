function interface(IN,OUT)
    IN.StateOfChargeValue = Type:Float()
    IN.SliderValue = Type:Float()
    IN.DisableWarningArea = Type:Bool()
    IN.SystemColors = {
        C1 = Type:Vec3f(),
        C2 = Type:Vec3f(),
        C3 = Type:Vec3f(),
        C4 = Type:Vec3f(),
        C5 = Type:Vec3f(),
        C6 = Type:Vec3f(),
    }
    
    OUT.StateOfChargeValue = Type:Float()
    OUT.SliderValue = Type:Float()
    OUT.DisableWarningArea = Type:Bool()
    OUT.SystemColors = {
        C1 = Type:Vec3f(),
        C2 = Type:Vec3f(),
        C3 = Type:Vec3f(),
        C4 = Type:Vec3f(),
        C5 = Type:Vec3f(),
        C6 = Type:Vec3f(),
    }
end

function run(IN,OUT)
	OUT.StateOfChargeValue = IN.StateOfChargeValue
	OUT.SliderValue = IN.SliderValue
	OUT.DisableWarningArea = IN.DisableWarningArea
	OUT.SystemColors = IN.SystemColors
end
