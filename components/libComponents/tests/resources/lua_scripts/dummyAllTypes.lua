function interface(IN,OUT)
  IN.propIntegerIn = Type:Int32()
  IN.propFloatIn = Type:Float()
  IN.propBoolIn = Type:Bool()

  OUT.propIntegerOut = Type:Int32()
  OUT.propFloatOut = Type:Float()
  OUT.propBoolOut = Type:Bool()
end

function run(IN,OUT)
  OUT.propIntegerOut = IN.propIntegerIn
  OUT.propFloatOut = IN.propFloatIn
  OUT.propBoolOut = IN.propBoolIn
end