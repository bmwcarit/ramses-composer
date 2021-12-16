local coalaModule = {}

coalaModule.coalaChief = "Alfred"

coalaModule.coalaStruct = {
    preferredFood = STRING,
    weight = INT
}

function coalaModule.bark() 
    print("Coalas don't bark...")
end

return coalaModule