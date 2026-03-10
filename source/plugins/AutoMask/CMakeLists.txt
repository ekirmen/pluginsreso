add_library(ffgl-plugin-automask MODULE AutoMask.cpp AutoMaskPro.h)
add_library(ffgl::plugin::automask ALIAS ffgl-plugin-automask)

target_link_libraries(ffgl-plugin-automask PRIVATE ffgl::sdk)

set_target_properties(ffgl-plugin-automask PROPERTIES
    OUTPUT_NAME "AutoMaskPro"
    PREFIX "" # Quita el 'lib' inicial en Windows para que se llame AutoMaskPro.dll
)

install(
    TARGETS     ffgl-plugin-automask
    DESTINATION ${CMAKE_INSTALL_LIBDIR} # Aquí es donde se copiará el .dll generado
)
