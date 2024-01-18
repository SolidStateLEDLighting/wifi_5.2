#
FILE(GLOB_RECURSE SOURCES src/*.cpp)
#
# Exposes components to both source and header files.
set(WIFI_REQUIRES
    nvs_5.2
    esp_netif
    esp_wifi
)
#
#
# Anything that must be exposed to the sources files, but may remain hidden from the header files.
set(WIFI_PRIV_REQUIRES
    main
)
#
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES ${WIFI_REQUIRES}
                       PRIV_REQUIRES ${WIFI_PRIV_REQUIRES}
                      )
