SET(folder_package_depend ${ANTTWEAKBAR_FOUND} ${MATLAB_FOUND})
SET(folder_link_libs ${ANTTWEAKBAR_LIBRARY} d3d11.lib dxgi.lib D3DCompiler.lib ${PROJECT_SOURCE_DIR}/extern/x64/d3dx11.lib ${MATLAB_ENG_LIBRARY} ${MATLAB_MX_LIBRARY} eigen3::eigen)
SET(folder_include_dirs ${ANTTWEAKBAR_INCLUDE_DIR} ${MATLAB_INCLUDE_DIR})
SET(folder_lib_depend effect-framework)

SET(current_dir_lists ${CMAKE_CURRENT_LIST_FILE})

IF(WIN32)
  SET(folder_lib_type STATIC)
ENDIF()

INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/BuildSrcFolderGenericLists.txt NO_POLICY_SCOPE)

ADD_SUBDIRECTORY(Effects)

if (CMAKE_CL_64)
add_custom_command(TARGET imagetri_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/extern/x64/AntTweakBar64.dll" "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/."
    COMMENT "copying AntTweakBar.dll..."
)
else ()
add_custom_command(TARGET imagetri_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/extern/x86/AntTweakBar.dll" "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/."
    COMMENT "copying AntTweakBar.dll..."
)
endif ()


add_custom_command(TARGET imagetri_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/matlab/getRegularGrid.m" "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/."
    COMMENT "copying getRegularGrid.m..."
)
add_custom_command(TARGET imagetri_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/matlab/updateTriangulationBasedOnMask.m" "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/."
    COMMENT "copying updateTriangulationBasedOnMask.m..."
)
add_custom_command(TARGET imagetri_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/matlab/splitTriangles.m" "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/."
    COMMENT "copying splitTriangles.m..."
)
add_custom_command(TARGET imagetri_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/matlab/GetNeighbors.m" "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/."
    COMMENT "copying GetNeighbors.m..."
)
