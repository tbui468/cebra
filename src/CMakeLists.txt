set(Sources
    main.c
    value.c
    statement_list.c
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
    statement_list.h
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
