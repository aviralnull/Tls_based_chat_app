cmake_minimum_required(VERSION 3.12)
project(chat_tls_system)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set paths to local dependencies
set(OPENSSL_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(OPENSSL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(OPENSSL_LIBRARIES 
    ${CMAKE_SOURCE_DIR}/lib/libssl.a
    ${CMAKE_SOURCE_DIR}/lib/libcrypto.a
)

set(BOOST_INCLUDEDIR ${CMAKE_SOURCE_DIR}/include/boost)
set(BOOST_LIBRARYDIR ${CMAKE_SOURCE_DIR}/lib)

# Find all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h*")

# Create executable
add_executable(${CHAT_TLS_SYSTEM} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${CHAT_TLS_SYSTEM} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${BOOST_INCLUDEDIR}
)

# Link libraries
target_link_libraries(${CHAT_TLS_SYSTEM} PRIVATE
    ${OPENSSL_LIBRARIES}
    ${BOOST_LIBRARYDIR}/libboost_system-mt.a
    ws2_32
    wsock32
)

# Windows-specific configurations
if(WIN32)
    # Copy all DLLs from lib folder
    file(GLOB DLL_FILES "${CMAKE_SOURCE_DIR}/lib/*.dll")
    foreach(DLL_FILE ${DLL_FILES})
        add_custom_command(TARGET ${CHAT_TLS_SYSTEM} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${DLL_FILE}"
            $<TARGET_FILE_DIR:${CHAT_TLS_SYSTEM}>)
    endforeach()

    # Copy certificate files
    foreach(CERT_FILE server.crt server.key)
        add_custom_command(TARGET ${CHAT_TLS_SYSTEM} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/${CERT_FILE}"
            $<TARGET_FILE_DIR:${CHAT_TLS_SYSTEM}>)
    endforeach()
endif()