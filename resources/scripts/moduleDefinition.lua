local coalaModule = {}

coalaModule.coalaChief = "Alfred"

coalaModule.coalaStruct = {
    preferredFood = Type:String(),
    weight = Type:Int32()
}

function coalaModule.getStruct()
	return {
		preferredFood = Type:String(),
		weight = Type:Int32()
	}
end

coalaModule.value = {
	preferredFood = "eucalypt",
	weight = 42
}

function coalaModule.bark() 
    print("Coalas don't bark...")
end

return coalaModule