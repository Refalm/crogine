cmake_minimum_required(VERSION 3.2.2)
project(cro)

set(BUILD_TEMPLATES false CACHE BOOL "Build the project templates")
set(BUILD_SCRATCHPAD false CACHE BOOL "Build the scratchpad application")
set(BUILD_TL false CACHE BOOL "Build the Threat Level sample application")

add_subdirectory(crogine)
#add_subdirectory(editor)

if(BUILD_TEMPLATES)
  #add_subdirectory(samples/multiplayer_game)
  #add_subdirectory(samples/project_template)
endif()

if(BUILD_SCRATCHPAD)
  add_subdirectory(samples/scratchpad)
endif()

if(BUILD_TL)
  add_subdirectory(samples/threat_level)
endif()