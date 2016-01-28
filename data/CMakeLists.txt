add_custom_command(
        OUTPUT chmsee.desktop
        COMMAND intltool-merge -d ${CMAKE_SOURCE_DIR}/po ${CMAKE_CURRENT_SOURCE_DIR}/chmsee.desktop.in chmsee.desktop
        DEPENDS chmsee.desktop.in
        )
add_custom_target(driver ALL DEPENDS chmsee.desktop default-prefs.js)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/chmsee.desktop DESTINATION share/applications)
install(FILES chmsee.mime chmsee.keys DESTINATION share/mime-info)
install(FILES chmsee-icon.png DESTINATION share/pixmaps)
install(FILES
        book-closed.png book-open.png chmsee-icon.png
        helpdoc.png hide-pane.png show-pane.png
        about-dialog.ui openfile-dialog.ui setup-window.ui
        default-prefs.js
        DESTINATION share/chmsee)

add_subdirectory(icons)
