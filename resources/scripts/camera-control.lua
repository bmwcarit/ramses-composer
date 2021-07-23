
function interface()
	IN.struct = {
        a = INT,
        b = INT,
		f = FLOAT,
		g = FLOAT
    }
	OUT.viewport = {
		offsetX = INT,
		offsetY = INT,
		width = INT,
		height = INT
	}
	OUT.perspFrustum = {
		nearPlane = FLOAT,
		farPlane = FLOAT,
		fieldOfView = FLOAT,
		aspectRatio = FLOAT
	}
	OUT.orthoFrustum = {
		nearPlane = FLOAT,
		farPlane = FLOAT,
		leftPlane = FLOAT,
		rightPlane = FLOAT,
		topPlane = FLOAT,
		bottomPlane = FLOAT
	}
end

function run()
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
