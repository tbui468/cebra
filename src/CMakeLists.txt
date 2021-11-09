set(Sources
    obj.c
    table.c
    main.c
    memory.c
    token.c
    type.c
    value.c
    lexer.c
    parser.c
    ast.c
    chunk.c
    compiler.c
    vm.c
    )

set(Headers
    obj.h
    table.h
    result_code.h
    common.h
    memory.h
    token.h
    type.h
    value.h
    lexer.h
    parser.h
    ast.h
    chunk.h
    compiler.h
    vm.h
    native.h
    error.h
    )

add_executable(
    Cebra
    ${Headers}
    ${Sources}
    )

target_link_libraries(Cebra m) #libm (for math.h) requires explicitly linking for some reason
