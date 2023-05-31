
function interface(IN,OUT)

	IN.wheelFactor = Type:Float()
	IN.steeringFactor = Type:Float()

	OUT.rotationAngle = Type:Float()
	OUT.steeringAngle = Type:Float()
	
end

function run(IN,OUT)

  OUT.rotationAngle = IN.wheelFactor * 360
  OUT.steeringAngle = IN.steeringFactor * 90
  
end
