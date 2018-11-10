# 此CMakeList只支持gcc和clang
cmake_minimum_required(VERSION 2.6.2)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)

# 项目名
project(font_subsetter)

# 默认构建Debug版本
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
	add_definitions("-DDEBUG -D_DEBUG -g -fno-inline -fno-omit-frame-pointer -fno-builtin")
endif(NOT CMAKE_BUILD_TYPE)

# 设置输出lib和bin的路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

# 设置include的目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src
		    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfntly
		    ${CMAKE_CURRENT_SOURCE_DIR}/src/subtly)

# 不使用异常
add_definitions(-DSFNTLY_NO_EXCEPTION)
# 设置编译器警告/报错级别
add_definitions(-D__wur=__attribute__\(\(warn_unused_result\)\) -Wall -Werror -fno-exceptions)
# 启用C++11
add_definitions(-std=c++11)
# 启用CMap和Bitmap
add_definitions(-DSFNTLY_EXPERIMENTAL)

# 查找所有文件并添加
file(GLOB SFNTLY_CORE_FILES src/sfntly/*.h src/sfntly/*.cc)
file(GLOB SFNTLY_PORT_FILES src/sfntly/port/*.h src/sfntly/port/*.cc)
file(GLOB SFNTLY_DATA_FILES src/sfntly/data/*.h src/sfntly/data/*.cc)
file(GLOB SFNTLY_MATH_FILES src/sfntly/math/*.h src/sfntly/math/*.cc)
file(GLOB SFNTLY_TABLE_COMMON_FILES src/sfntly/table/*.h src/sfntly/table/*.cc)
file(GLOB SFNTLY_TABLE_BITMAP_FILES src/sfntly/table/bitmap/*.h src/sfntly/table/bitmap/*.cc)
file(GLOB SFNTLY_TABLE_CORE_FILES src/sfntly/table/core/*.h src/sfntly/table/core/*.cc)
file(GLOB SFNTLY_TABLE_TTF_FILES src/sfntly/table/truetype/*.h src/sfntly/table/truetype/*.cc)
source_group(core FILES ${SFNTLY_CORE_FILES})
source_group(ports FILES ${SFNTLY_PORT_FILES})
source_group(data FILES ${SFNTLY_DATA_FILES})
source_group(math FILES ${SFNTLY_MATH_FILES})
source_group(table FILES ${SFNTLY_TABLE_COMMON_FILES})
source_group(table\\bitmap FILES ${SFNTLY_TABLE_BITMAP_FILES})
source_group(table\\core FILES ${SFNTLY_TABLE_CORE_FILES})
source_group(table\\truetype FILES ${SFNTLY_TABLE_TTF_FILES})
add_library(sfntly
      	    ${SFNTLY_CORE_FILES}
      	    ${SFNTLY_PORT_FILES}
      	    ${SFNTLY_DATA_FILES}
      	    ${SFNTLY_MATH_FILES}
      	    ${SFNTLY_TABLE_COMMON_FILES}
      	    ${SFNTLY_TABLE_BITMAP_FILES}
      	    ${SFNTLY_TABLE_CORE_FILES}
      	    ${SFNTLY_TABLE_TTF_FILES})

file(GLOB SUBTLY_FILES src/subtly/*.h src/subtly/*.cc)
add_library(subtly ${SUBTLY_FILES})

file(GLOB FNTSUB_FILES src/fntsub/*.h src/fntsub/*.cc)
file(GLOB FNTSUB_MAIN src/fntsub/main.cc)
list(REMOVE_ITEM FNTSUB_FILES ${FNTSUB_MAIN})
if(FNTSUB_FILES)
    add_library(fntsublib ${FNTSUB_FILES})
endif(FNTSUB_FILES)
add_executable(fntsub ${FNTSUB_MAIN})
target_link_libraries(fntsub subtly sfntly icuuc pthread)
if (FNTSUB_FILES)
    target_link_libraries(fntsub fntsublib)
endif(FNTSUB_FILES)
