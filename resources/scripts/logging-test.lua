function interface(IN,OUT)
	IN.choice = Type:Int32()
end

function run(IN,OUT)
    if IN.choice < 0 then
		rl_logError(string.format("choice < 0: %s", IN.choice))
	elseif IN.choice == 0 then
		rl_logWarn("choice == 0")
	else 
		rl_logInfo(string.format("choice > 0: %s", IN.choice))
    end
end
