
cmake_minimum_required(VERSION 2.7)
project(PixilePython)
set(PYTHON_PATH ${CMAKE_SYSROOT}/usr/include/python3.4/)
set(LIBRARIES_FROM_REFERENCES "")
add_library(PixilePython 
source/dllmain.cpp
source/CScriptLanguagePython.cpp
source/CScriptPython.cpp
source/Python_Attribute.cpp
source/Python_AttributeManager.cpp
source/Python_CAtrObj.cpp
source/Python_Vector.cpp
)
target_link_libraries(PixilePython "${LIBRARIES_FROM_REFERENCES}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty/vcpkg/installed/raspberrypi/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../PixileStudio/ns/include ${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty/urdl/include ${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty/RakNet/Source ${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty/gwen/include ${CMAKE_CURRENT_SOURCE_DIR}/../../PixileStudio/Engine/source)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty/squirrel/3.0.4e/include ${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty/sqrat/0.8.2/include ${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty/freetype/2.4.10/include)
include_directories(${PYTHON_PATH})

#include_directories(${RTIMULib})
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Thirdparty/angle/include ${CMAKE_CURRENT_SOURCE_DIR}/../Thirdparty/angle/util ${CMAKE_CURRENT_SOURCE_DIR}/../Thirdparty/angle/src)
#include_directories(/home/adam/rasp_root/sysroot/opt/vc/include)

#include_directories(/usr/local/include) 		
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive")
add_definitions(-DRASPBERRYPI -DHAL_QUIET -DMONGO_HAVE_STDINT -DZIP_STD -DFREEIMAGE_LIB)#