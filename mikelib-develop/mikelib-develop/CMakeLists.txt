cmake_minimum_required(VERSION 3.12...3.31)

project(
    mikelib
    LANGUAGES C
    VERSION 1.3.2.1
    HOMEPAGE_URL "https://mikelib.net/"
    DESCRIPTION "a general-purpose lossless data-compression library")

# ============================================================================
# CPack
# ============================================================================
set(CPACK_PACKAGE_VENDOR "mikelib-Project")
set(CPACK_PACKAGE_DESCRIPTION_FILE ${mikelib_SOURCE_DIR}/README)
set(CPACK_RESOURCE_FILE_LICENSE ${mikelib_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${mikelib_SOURCE_DIR}/README)

# ============================================================================
# configuration
# ============================================================================

option(MIKELIB_BUILD_TESTING "Enable Zlib Examples as tests" ON)
option(MIKELIB_BUILD_SHARED "Enable building mikelib shared library" ON)
option(MIKELIB_BUILD_STATIC "Enable building mikelib static library" ON)
option(MIKELIB_INSTALL "Enable installation of mikelib" ON)
option(MIKELIB_PREFIX "prefix for all types and library functions, see zconf.h.in"
       OFF)
mark_as_advanced(MIKELIB_PREFIX)

get_property(IS_MULTI GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT IS_MULTI)
    message(STATUS "No CMAKE_BUILD_TYPE set -- using Release")
    set(CMAKE_BUILD_TYPE Release)
endif(NOT DEFINED CMAKE_BUILD_TYPE AND NOT IS_MULTI)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CMakePackageConfigHelpers)
include(CheckTypeSize)
include(CPack)
include(GNUInstallDirs)

set(CPACK_INCLUDED TRUE)

if(NOT MIKELIB_CONF_WRITTEN)
    set(Z_PREFIX ${MIKELIB_PREFIX})
    set(CONF_OUT_FILE ${mikelib_BINARY_DIR}/zconf.h.cmakein)
    file(READ ${mikelib_SOURCE_DIR}/zconf.h ZCONF_CONTENT LIMIT 245)
    file(WRITE ${CONF_OUT_FILE} ${ZCONF_CONTENT})
    file(APPEND ${CONF_OUT_FILE} "#cmakedefine Z_PREFIX 1\n")
    file(APPEND ${CONF_OUT_FILE} "#cmakedefine HAVE_STDARG_H 1\n")
    file(APPEND ${CONF_OUT_FILE} "#cmakedefine HAVE_UNISTD_H 1\n")
    file(READ ${mikelib_SOURCE_DIR}/zconf.h ZCONF_CONTENT OFFSET 244)
    set(FIRST_ITEM TRUE)

    foreach(item IN LISTS ZCONF_CONTENT)
        if(FIRST_ITEM)
            string(APPEND OUT_CONTENT ${item})
            set(FIRST_ITEM FALSE)
        else(FIRST_ITEM)
            string(APPEND OUT_CONTENT "\;" ${item})
        endif(FIRST_ITEM)
    endforeach(item IN LISTS ${ZCONF_CONTENT})

    file(APPEND ${CONF_OUT_FILE} ${OUT_CONTENT})
    set(MIKELIB_CONF_WRITTEN
        TRUE
        CACHE BOOL "zconf.h.cmakein was created")
    mark_as_advanced(MIKELIB_CONF_WRITTEN)
endif(NOT MIKELIB_CONF_WRITTEN)

#
# Check to see if we have large file support
#
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
check_type_size(off64_t OFF64_T)
unset(CMAKE_REQUIRED_DEFINITIONS) # clear variable

#
# Check for fseeko
#
check_function_exists(fseeko HAVE_FSEEKO)

#
# Check for stdarg.h
#
check_include_file(stdarg.h HAVE_STDARG_H)

#
# Check for unistd.h
#
check_include_file(unistd.h HAVE_UNISTD_H)

#
# Check visibility attribute is supported
#
if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "-WX")
else(MSVC)
    set(CMAKE_REQUIRED_FLAGS "-Werror")
endif(MSVC)

