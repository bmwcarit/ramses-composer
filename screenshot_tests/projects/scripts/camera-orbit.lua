function interface(IN, OUT)
	IN.dist = Type:Float()
	IN.pitch = Type:Float()
	IN.yaw = Type:Float()
	OUT.rotation = Type:Vec3f()
	OUT.translation = Type:Vec3f()
end

function run(IN, OUT)

	-- local yaw = IN.yaw * math.pi / 180.0
	local yaw = IN.yaw
	local dir = {IN.dist * math.sin(yaw), 0, IN.dist * math.cos(yaw)}
	
	OUT.rotation = {IN.pitch, yaw * 180.0 / math.pi, 0.0}
	OUT.translation = dir

end