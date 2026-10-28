include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
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
        PkgConfig::CHMSEE PkgConfig::WEBKIT ${LIBCHM}
        )

add_executable(chmsee main.c)
target_link_libraries(chmsee
        chmseelib
        )

install(TARGETS chmsee DESTINATION bin)
