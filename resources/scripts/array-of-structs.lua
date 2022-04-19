function interface()
	local FloatPair = { a = FLOAT, b = FLOAT }
	IN.array = ARRAY(1, FloatPair)
	OUT.array = ARRAY(1, FloatPair)
end

function run()
    OUT.array = IN.array
end
