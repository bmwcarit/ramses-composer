
function interface(IN,OUT)
    IN.float = Type:Float()
	if math.pi > 3.0 then 
		OUT.float = Type:Float()
	end
end

function run(IN,OUT)
	OUT.float = IN.float
end
