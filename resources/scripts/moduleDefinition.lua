local coalaModule = {}

coalaModule.coalaChief = "Alfred"

coalaModule.coalaStruct = {
    preferredFood = STRING,
    weight = INT
}

function coalaModule.getStruct()
	return {
		preferredFood = STRING,
		weight = INT
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