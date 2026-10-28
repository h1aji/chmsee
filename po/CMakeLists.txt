set(CATALOGS bn ca cs da de el en_GB es fr he id it ms nb pl pt pt_BR ro ru sk sv sw uk vi zh_CN zh_TW)
set(UPDATE_PO_TARGETS )
set(COMPILE_PO_TARGETS )

foreach(catalog ${CATALOGS})
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${catalog}/LC_MESSAGES")

        add_custom_command(
                OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${catalog}/LC_MESSAGES/chmsee.mo
                COMMAND msgfmt -c -o
                        ${CMAKE_CURRENT_BINARY_DIR}/${catalog}/LC_MESSAGES/chmsee.mo
                        ${CMAKE_CURRENT_SOURCE_DIR}/${catalog}.po
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${catalog}.po)

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${catalog}/LC_MESSAGES/chmsee.mo
                DESTINATION share/locale/${catalog}/LC_MESSAGES)

        set(COMPILE_PO_TARGETS ${COMPILE_PO_TARGETS} ${CMAKE_CURRENT_BINARY_DIR}/${catalog}/LC_MESSAGES/chmsee.mo)
endforeach()

add_custom_target(compile-po
        ALL
        DEPENDS ${COMPILE_PO_TARGETS})
