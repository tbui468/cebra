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
    )

add_executable(
    Cebra
    ${Headers}
    ${Sources}
    )
