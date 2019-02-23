cmake_minimum_required( VERSION 2.6.3 )

if( DEFINED CMAKE_CROSSCOMPILING )
  # subsequent toolchain loading is not really needed
  return()
endif()

if( CMAKE_TOOLCHAIN_FILE )
  # touch toolchain variable to suppress "unused variable" warning
endif()

set( CMAKE_SYSTEM_NAME Generic )
set( CMAKE_SYSTEM_VERSION 1 )

# search for Vita SDK path 1) where this toolchain file is, 2) in environment var, 3) manually defined
if( NOT DEFINED ENV{DEVKITPRO} )
  get_filename_component(__devkitpro_path ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
  if( EXISTS "${__devkitpro_path}" )
    set( DEVKITPRO "${__devkitpro_path}" )
    set( ENV{DEVKITPRO} "${__devkitpro_path}" )
  endif()
else()
  set( DEVKITPRO "$ENV{DEVKITPRO}" )
endif()
set( DEVKITPRO "${DEVKITPRO}" CACHE PATH "Path to DEVKITPRO root" )

if( NOT EXISTS "${DEVKITPRO}" )
  message( FATAL_ERROR "Cannot find DEVKITPRO at ${DEVKITPRO}" )
endif()

set( TOOL_OS_SUFFIX "" )
if( CMAKE_HOST_WIN32 )
 set( TOOL_OS_SUFFIX ".exe" )
endif()

set(CMAKE_SYSTEM_PROCESSOR "armv8-a")
set(CMAKE_C_COMPILER   "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc${TOOL_OS_SUFFIX}"     CACHE PATH "C compiler")
set(CMAKE_CXX_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-g++${TOOL_OS_SUFFIX}"     CACHE PATH "C++ compiler")
set(CMAKE_ASM_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-as${TOOL_OS_SUFFIX}"      CACHE PATH "C++ compiler")
set(CMAKE_STRIP        "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-strip${TOOL_OS_SUFFIX}"   CACHE PATH "strip")
set(CMAKE_AR           "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-ar${TOOL_OS_SUFFIX}"      CACHE PATH "archive")
set(CMAKE_LINKER       "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-ld${TOOL_OS_SUFFIX}"      CACHE PATH "linker")
set(CMAKE_NM           "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-nm${TOOL_OS_SUFFIX}"      CACHE PATH "nm")
set(CMAKE_OBJCOPY      "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-objcopy${TOOL_OS_SUFFIX}" CACHE PATH "objcopy")
set(CMAKE_OBJDUMP      "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-objdump${TOOL_OS_SUFFIX}" CACHE PATH "objdump")
set(CMAKE_RANLIB       "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-ranlib${TOOL_OS_SUFFIX}"  CACHE PATH "ranlib")




# cache flags
set( CMAKE_CXX_FLAGS           ""                        CACHE STRING "c++ flags" )
set( CMAKE_C_FLAGS             ""                        CACHE STRING "c flags" )
set( CMAKE_CXX_FLAGS_RELEASE   "-O3 -DNDEBUG"            CACHE STRING "c++ Release flags" )
set( CMAKE_C_FLAGS_RELEASE     "-O3 -DNDEBUG"            CACHE STRING "c Release flags" )
set( CMAKE_CXX_FLAGS_DEBUG     "-O0 -g -DDEBUG -D_DEBUG" CACHE STRING "c++ Debug flags" )
set( CMAKE_C_FLAGS_DEBUG       "-O0 -g -DDEBUG -D_DEBUG" CACHE STRING "c Debug flags" )
set( CMAKE_SHARED_LINKER_FLAGS ""                        CACHE STRING "shared linker flags" )
set( CMAKE_MODULE_LINKER_FLAGS ""                        CACHE STRING "module linker flags" )
set( CMAKE_EXE_LINKER_FLAGS    "-mtp=soft -fPIE -L${DEVKITPRO}/portlibs/switch/lib -L${DEVKITPRO}/libnx/lib -specs=${DEVKITPRO}/libnx/switch.specs -g"      CACHE STRING "executable linker flags" )

# we require the relocation table
set(CMAKE_C_FLAGS "-I/opt/devkitpro/libnx/include -D__SWITCH__ -march=armv8-a -mtune=cortex-a57 -mtp=soft -ffunction-sections -fdata-sections -fPIE")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti")

# set these global flags for cmake client scripts to change behavior
set( SWITCH True )
set( BUILD_SWITCH True )

# where is the target environment
set( CMAKE_FIND_ROOT_PATH ${DEVKITPRO} ${DEVKITPRO}/devkitA64 ${DEVKITPRO}/libnx ${DEVKITPRO}/portlibs/switch "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_PREFIX}/share" )

# only search for libraries and includes in toolchain
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
