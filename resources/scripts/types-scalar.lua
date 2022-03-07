
function interface()
	IN.float = FLOAT
	IN.vector2f = VEC2F
	IN.vector3f = VEC3F
	IN.vector4f = VEC4F
	IN.integer = INT
	IN.integer64 = INT64
	IN.vector2i = VEC2I
	IN.vector3i = VEC3I
	IN.vector4i = VEC4I
	IN.bool = BOOL
	IN.s = STRING

	OUT.ofloat = FLOAT
	OUT.ointeger = INT
	OUT.ointeger64 = INT64
	OUT.ovector3f = VEC3F
	OUT.ovector4f = VEC4F
	OUT.obool = BOOL
	OUT.foo = FLOAT
	OUT.bar = FLOAT
	OUT.flag = BOOL
end

function test(v)
	return v[1]
end

function run()
	local v = IN.vector3f
	-- OUT.ofloat = v[0]
	OUT.ofloat = test(IN.vector3f)
	
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
