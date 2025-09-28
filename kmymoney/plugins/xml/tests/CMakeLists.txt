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

add_executable(test-filelocking test-filelocking.cpp)
target_link_libraries(test-filelocking PRIVATE Qt::Core Qt::Test)
if(BUILD_TESTING)
    add_test(NAME test-filelocking-local COMMAND $<TARGET_NAME:test-filelocking>)
    add_test(NAME test-filelocking-nfs COMMAND $<TARGET_NAME:test-filelocking> -- /server/daten/downloads)
endif()
#set_tests_properties(testfilelocking_nfs PROPERTIES ENVIRONMENT TESTFILELOCKING_PATH=/server/daten/downloads)
