project(chmsee)
cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)

set(PACKAGE_VERSION "1.4.0" )

set(CMAKE_BUILD_TYPE Release)

find_package(PkgConfig)

pkg_check_modules (CHMSEE REQUIRED gthread-2.0>=2.22 glib-2.0>=2.22 libxml-2.0 gtk+-3.0>=3.10 nspr)
pkg_search_module(WEBKIT REQUIRED webkitgtk-3.0)

set(CHMSEE_INCLUDE_DIRS ${CHMSEE_INCLUDE_DIRS} ${WEBKIT_INCLUDE_DIRS})
set(CHMSEE_CFLAGS_OTHER ${CHMSEE_CFLAGS_OTHER} ${WEBKIT_CFLAGS_OTHER} -Wall)
set(CHMSEE_LIBRARY_DIRS ${CHMSEE_LIBRARY_DIRS} ${WEBKIT_LIBRARY_DIRS})
set(CHMSEE_LIBRARIES ${CHMSEE_LIBRARIES} ${WEBKIT_LIBRARIES} chm gcrypt)

set(CHMSEE_CONFIG_FILE "config")
set(CHMSEE_BOOKMARKS_FILE "chmsee_bookmarks")
set(CHMSEE_BOOKINFO_FILE "chmsee_bookinfo")
set(CHMSEE_NO_LINK "chmsee_no_link")
set(CHMSEE_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/chmsee")
set(CHMSEE_BOOKSHELF_DEFAULT "bookshelf")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/src/config.h @ONLY)

find_program(INTLTOOL_MERGE intltool-merge)
if(${INTLTOOL_MERGE} STREQUAL "INTLTOOL_MERGE-NOTFOUND")
        message(FATAL_ERROR "intltool-merge not found, you can find one in http://ftp.gnome.org/pub/gnome/sources/intltool/")
endif()

find_library(LIBCHM chm)
if(${LIBCHM} STREQUAL "LIBCHM-NOTFOUND")
        message(FATAL_ERROR "libchm not found, you can find one in http://www.jedrea.com/chmlib/")
endif()

find_library(LIBGCRYPT gcrypt)
if(${LIBGCRYPT} STREQUAL "LIBGCRYPT-NOTFOUND")
        message(FATAL_ERROR "libgcrypt not found, you can find one in ftp://ftp.gnupg.org/gcrypt/libgcrypt/")
endif()

# Set RPATH for installing
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH ${CMAKE_SYSTEM_LIBRARY_PATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(po)

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "chmsee-1.4.0")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES ".*~;/.git/;.gitignore;ID;TAGS;/gtkmoz/")

INCLUDE(CPack)
