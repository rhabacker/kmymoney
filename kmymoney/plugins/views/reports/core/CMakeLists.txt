if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

set (libreports_a_SOURCES
  cashflowlist.cpp
  kreportcartesianaxis.cpp
  kreportchartview.cpp
  kreportdateaxis.cpp
  listtable.cpp
  objectinfotable.cpp
  pivotgrid.cpp
  pivottable.cpp
  querytable.cpp
  reportaccount.cpp
  reporttable.cpp
)

add_library(reports STATIC ${libreports_a_SOURCES})
target_link_libraries(reports
  PUBLIC
    $<$<BOOL:${KChart_FOUND}>:KChart>
    $<$<BOOL:${KChart6_FOUND}>:KChart6>
    Alkimia::alkimia
    Qt::PrintSupport
    kmymoney_common
    kmm_settings
    PRIVATE
    KF${QT_MAJOR_VERSION}::I18n
)

add_dependencies(reports kmm_settings)
