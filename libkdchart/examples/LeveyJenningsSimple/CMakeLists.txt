set(AUTORCC 1)

include_directories(../../include/KDChart)
set(SOURCES main.cpp)
QT4_ADD_RESOURCES(SOURCES images.qrc)
add_executable(leveyjenningssimple ${SOURCES})
target_link_libraries(leveyjenningssimple ${QT_LIBRARIES} kmm_kdchart)
