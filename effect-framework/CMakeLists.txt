SET(folder_package_depend )
SET(folder_link_libs )
SET(folder_include_dirs )
SET(folder_lib_depend )

SET(current_dir_lists ${CMAKE_CURRENT_LIST_FILE})

IF(WIN32)
  SET(folder_lib_type STATIC)
ENDIF()

INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/BuildSrcFolderGenericLists.txt NO_POLICY_SCOPE)