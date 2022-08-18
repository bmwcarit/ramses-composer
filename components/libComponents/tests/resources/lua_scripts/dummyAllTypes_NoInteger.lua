function interface(IN,OUT)
  IN.propFloatIn = Type:Float()
  IN.propBoolIn = Type:Bool()

  OUT.propFloatOut = Type:Float()
  OUT.propBoolOut = Type:Bool()
end

function run(IN,OUT)
  OUT.propFloatIn = IN.propFloatOut
  OUT.propBoolIn = IN.propBoolOut
end