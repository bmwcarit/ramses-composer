
function interface()
	IN.choice = INT
    IN.value = FLOAT
    
    OUT.value = STRING
end


function run()
    if IN.choice > 0 then
        OUT.value = IN.value
        error("WUFF")
    end
end
