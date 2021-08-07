set(Sources
    main.c
    parser.c
    ast.c
    )

set(Headers
    parser.h
    common.h
    ast.h
    memory.h
    token.h
    result_code.h
    )

add_executable(
    Cebra
    ${Headers}
    ${Sources}
    )
