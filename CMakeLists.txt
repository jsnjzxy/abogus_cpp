cmake_minimum_required(VERSION 3.14)

project(abogus_cpp
    VERSION 1.0.0
    DESCRIPTION "Douyin a_bogus signature generator"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
option(ABOGUS_BUILD_SHARED "Build shared library" OFF)
option(ABOGUS_BUILD_TESTS "Build tests" OFF)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找 nlohmann_json (header-only)
find_package(nlohmann_json 3.11.0 QUIET)

if(NOT nlohmann_json_FOUND)
    # 使用 FetchContent 仅下载，不创建 target
    include(FetchContent)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_GetProperties(json)
    if(NOT json_POPULATED)
        FetchContent_Populate(json)
    endif()
    set(NLOHMANN_JSON_INCLUDE_DIR "${json_SOURCE_DIR}/include")
endif()

# 库类型
if(ABOGUS_BUILD_SHARED)
    add_library(abogus SHARED src/abogus.cpp)
else()
    add_library(abogus STATIC src/abogus.cpp)
endif()

target_include_directories(abogus
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 添加 nlohmann_json 头文件
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(abogus PRIVATE nlohmann_json::nlohmann_json)
elseif(DEFINED NLOHMANN_JSON_INCLUDE_DIR)
    target_include_directories(abogus PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})
endif()

# 平台特定设置
if(APPLE)
    target_link_libraries(abogus PRIVATE
        "-framework Foundation"
    )
endif()

# 安装配置
include(GNUInstallDirs)

install(TARGETS abogus
    EXPORT abogus-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT abogus-targets
    FILE abogus-targets.cmake
    NAMESPACE abogus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/abogus
)

# CMake 配置文件
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/abogusConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/abogusConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/abogus
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/abogusConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/abogusConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/abogusConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/abogus
)

# 测试
if(ABOGUS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
