function interface()
	IN.float = FLOAT
	OUT.float = FLOAT
end

function init()
	GLOBAL.test = 0
end

function run()
	GLOBAL.test = GLOBAL.test + 1
	OUT.float = IN.float + GLOBAL.test
end