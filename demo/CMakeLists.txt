include_directories(${DemoPrj_SOURCE_DIR}/engine/include)
include_directories(${DemoPrj_SOURCE_DIR}/engine/source)
link_directories(${DemoPrj_SOURCE_DIR}/engine/libraries)

set(
    ROOT_SOURCES 
	"entry.cpp"
)

add_executable(demo ${ROOT_SOURCES})

add_compile_definitions(DEBUG=$<CONFIG:Debug>)
set_target_properties(demo PROPERTIES  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DemoPrj_SOURCE_DIR}/bin)
set_target_properties(demo PROPERTIES  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DemoPrj_SOURCE_DIR}/bin)
target_link_libraries(demo assimp.lib glfw3.lib vulkan-1.lib)
target_link_libraries(demo source)

if (DEFINED COPY_PATH)
    file(GLOB CONTENT_SRC ${DemoPrj_SOURCE_DIR}/content/*)
    add_custom_command(TARGET demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${COPY_PATH}/content)
    add_custom_command(TARGET demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CONTENT_SRC} ${COPY_PATH}/content/)
endif()