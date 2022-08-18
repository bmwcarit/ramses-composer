function init()
    GLOBAL.TrafficSigns = {
        posX = Type:Float(),
        code = Type:Int32()
    }
end

function interface(IN,OUT)
    IN.RoadSigns = Type:Array(10, GLOBAL.TrafficSigns)
    OUT.TrafficSigns = Type:Array(10, GLOBAL.TrafficSigns)
end

function run(IN,OUT)
    OUT.TrafficSigns = IN.RoadSigns
end
