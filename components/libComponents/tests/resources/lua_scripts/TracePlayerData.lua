function init()
  GLOBAL.iCustomTimer = {
    delta_time = Type:Int32(),
    timestamp_milli = Type:Int32(),
    activate_animation = Type:Bool()
  }
end

function interface(IN,OUT)
  IN.TracePlayerData = GLOBAL.iCustomTimer
  OUT.Timer = GLOBAL.iCustomTimer
end

function run(IN,OUT)
  OUT.Timer = IN.TracePlayerData
end