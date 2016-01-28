include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CHMSEE_INCLUDE_DIRS}
        )
add_definitions(
        ${CHMSEE_CFLAGS_OTHER}
        )

link_directories(
        ${CHMSEE_LIBRARY_DIRS}
        )

set(CHMSEE_SRCS
        chmsee.h chmsee.c
        setup.h setup.c
        utils.h utils.c
        )
set(CHMSEE_SRCS
        ${CHMSEE_SRCS}
        components/book.h components/book.c
        components/toc.h components/toc.c
        components/index.h components/index.c
        components/bookmarks.h components/bookmarks.c
        components/treeview.h components/treeview.c
        components/html-webkit.h components/html-webkit.c
        )
set(CHMSEE_SRCS
        ${CHMSEE_SRCS}
        models/bookmarksfile.h models/bookmarksfile.c
        models/chmfile.h models/chmfile.c
        models/parser.h models/parser.c
        models/link.h models/link.c
        )

add_library(chmseelib STATIC ${CHMSEE_SRCS})
target_link_libraries(chmseelib
        ${CHMSEE_LIBRARIES}
        )

add_executable(chmsee main.c)
target_link_libraries(chmsee
        chmseelib
        )
set_target_properties(chmsee
        PROPERTIES LINKER_LANGUAGE CXX
        )

install(TARGETS chmsee DESTINATION bin)
