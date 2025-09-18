set(mymoneystoragexml_SOURCES
  ../mymoneystoragenames.cpp
  ../mymoneyxmlreader.cpp
  ../mymoneyxmlwriter.cpp
  )

add_library(mymoneystoragexml STATIC ${mymoneystoragexml_SOURCES})
target_link_libraries(mymoneystoragexml
  PUBLIC
    Qt::Xml
    KF${QT_MAJOR_VERSION}::I18n
    onlinetask_interfaces
    kmm_mymoney
  PRIVATE
    xmlstoragehelper
)

add_definitions(-DCMAKE_CURRENT_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\" -DCMAKE_CURRENT_BINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\")
file(GLOB tests_sources "*-test.cpp")
kmm_add_tests(${tests_sources}
  LINK_LIBRARIES
    Qt::Test
    mymoneystoragexml
    kmm_testutils
    kmm_testutilities
)

add_executable(testfilelocking testfilelocking.cpp)
target_link_libraries(testfilelocking PRIVATE Qt::Core Qt::Test)
add_test(NAME testfilelocking_local COMMAND $<TARGET_NAME:testfilelocking>)
add_test(NAME testfilelocking_nfs COMMAND $<TARGET_NAME:testfilelocking>)
set_tests_properties(testfilelocking_nfs PROPERTIES ENVIRONMENT TESTFILELOCKING_PATH=/server/daten/downloads)
