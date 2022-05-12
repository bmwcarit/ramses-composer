BLUBB

function interface(IN,OUT)
	IN.value = Type:Int32()
    OUT.value = Type:String()
end


function run(IN,OUT)
    OUT.value = IN.value
end
