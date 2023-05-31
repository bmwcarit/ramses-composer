
function interface(IN,OUT)

  IN.lightSwitch = Type:Bool()

  OUT.emissive = Type:Vec3f()
  
	OUT.ambientColor = Type:Vec3f()
	OUT.ambientLight = Type:Float()

end

function run(IN,OUT)

 	if IN.lightSwitch then
		OUT.emissive = { 1,1,1 }
	else
		OUT.emissive = { 0,0,0 }
	end
  
	OUT.ambientColor = { 1, 1, 1 }
	OUT.ambientLight = 1

end