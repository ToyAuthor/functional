# Name of project.
# 決定生出來的專案檔名稱
project(functional)

cmake_minimum_required(VERSION 2.8)

# Release or debug. Up to you.
# 直接在這裡替IDE做決定
set(CMAKE_BUILD_TYPE Release)

# Declare a variable.
# 當成一個變數看待就行了
set(NAME "functional")

# It can be ignored.
# 專案當下的資料夾其實已經是預設搜尋路徑了
include_directories(${PROJECT_SOURCE_DIR})

# Where you put that product.
# 決定編譯出來的執行檔要放哪裡
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

# Make your binary.
# 決定編譯所需的程式碼以及執行檔名稱
add_executable(${NAME} main.cpp)

# Make Visual Studio stop from creating debug directory or release directory.
# 阻止Visual Studio創造debug跟release資料夾，資料夾結構完全用CMake決定就好
if(MSVC)
	set_target_properties(${NAME} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY            ${PROJECT_BINARY_DIR}
		RUNTIME_OUTPUT_DIRECTORY_DEBUG      ${PROJECT_BINARY_DIR}
		RUNTIME_OUTPUT_DIRECTORY_RELEASE    ${PROJECT_BINARY_DIR}
		ARCHIVE_OUTPUT_DIRECTORY            ${PROJECT_BINARY_DIR}
		ARCHIVE_OUTPUT_DIRECTORY_DEBUG      ${PROJECT_BINARY_DIR}
		ARCHIVE_OUTPUT_DIRECTORY_RELEASE    ${PROJECT_BINARY_DIR}
	)
endif()
