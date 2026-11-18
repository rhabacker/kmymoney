function(kdoctools_get_unsupported_languages subdirectory)
    set(options)
    set(oneValueArgs RESULT_VAR)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_RESULT_VAR)
        message(FATAL_ERROR
            "kdoctools_get_unsupported_languages: RESULT_VAR is required"
        )
    endif()

    set(unsupported_languages)

    file(GLOB language_dirs
        LIST_DIRECTORIES true
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/${subdirectory}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${subdirectory}/*"
    )

    foreach(language IN LISTS language_dirs)
        if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${subdirectory}/${language}"
           AND NOT EXISTS "${KDOCTOOLS_CUSTOMIZATION_DIR}/${language}/lang.entities")
            list(APPEND unsupported_languages "${language}")
        endif()
    endforeach()

    set(${ARG_RESULT_VAR} "${unsupported_languages}" PARENT_SCOPE)
endfunction()
