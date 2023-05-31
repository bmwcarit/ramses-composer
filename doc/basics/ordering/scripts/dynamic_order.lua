function interface(IN,OUT)
	IN.state = Type:Int32()
    OUT.red_order = Type:Int32()
	OUT.green_order = Type:Int32()
	OUT.main_order = Type:Int32()
	OUT.yellow_order = Type:Int32()
end

function run(IN,OUT)
	if IN.state == 0 then
		OUT.red_order = 1
		OUT.green_order = 2
		OUT.main_order = 1
		OUT.yellow_order = 2
	else
		OUT.red_order = 2
		OUT.green_order = 1
		OUT.main_order = 2
		OUT.yellow_order = 1
	end	
end
