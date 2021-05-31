function interface()
	IN.float = FLOAT
	OUT.float = FLOAT
end

test = 0

function run()
	test = test + 1
	OUT.float = IN.float + test
end