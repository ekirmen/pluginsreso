add_library(ffgl-plugin-alphaedge STATIC)
add_library(ffgl::plugin::alphaedge ALIAS ffgl-plugin-alphaedge)
target_sources(ffgl-plugin-alphaedge PRIVATE AlphaEdge.h AlphaEdge.cpp)
target_link_libraries(ffgl-plugin-alphaedge PRIVATE ffgl::sdk)

install(
    TARGETS     ffgl-plugin-alphaedge
    EXPORT      ffgl-plugin-alphaedge-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
