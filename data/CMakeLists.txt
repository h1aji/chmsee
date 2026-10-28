configure_file(chmsee.desktop.in chmsee.desktop COPYONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/chmsee.desktop DESTINATION share/applications)
install(FILES chmsee.xml DESTINATION share/mime/packages)
install(FILES chmsee-icon.png DESTINATION share/pixmaps)
install(FILES
        book-closed.png book-open.png chmsee-icon.png
        helpdoc.png hide-pane.png show-pane.png
        about-dialog.ui openfile-dialog.ui setup-window.ui
        default-prefs.js
        DESTINATION share/chmsee)

add_subdirectory(icons)
