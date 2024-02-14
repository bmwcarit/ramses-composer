
function interface(INOUT)
	INOUT.bvec = Type:Array(3, Type:Bool())
	INOUT.ivec = Type:Array(2, Type:Int32())
	INOUT.fvec = Type:Array(5, Type:Float())
	
	INOUT.avec2 = Type:Array(4, Type:Vec2f())
	INOUT.avec3 = Type:Array(5, Type:Vec3f())
	INOUT.avec4 = Type:Array(6, Type:Vec4f())
	
	INOUT.aivec2 = Type:Array(4, Type:Vec2i())
	INOUT.aivec3 = Type:Array(5, Type:Vec3i())
	INOUT.aivec4 = Type:Array(6, Type:Vec4i())
end
