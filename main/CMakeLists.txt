set(TARGET_NAME "main")

set(SOURCES
    main.c
    platform.c
    xorgfx.c
    input.c
    flight.c
    stardust.c
    ship.c
    ship_data.c
    generation.c
    market.c
    upgrades.c
    linear.c
    intmath.c
    trig.c
    font_data.c
    dashleft.c
    dashcenter.c
    dashright.c
    stationsoi.c
    global_palette.c
)

add_exec_vxp(${TARGET_NAME}
    SRCS ${SOURCES}
    RESOURCES resources
    APP_NAME "${APP_NAME}"
    APPID "${APPID}"
    BACKGROUND ${BACKGROUND}
    API "${API}"
    RAM "${RAM}"
)

target_link_libraries(${TARGET_NAME} mreapi)
