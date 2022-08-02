function interface(IN, OUT)
    IN.testFloat = Type:Float() -- unlinked
    IN.testInt = Type:Int64() -- linked to script1
    IN.timer1 = Type:Int64() -- linked to Timer
    IN.timer2 = Type:Int64() -- linked to script1
end


function run(IN, OUT)
end
