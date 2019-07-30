# patch the version with the version defined in the build system
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/kmm_business.desktop
  ${CMAKE_CURRENT_BINARY_DIR}/kmm_business.desktop
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

########### next target ###############

set(kmm_business_PART_SRCS
  businessplugin.cpp
  businessdlg.cpp
)

kde4_add_ui_files(kmm_business_PART_SRCS
  businessdlg.ui
)

kde4_add_plugin(kmm_business
  ${kmm_business_PART_SRCS}
)

target_link_libraries(kmm_business
  kmm_mymoney kmm_plugin
  ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS}
  ${LIBALKIMIA_LIBRARIES}
)

########### install files ###############

install(TARGETS kmm_business
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kmm_business.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})
