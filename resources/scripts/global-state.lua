function interface(IN,OUT)
	IN.float = Type:Float()
	OUT.float = Type:Float()
end

function init()
	GLOBAL.test = 0
end

function run(IN,OUT)
	GLOBAL.test = GLOBAL.test + 1
	OUT.float = IN.float + GLOBAL.test
end