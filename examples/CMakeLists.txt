include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/examples)
include_directories(${PROJECT_SOURCE_DIR}/arch)
include_directories(${PROJECT_SOURCE_DIR}/plugins/historydata)

set(examples_headers
    ${examples_headers}
    ${PROJECT_SOURCE_DIR}/examples/common.h
    )


if(UA_ENABLE_AMALGAMATION)
    add_definitions(-DUA_ENABLE_AMALGAMATION)
endif()

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/examples)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/examples)

macro(add_example EXAMPLE_NAME EXAMPLE_SOURCE)
  add_executable(${EXAMPLE_NAME} ${STATIC_OBJECTS} ${EXAMPLE_SOURCE} ${ARGN} ${examples_headers})
  target_link_libraries(${EXAMPLE_NAME} open62541 ${open62541_LIBRARIES})
  assign_source_group(${EXAMPLE_SOURCE})
  add_dependencies(${EXAMPLE_NAME} open62541-object)
  set_target_properties(${EXAMPLE_NAME} PROPERTIES FOLDER "open62541/examples")
  set_target_properties(${EXAMPLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  if(UA_COMPILE_AS_CXX)
    set_source_files_properties(${EXAMPLE_SOURCE} PROPERTIES LANGUAGE CXX)
  endif()
endmacro()

#########################
# Experimental Examples #
#########################

add_example(server_robolink server_robolink.c)
add_example(server_setup_motoraddr server_setup_motoraddr.c)
