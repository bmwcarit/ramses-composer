
function interface(IN, OUT)
	IN.float_array = Type:Array(3, Type:Array(2, Type:Float()))
	OUT.float_array = Type:Array(3, Type:Array(2, Type:Float()))
end

function run(IN, OUT)
	OUT.float_array[2] = IN.float_array[1]
end
