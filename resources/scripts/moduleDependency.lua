modules("coalas")

function interface(IN,OUT)
    OUT.coala = coalas.getStruct()
end

function run(IN,OUT)
    OUT.coala = coalas.value
end
