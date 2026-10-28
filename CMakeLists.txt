cmake_minimum_required(VERSION 3.16)
project(chmsee LANGUAGES C)

set(PACKAGE_VERSION "3.1.0")

find_package(PkgConfig REQUIRED)
pkg_check_modules(CHMSEE REQUIRED IMPORTED_TARGET glib-2.0>=2.56 libxml-2.0 gtk+-3.0>=3.24)
# WebKitGTK 4.1 is the GTK3 API built against libsoup 3.
pkg_check_modules(WEBKIT REQUIRED IMPORTED_TARGET webkit2gtk-4.1)

set(CHMSEE_CONFIG_FILE "config")
set(CHMSEE_BOOKMARKS_FILE "chmsee_bookmarks")
set(CHMSEE_BOOKINFO_FILE "chmsee_bookinfo")
set(CHMSEE_NO_LINK "chmsee_no_link")
set(CHMSEE_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/chmsee")
set(CHMSEE_BOOKSHELF_DEFAULT "bookshelf")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/src/config.h @ONLY)

find_library(LIBCHM chm)
if(NOT LIBCHM)
        message(FATAL_ERROR "chmlib (libchm) is required")
endif()

find_path(LIBCHM_INCLUDE_DIR chm_lib.h)
if(NOT LIBCHM_INCLUDE_DIR)
        message(FATAL_ERROR "chmlib header (chm_lib.h) is required")
endif()
include_directories(${LIBCHM_INCLUDE_DIR})

install(FILES COPYING README DESTINATION share/doc/chmsee)
install(FILES doc/chmsee.1 DESTINATION share/man/man1)

# Set RPATH for installing
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH ${CMAKE_SYSTEM_LIBRARY_PATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(po)

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "chmsee-${PACKAGE_VERSION}")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES ".*~;/.git/;.gitignore;ID;TAGS;/gtkmoz/")

set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GTK 3 viewer for compiled HTML Help (CHM) files")
set(CPACK_PACKAGE_CONTACT "Ji YongGang <jungleji@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "doc")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/h1aji/chmsee")
# Generate Depends from the libraries linked by the installed executable.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

INCLUDE(CPack)
