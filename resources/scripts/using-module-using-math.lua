modules("cangaroo")

function interface(IN,OUT)
	IN.value = Type:Float()
	if cangaroo.value > 3.0 then
		OUT.value = Type:Float()
	end
end

function run(IN,OUT)
	OUT.value = IN.value
end
