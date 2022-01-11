cmake_minimum_required (VERSION 3.18)
project(modplug123)

add_executable(modplug123 modplug123.c)

install(TARGETS modplug123 DESTINATION bin)

OPTION(OPENMPT-MODPLUG "Link against openmpt-modplug." OFF)

find_package(PkgConfig)
pkg_check_modules(AO REQUIRED ao)
#pkg_check_modules(OPENMPT-MODPLUG libopenmpt)
pkg_check_modules(MODPLUG libmodplug)

target_link_libraries(modplug123 ${AO_LIBRARIES})
target_include_directories(modplug123 PUBLIC ${AO_INCLUDE_DIRS})
target_link_directories(modplug123 PUBLIC ${AO_LIBRARY_DIRS})

target_link_libraries(modplug123 ${MODPLUG_LIBRARIES})
target_include_directories(modplug123 PUBLIC ${MODPLUG_INCLUDE_DIRS})
target_link_directories(modplug123 PUBLIC ${MODPLUG_LIBRARY_DIRS})
