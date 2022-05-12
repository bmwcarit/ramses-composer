
function interface(IN,OUT)
	local struct = {
		bool = Type:Bool(),
		float = Type:Float(),
		vector2f = Type:Vec2f(),
		vector3f = Type:Vec3f(),
		vector4f = Type:Vec4f(),
		integer = Type:Int32(),
		integer64 = Type:Int64(),
		vector2i = Type:Vec2i(),
		vector3i = Type:Vec3i(),
		vector4i = Type:Vec4i(),
		string = Type:String(),
    }
	IN.s = struct
	OUT.s = struct
end

function run(IN,OUT)
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