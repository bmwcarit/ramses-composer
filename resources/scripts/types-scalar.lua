
function interface(IN,OUT)
	IN.float = Type:Float()
	IN.vector2f = Type:Vec2f()
	IN.vector3f = Type:Vec3f()
	IN.vector4f = Type:Vec4f()
	IN.integer = Type:Int32()
	IN.integer64 = Type:Int64()
	IN.vector2i = Type:Vec2i()
	IN.vector3i = Type:Vec3i()
	IN.vector4i = Type:Vec4i()
	IN.bool = Type:Bool()
	IN.s = Type:String()

	OUT.ofloat = Type:Float()
	OUT.ointeger = Type:Int32()
	OUT.ointeger64 = Type:Int64()
	OUT.ovector2f = Type:Vec2f()
	OUT.ovector3f = Type:Vec3f()
	OUT.ovector4f = Type:Vec4f()
	OUT.ovector2i = Type:Vec2i()
	OUT.ovector3i = Type:Vec3i()
	OUT.ovector4i = Type:Vec4i()
	OUT.obool = Type:Bool()
	OUT.foo = Type:Float()
	OUT.bar = Type:Float()
	OUT.flag = Type:Bool()
end

function init()
	GLOBAL.test = function (v)
		return v[1]
	end
end

function run(IN,OUT)
	local v = IN.vector3f
	-- OUT.ofloat = v[0]
	OUT.ofloat = GLOBAL.test(IN.vector3f)
	
	OUT.ointeger = 2*IN.integer
	OUT.ointeger64 = 2*IN.integer64
	OUT.ovector3f = {IN.float, 2*IN.float, 3.0}
	
	OUT.ovector4f = {v[1], IN.float, IN.vector3f[1], IN.vector3f[2]}
	OUT.obool = not IN.bool
	OUT.flag = IN.float > 0.5
	-- OUT.flag = IN.vector3f


	if IN.bool then
		OUT.foo = IN.float
	else
		OUT.bar = IN.float
	end
end
