
function interface(IN,OUT)
	IN.float_array = Type:Array(5, Type:Float())
	OUT.float_array = Type:Array(5, Type:Float())
end

function run(IN,OUT)
	OUT.float_array[1] = IN.float_array[1] 
	OUT.float_array[2] = IN.float_array[2]
	OUT.float_array[3] = IN.float_array[3]
	OUT.float_array[4] = IN.float_array[4]
	OUT.float_array[5] = IN.float_array[5]
end
