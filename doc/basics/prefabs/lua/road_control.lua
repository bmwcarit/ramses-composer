
function interface(IN,OUT)

  IN.lightSwitch = Type:Bool()
	
  OUT.lightSwitch = Type:Bool()
	
end

function run(IN,OUT)

  OUT.lightSwitch = IN.lightSwitch
	
end