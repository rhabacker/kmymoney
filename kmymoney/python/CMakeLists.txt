find_package(SWIG REQUIRED)
include(UseSWIG)
find_package(Python3 COMPONENTS Development Interpreter REQUIRED)

set_source_files_properties(kmymoney_python.i PROPERTIES CPLUSPLUS ON)

swig_add_library(kmymoney_python
    TYPE MODULE
    LANGUAGE python
    SOURCES
        kmymoney_python.i
        transactionbuilder.cpp
)

set_property(TARGET kmymoney_python PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON)

target_include_directories(kmymoney_python
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../mymoney
)

target_link_libraries(kmymoney_python
    PRIVATE
        kmm_mymoney
        Qt::Core
        Python3::Python
)

install(TARGETS kmymoney_python LIBRARY DESTINATION ${KDE_INSTALL_LIBDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kmymoney_python.py DESTINATION ${KDE_INSTALL_DATADIR}/kmymoney/python)

configure_file(test_transaction.py.cmake test_transaction.py)
add_test(
    NAME test-python-transaction
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_transaction.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)

