set(Sources
    main.c
    value.c
    parser.c
    ast.c
    compiler.c
    vm.c
    )

set(Headers
    result_code.h
    common.h
    memory.h
    token.h
    value.h
    parser.h
    ast.h
    compiler.h
    vm.h
    )

add_executable(
    Cebra
    ${Headers}
    ${Sources}
    )
