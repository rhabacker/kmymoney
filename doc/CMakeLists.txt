kdoctools_create_handbook(index.docbook INSTALL_DESTINATION ${KDE_INSTALL_DOCBUNDLEDIR}/en SUBDIR kmymoney)
kdoctools_create_manpage(man-kmymoney.1.docbook 1 INSTALL_DESTINATION ${KDE_INSTALL_MANDIR})

find_program(XSLTPROC_EXECUTABLE xsltproc)
if(XSLTPROC_EXECUTABLE)
    message(STATUS "${XSLTPROC_EXECUTABLE} found")
endif()

#
# docbook based documentation
#
find_package(DocBookXSL MODULE)

if(DOCBOOKXSL_DIR AND XSLTPROC_EXECUTABLE)
    # always check doc target
    add_custom_target(doc ALL)
    option(ENABLE_XML_DOCS "build XML documentation" ON)
    set(XML_DOCS_ENABLED 1)
    message(STATUS "xsltproc docbook generator found")
    add_custom_target(xmldoc)
    add_dependencies(doc xmldoc)
endif()

if(ENABLE_XML_DOCS)

#
# generate output file from docbook xml source template or file
#
# @param _target            base name for the generated file
# @param TEMPLATE <file>    docbook xml template file to generated the output from
#                           (with '@var@' variable substitution)
# @param SOURCE <file>      alternative docbook xml file to generated the output from
#                           (without variable substitution)
# @param MAN_CATEGORY <cat> category for creating man pages (also used for html output)
# @param FORMATS <formats>  list with output formats to generate ('html' and/or 'man')
# @param LOAD_TRACE         show load traces of external entities)
#
macro(add_docbook _target)
    set(options LOAD_TRACE)
    set(oneValueArgs SOURCE TEMPLATE MAN_CATEGORY)
    set(multiValueArgs FORMATS)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(_format ${ARGS_FORMATS})
        if(ARGS_TEMPLATE)
            set(_xmlfile "${CMAKE_CURRENT_BINARY_DIR}/${_target}-${_format}.xml")
            get_filename_component(_infile ${ARGS_TEMPLATE} ABSOLUTE)
            configure_file(${_infile} ${_xmlfile})
        else()
            get_filename_component(_infile ${ARGS_SOURCE} ABSOLUTE)
            set(_xmlfile ${_infile})
        endif()
        if(${_format} STREQUAL "man")
            set(_outname "${_target}.${ARGS_MAN_CATEGORY}")
            set(STYLESHEET "${DOCBOOKXSL_DIR}/manpages/docbook.xsl")
            set(INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/man/man${ARGS_MAN_CATEGORY})
        else()
            if (NOT ARGS_MAN_CATEGORY)
                set(_outname "${_target}.html")
            else()
                set(_outname "${_target}.${ARGS_MAN_CATEGORY}.html")
            endif()
            set(STYLESHEET "${DOCBOOKXSL_DIR}/html/docbook.xsl")
            set(INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/doc/dbus)
        endif()
        set(_outfile ${CMAKE_CURRENT_BINARY_DIR}/${_outname})
        if(KDOCTOOLS_CUSTOMIZATION_DIR)
            list(APPEND optional --path ${KDOCTOOLS_CUSTOMIZATION_DIR}/dtd)
        endif()
        if(ARGS_LOAD_TRACE)
            list(APPEND optional --load-trace)
        endif()
        add_custom_command(
            OUTPUT ${_outfile}
            COMMAND ${XSLTPROC_EXECUTABLE} --output ${_outfile} --nonet --xinclude --param passivetex.extensions '1' --param generate.consistent.ids '1' ${optional} ${STYLESHEET} ${_xmlfile}
            DEPENDS ${XSLTPROC_EXECUTABLE} ${_xmlfile}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
        add_custom_target(xmldoc-${_outname} DEPENDS ${_outfile})
        add_dependencies(xmldoc xmldoc-${_outname})
        install(FILES ${_outfile} DESTINATION ${INSTALL_DIR})
    endforeach()
endmacro()

#set(CONFIG_VERBOSE 1)

### copy tests to builddir so that generated tests and static tests
### are all in one place.
### todo how to add more filetypes
macro(COPYDIR _src _type)
    foreach(FILE_TYPE ${_type})
        foreach(DIR ${_src})
            file(GLOB FILES "${CMAKE_SOURCE_DIR}/${DIR}/${FILE_TYPE}" )
            file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/${DIR})
            foreach(FILE ${FILES})
                get_filename_component(FILENAME ${FILE} NAME)
                set(TARGET ${PROJECT_BINARY_DIR}/${DIR}/${FILENAME})
                configure_file(${FILE} ${TARGET} COPYONLY)
                if(CONFIG_VERBOSE)
                    message("FROM: ${FILE}\nTO: ${TARGET}\n")
                endif()
            endforeach()
        endforeach()
    endforeach()
endmacro()

# copy source files from doc directory into associated binary directory
# which is required to run generated xml docs from build directory
COPYDIR(doc *.png)
COPYDIR(doc *.svg)

set(formats html)
# if(UNIX)
#     list(APPEND formats man)
# endif()
# generate docbook output
add_docbook(kmymoney SOURCE index.docbook FORMATS ${formats})

install(FILES ${DOC_DATA} DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/kmymoney)

endif()
