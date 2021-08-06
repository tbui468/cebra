set(Sources
    main.c
    parser.c
    )

set(Headers
    parser.h
    common.h
    )

add_executable(
    Cebra
    ${Headers}
    ${Sources}
    )
