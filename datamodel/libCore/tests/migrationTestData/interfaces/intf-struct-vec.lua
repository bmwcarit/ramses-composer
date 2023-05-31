function interface(INOUT)
	local struct_3f = { x = Type:Float(), y = Type:Float(), z = Type:Float() }
	INOUT.s3f = struct_3f
	INOUT.v3f = Type:Vec3f()

	local struct_3i = { i1 = Type:Int32(), i2 = Type:Int32(), i3 = Type:Int32() }
	INOUT.s3i = struct_3i
	INOUT.v3i = Type:Vec3i()
end