modules("coalas")

function interface(IN,OUT)
    IN.coala = coalas.getStruct()
    OUT.coala = coalas.getStruct()
end

function run(IN,OUT)
	OUT.coala = IN.coala
end
