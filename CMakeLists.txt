cmake_minimum_required (VERSION 3.18)
project(modplug123)

add_subdirectory(mp123)

INSTALL(FILES modplug123.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
