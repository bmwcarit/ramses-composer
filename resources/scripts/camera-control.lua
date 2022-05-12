
function interface(IN,OUT)
	IN.struct = {
        a = Type:Int32(),
        b = Type:Int32(),
		f = Type:Float(),
		g = Type:Float()
    }
	OUT.viewport = {
		offsetX = Type:Int32(),
		offsetY = Type:Int32(),
		width = Type:Int32(),
		height = Type:Int32()
	}
	OUT.perspFrustum = {
		nearPlane = Type:Float(),
		farPlane = Type:Float(),
		fieldOfView = Type:Float(),
		aspectRatio = Type:Float()
	}
	OUT.orthoFrustum = {
		nearPlane = Type:Float(),
		farPlane = Type:Float(),
		leftPlane = Type:Float(),
		rightPlane = Type:Float(),
		topPlane = Type:Float(),
		bottomPlane = Type:Float()
	}
end

function run(IN,OUT)
	OUT.viewport.offsetX = 0
	OUT.viewport.offsetY = IN.struct.b
	OUT.viewport.width = IN.struct.a
	OUT.viewport.height = 700
	
	OUT.perspFrustum.nearPlane = 0.01 * IN.struct.f
	OUT.perspFrustum.farPlane = 10 * IN.struct.f
	OUT.perspFrustum.fieldOfView = 50
	OUT.perspFrustum.aspectRatio = IN.struct.g

	OUT.orthoFrustum.nearPlane = 0.01 * IN.struct.f
	OUT.orthoFrustum.farPlane = 10 * IN.struct.f
	OUT.orthoFrustum.leftPlane = -10*IN.struct.g
	OUT.orthoFrustum.rightPlane = 10*IN.struct.g
	OUT.orthoFrustum.bottomPlane = -10*IN.struct.g
	OUT.orthoFrustum.topPlane = 10*IN.struct.g
end
