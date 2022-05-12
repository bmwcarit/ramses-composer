
function interface(IN,OUT)
	local s = { x = Type:Float(), y = Type:Float() }
	IN.v = s
	local complex = {
		bool = Type:Bool(),
		float = Type:Float(),
		vector2f = Type:Vec2f(),
		vector3f = Type:Vec3f(),
		vector4f = Type:Vec4f(),
		integer = Type:Int32(),
		vector2i = Type:Vec2i(),
		vector3i = Type:Vec3i(),
		vector4i = Type:Vec4i(),
		string = Type:String(),
		struct = s
    }
	IN.s = complex
	IN.nested = {
		a = Type:Float(),
		inner = complex
	}
	OUT.nested_out = {
		a = Type:Float(),
		inner = complex
	}
end


function run(IN,OUT)
end
