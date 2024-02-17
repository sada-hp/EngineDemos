include_directories(${DemoPrj_SOURCE_DIR}/engine/include)
include_directories(${DemoPrj_SOURCE_DIR}/engine/source)
link_directories(${DemoPrj_SOURCE_DIR}/engine/libraries)

set(
    ROOT_SOURCES 
	"entry.cpp"
)

add_executable(demo ${ROOT_SOURCES})

add_compile_definitions("DEBUG=$<CONFIG:Debug>")
set_target_properties(demo PROPERTIES  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DemoPrj_SOURCE_DIR}/bin)
set_target_properties(demo PROPERTIES  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DemoPrj_SOURCE_DIR}/bin)
target_link_libraries(demo assimp.lib glfw3.lib vulkan-1.lib)
target_link_libraries(demo source)

file(GLOB SHADERS_SRC "${DemoPrj_SOURCE_DIR}/engine/shaders/*.spv")
add_custom_command(TARGET demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:demo>/shaders)
add_custom_command(TARGET demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SHADERS_SRC} $<TARGET_FILE_DIR:demo>/shaders/  DEPENDS source)

file(GLOB CONTENT_SRC "${DemoPrj_SOURCE_DIR}/content/*")
add_custom_command(TARGET demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:demo>/content)
add_custom_command(TARGET demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CONTENT_SRC} $<TARGET_FILE_DIR:demo>/content/  DEPENDS demo)

file(GLOB EXTENSIONS_SRC "${DemoPrj_SOURCE_DIR}/engine/libraries/*.dll")
add_custom_command(TARGET demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${EXTENSIONS_SRC} $<TARGET_FILE_DIR:demo>/  DEPENDS source)

file(GLOB ENGINE_SRC "${DemoPrj_SOURCE_DIR}/engine/bin/*")
add_custom_command(TARGET demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${ENGINE_SRC} $<TARGET_FILE_DIR:demo>/ DEPENDS source)