check_c_source_compiles(
    "
        #include <stdlib.h>
        static void f(void) __attribute__ ((visibility(\"hidden\")));
        int main(void) {return 0;}
    "
    HAVE___ATTR__VIS_HIDDEN)

unset(CMAKE_COMPILE_FLAGS)
set(MIKELIB_PC ${mikelib_BINARY_DIR}/mikelib.pc)
configure_file(${mikelib_SOURCE_DIR}/mikelib.pc.cmakein ${MIKELIB_PC} @ONLY)
configure_file(${mikelib_BINARY_DIR}/zconf.h.cmakein ${mikelib_BINARY_DIR}/zconf.h)

# ============================================================================
# mikelib
# ============================================================================

set(MIKELIB_PUBLIC_HDRS ${mikelib_BINARY_DIR}/zconf.h mikelib.h)

set(MIKELIB_PRIVATE_HDRS
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h)

set(MIKELIB_SRCS
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c)

if(WIN32)
    set(mikelib_static_suffix "s")
    set(CMAKE_DEBUG_POSTFIX "d")
endif(WIN32)

if(MIKELIB_BUILD_SHARED)
    add_library(
        mikelib SHARED ${MIKELIB_SRCS} ${MIKELIB_PUBLIC_HDRS} ${MIKELIB_PRIVATE_HDRS}
                    $<$<OR:$<BOOL:${WIN32}>,$<BOOL:${CYGWIN}>>:win32/zlib1.rc>)
    add_library(MIKELIB::MIKELIB ALIAS mikelib)
    target_include_directories(
        mikelib
        PUBLIC $<BUILD_INTERFACE:${mikelib_BINARY_DIR}>
               $<BUILD_INTERFACE:${mikelib_SOURCE_DIR}>
               $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_compile_definitions(
        mikelib
        PRIVATE MIKELIB_BUILD
                $<$<BOOL:NOT:${HAVE_FSEEKO}>:NO_FSEEKO>
                $<$<BOOL:${HAVE___ATTR__VIS_HIDDEN}>:HAVE_HIDDEN>
                $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_DEPRECATE>
                $<$<BOOL:${MSVC}>:_CRT_NONSTDC_NO_DEPRECATE>
        PUBLIC $<$<BOOL:${HAVE_OFF64_T}>:_LARGEFILE64_SOURCE=1>)
    set(INSTALL_VERSION ${mikelib_VERSION})

    if(NOT CYGWIN)
        set_target_properties(mikelib PROPERTIES SOVERSION ${mikelib_VERSION_MAJOR}
                                              VERSION ${INSTALL_VERSION})
    endif(NOT CYGWIN)

    set_target_properties(
        mikelib
        PROPERTIES DEFINE_SYMBOL MIKELIB_DLL
                   EXPORT_NAME MIKELIB
                   OUTPUT_NAME z)
    if(UNIX
        AND NOT APPLE
        AND NOT (CMAKE_SYSTEM_NAME STREQUAL AIX)
        AND NOT (CMAKE_SYSTEM_NAME STREQUAL SunOS))
        # On unix-like platforms the library is almost always called libz
        set_target_properties(
            mikelib
            PROPERTIES LINK_FLAGS
                       "-Wl,--version-script,\"${mikelib_SOURCE_DIR}/mikelib.map\"")
    endif(
        UNIX
        AND NOT APPLE
        AND NOT (CMAKE_SYSTEM_NAME STREQUAL AIX)
        AND NOT (CMAKE_SYSTEM_NAME STREQUAL SunOS))
endif(MIKELIB_BUILD_SHARED)

if(MIKELIB_BUILD_STATIC)
    add_library(mikelibstatic STATIC ${MIKELIB_SRCS} ${MIKELIB_PUBLIC_HDRS}
                                  ${MIKELIB_PRIVATE_HDRS})
    add_library(MIKELIB::MIKELIBSTATIC ALIAS mikelibstatic)
    target_include_directories(
        mikelibstatic
        PUBLIC $<BUILD_INTERFACE:${mikelib_BINARY_DIR}>
               $<BUILD_INTERFACE:${mikelib_SOURCE_DIR}>
               $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_compile_definitions(
        mikelibstatic
        PRIVATE MIKELIB_BUILD
                $<$<BOOL:NOT:${HAVE_FSEEKO}>:NO_FSEEKO>
                $<$<BOOL:${HAVE___ATTR__VIS_HIDDEN}>:HAVE_HIDDEN>
                $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_DEPRECATE>
                $<$<BOOL:${MSVC}>:_CRT_NONSTDC_NO_DEPRECATE>
        PUBLIC $<$<BOOL:${HAVE_OFF64_T}>:_LARGEFILE64_SOURCE=1>)
    set_target_properties(
        mikelibstatic PROPERTIES EXPORT_NAME MIKELIBSTATIC OUTPUT_NAME
                                                     z${mikelib_static_suffix})
endif(MIKELIB_BUILD_STATIC)

if(MIKELIB_INSTALL)
    if(MIKELIB_BUILD_SHARED)
        install(
            TARGETS mikelib
            COMPONENT Runtime
            EXPORT mikelibSharedExport
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
        install(
            EXPORT mikelibSharedExport
            FILE MIKELIB-shared.cmake
            NAMESPACE MIKELIB::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mikelib)

        if(MSVC)
            install(
                FILES $<TARGET_PDB_FILE:mikelib>
                COMPONENT Runtime
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                CONFIGURATIONS Debug OR RelWithDebInfo
                OPTIONAL)
        endif(MSVC)
    endif(MIKELIB_BUILD_SHARED)

    if(MIKELIB_BUILD_STATIC)
        install(
            TARGETS mikelibstatic
            COMPONENT Development
            EXPORT mikelibStaticExport
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
        install(
            EXPORT mikelibStaticExport
            FILE MIKELIB-static.cmake
            NAMESPACE MIKELIB::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mikelib)
    endif(MIKELIB_BUILD_STATIC)

    configure_package_config_file(
        ${mikelib_SOURCE_DIR}/mikelibConfig.cmake.in
        ${mikelib_BINARY_DIR}/MIKELIBConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mikelib)

    write_basic_package_version_file(
        "${mikelib_BINARY_DIR}/MIKELIBConfigVersion.cmake"
        VERSION "${mikelib_VERSION}"
        COMPATIBILITY AnyNewerVersion)

    install(FILES ${mikelib_BINARY_DIR}/MIKELIBConfig.cmake
                  ${mikelib_BINARY_DIR}/MIKELIBConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mikelib)
    install(
        FILES ${MIKELIB_PUBLIC_HDRS}
        COMPONENT Development
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(
        FILES mikelib.3
        COMPONENT Docs
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man3")
    install(
        FILES LICENSE
              doc/algorithm.txt
              doc/crc-doc.1.0.pdf
              doc/rfc1950.txt
              doc/rfc1951.txt
              doc/rfc1952.txt
              doc/txtvsbin.txt
        COMPONENT Docs
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/mikelib")
    install(
        FILES ${MIKELIB_PC}
        COMPONENT Development
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif(MIKELIB_INSTALL)

# ============================================================================
# Tests
# ============================================================================
if(MIKELIB_BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif(MIKELIB_BUILD_TESTING)

add_subdirectory(contrib)
