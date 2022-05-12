
function interface(IN,OUT)
	IN.choice = Type:Int32()
    IN.value = Type:Float()
    
    OUT.value = Type:String()
end


function run(IN,OUT)
    if IN.choice > 0 then
        OUT.value = IN.value
        error("WUFF")
    end
end
