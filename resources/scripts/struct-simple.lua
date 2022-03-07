
function interface()
	struct = {
		bool = BOOL,
		float = FLOAT,
		vector2f = VEC2F,
		vector3f = VEC3F,
		vector4f = VEC4F,
		integer = INT,
		integer64 = INT64,
		vector2i = VEC2I,
		vector3i = VEC3I,
		vector4i = VEC4I,
		string = STRING,
    }
	IN.s = struct
	OUT.s = struct
end

function run()
	OUT.s.bool = IN.s.bool
	OUT.s.float = IN.s.float
    OUT.s.vector2f = IN.s.vector2f
    OUT.s.vector3f = IN.s.vector3f
    OUT.s.vector4f = IN.s.vector4f
    
    OUT.s.integer = IN.s.integer
    OUT.s.integer64 = IN.s.integer64
    OUT.s.vector2i = IN.s.vector2i
    OUT.s.vector3i = IN.s.vector3i
    OUT.s.vector4i = IN.s.vector4i
end