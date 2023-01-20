modules("cangaroo")

function interface(INOUT)
    INOUT.float = Type:Float()
	if cangaroo.value > 3.0 then 
		INOUT.pp = Type:Float()
	end
end
