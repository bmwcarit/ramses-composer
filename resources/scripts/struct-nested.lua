
function interface()
	local s = { x = FLOAT, y = FLOAT }
	IN.v = s
	local complex = {
		bool = BOOL,
		float = FLOAT,
		vector2f = VEC2F,
		vector3f = VEC3F,
		vector4f = VEC4F,
		integer = INT,
		vector2i = VEC2I,
		vector3i = VEC3I,
		vector4i = VEC4I,
		string = STRING,
		struct = s
    }
	IN.s = complex
	IN.nested = {
		a = FLOAT,
		inner = complex
	}
	OUT.nested_out = {
		a = FLOAT,
		inner = complex
	}
end


function run()
end
