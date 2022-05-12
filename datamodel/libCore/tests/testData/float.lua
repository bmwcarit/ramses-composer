function interface(IN,OUT)
	IN.float = Type:Float()
	IN.int = Type:Int32()
	OUT.float = Type:Float()
	OUT.int = Type:Int32()
end

function run(IN,OUT)
	OUT.float = IN.float
	OUT.int = IN.int
end
