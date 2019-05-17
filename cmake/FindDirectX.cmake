# - try to find DirectX include dirs and libraries

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

	if (CMAKE_CL_64)
		set (DirectX_ARCHITECTURE x64)
	else ()
		set (DirectX_ARCHITECTURE x86)
	endif ()

	set(PROGWIN64 "ProgramFiles")
    set(PROGWIN32 "ProgramFiles(x86)")

	# Find the Windows SDK
	if (DEFINED MSVC_VERSION AND NOT ${MSVC_VERSION} LESS 1700)
	
		# iterate the Windows SDK versions
		foreach(winsdk_version 10.0.18362.0 10.0.16299.0 10.0.15063.0  10.0.14393.0  10.0.10586.0  10.0.10240.0  10.0.10150.0)
		
			find_path (WIN10_SDK_ROOT_DIR
				Include/${winsdk_version}/um/windows.h
				PATHS
					"$ENV{${PROGWIN64}}/Windows Kits/10"
					"$ENV{${PROGWIN32}}/Windows Kits/10"
					DOC "Windows 10 SDK root directory"
			)
			if (WIN10_SDK_ROOT_DIR)
				set (DirectX_INC_SEARCH_PATH "${WIN10_SDK_ROOT_DIR}/Include/${winsdk_version}/um" "${WIN10_SDK_ROOT_DIR}/Include/shared")
				set (DirectX_LIB_SEARCH_PATH "${WIN10_SDK_ROOT_DIR}/Lib/${winsdk_version}/um/${DirectX_ARCHITECTURE}")
				set (DirectX_BIN_SEARCH_PATH "${WIN10_SDK_ROOT_DIR}/bin/x86" "${WIN10_SDK_ROOT_DIR}/bin/${winsdk_version}/x86")
				break()
			endif()
		
		endforeach(winsdk_version)
		
	endif ()


	find_path (DirectX_D3D11_INCLUDE_DIR d3d11.h
		PATHS ${DirectX_INC_SEARCH_PATH}
		DOC "The directory where d3d11.h resides")

	find_path (DirectX_D3DX11_INCLUDE_DIR d3dx11.h
		PATHS ${DirectX_INC_SEARCH_PATH}
		DOC "The directory where d3dx11.h resides")

	find_library (DirectX_D3D11_LIBRARY d3d11
		PATHS ${DirectX_LIB_SEARCH_PATH}
		DOC "The directory where d3d11 resides")

	find_library (DirectX_D3DX11_LIBRARY d3dx11
		PATHS ${DirectX_LIB_SEARCH_PATH} extern/x64
		DOC "The directory where d3dx11 resides")

	if (DirectX_D3D11_INCLUDE_DIR AND DirectX_D3D11_LIBRARY)
		set (DirectX_D3D11_FOUND 1)
		if (DirectX_D3DX11_INCLUDE_DIR AND DirectX_D3DX11_LIBRARY)
			set (DirectX_D3DX11_FOUND 1)
		endif ()
	endif ()

	find_path (DirectX_D3D11_1_INCLUDE_DIR d3d11_1.h
		PATHS ${DirectX_INC_SEARCH_PATH}
		DOC "The directory where d3d11_1.h resides")

	if (DirectX_D3D11_1_INCLUDE_DIR AND DirectX_D3D11_LIBRARY)
		set (DirectX_D3D11_1_FOUND 1)
	endif ()

	find_program (DirectX_FXC_EXECUTABLE fxc
		PATHS ${DirectX_BIN_SEARCH_PATH}
		DOC "Path to fxc.exe executable.")
    
	find_library (DirectX_D3DCOMPILER_LIBRARY d3dcompiler
		PATHS ${DirectX_LIB_SEARCH_PATH}
		DOC "The directory where d3dcompiler resides")
    
	find_library (DirectX_DXGI_LIBRARY dxgi
		PATHS ${DirectX_LIB_SEARCH_PATH}
		DOC "The directory where dxgi resides")
    

	mark_as_advanced (
		DirectX_D3D_INCLUDE_DIR
		DirectX_D3D_INCLUDE_DIR
		DirectX_D3DX_INCLUDE_DIR
		DirectX_D3DX_INCLUDE_DIR
		DirectX_D3DX_LIBRARY
		DirectX_D3DX_LIBRARY
		DirectX_D3D11_INCLUDE_DIR
		DirectX_D3D11_LIBRARY
		DirectX_D3DX11_INCLUDE_DIR
		DirectX_D3DX11_LIBRARY
		DirectX_D3D11_1_INCLUDE_DIR
	)

endif ()

mark_as_advanced (
	DirectX_D3D_FOUND
	DirectX_D3DX_FOUND
	DirectX_D3D11_FOUND
	DirectX_D3DX11_FOUND
	DirectX_D3D11_1_FOUND
)

# vim:set sw=4 ts=4 noet:
