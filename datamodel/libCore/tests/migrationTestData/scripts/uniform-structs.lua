
function interface(INOUT)
	local struct_prim = {
        i = Type:Int32(),
        f = Type:Float(),
		
		v2 = Type:Vec2f(),
		v3 = Type:Vec3f(),
		v4 = Type:Vec4f(),
		
		iv2 = Type:Vec2i(),
		iv3 = Type:Vec3i(),
		iv4 = Type:Vec4i()
    }
	
	local struct_array_prim = {
        ivec = Type:Array(2, Type:Int32()),
        fvec = Type:Array(5, Type:Float()),
		
		avec2 = Type:Array(4, Type:Vec2f()),
		avec3 = Type:Array(5, Type:Vec3f()),
		avec4 = Type:Array(6, Type:Vec4f()),
		
		aivec2 = Type:Array(4, Type:Vec2i()),
		aivec3 = Type:Array(5, Type:Vec3i()),
		aivec4 = Type:Array(6, Type:Vec4i())
    }

	local struct_array_struct = {
		prims = Type:Array(2, struct_prim)
	}

	INOUT.s_prims = struct_prim
	INOUT.s_a_prims = struct_array_prim
	INOUT.a_s_prims = Type:Array(2, struct_prim)
	INOUT.s_a_struct_prim = struct_array_struct
end
