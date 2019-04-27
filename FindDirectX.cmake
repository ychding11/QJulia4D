# - try to find DirectX include dirs and libraries
# - FindDirectX.cmake
# - (DirectX_D3D11_INCLUDE_DIR AND DirectX_D3D11_LIBRARY AND DirectX_D3D11_COMPILER )

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

	if (CMAKE_CL_64)
		set (DirectX_ARCHITECTURE x64)
	else ()
		set (DirectX_ARCHITECTURE x86)
	endif ()

	# From VS 2011 and Windows 8 SDK, the DirectX SDK is included as part of
	# the Windows SDK.
	#
	# See also:
	# - http://msdn.microsoft.com/en-us/library/windows/desktop/ee663275.aspx
	#
	# - http://cmake.3232098.n2.nabble.com/CMP0053-Unable-to-refer-to-ENV-PROGRAMFILES-X86-td7588994.html
	# - Hard code Widnows SDK
	set(_PF86 "ProgramFiles(x86)")
	set(_PF "ProgramFiles")
	set(WIN_SDK_ROOT_DIR "$ENV{${_PF86}}/Windows Kits/10/")
	set(WIN_SDK_VERSION "10.0.17763.0")
	if (DEFINED MSVC_VERSION AND NOT ${MSVC_VERSION} LESS 1700)
		if (WIN_SDK_ROOT_DIR)
			set (DirectX_INC_SEARCH_PATH "${WIN_SDK_ROOT_DIR}/Include/${WIN_SDK_VERSION}/um" "${WIN_SDK_ROOT_DIR}/Include/${WIN_SDK_VERSION}/shared")
			set (DirectX_LIB_SEARCH_PATH "${WIN_SDK_ROOT_DIR}/Lib/${WIN_SDK_VERSION}/um/${DirectX_ARCHITECTURE}")
			set (DirectX_BIN_SEARCH_PATH "${WIN_SDK_ROOT_DIR}/${WIN_SDK_VERSION}/${DirectX_ARCHITECTURE}")
		endif ()
	endif ()

	find_path (DirectX_D3D11_INCLUDE_DIR d3d11.h
		PATHS ${DirectX_INC_SEARCH_PATH}
		DOC "The directory where d3d11.h resides")

	find_path (DirectX_D3DX11_INCLUDE_DIR d3dx11.h
		PATHS ${DirectX_INC_SEARCH_PATH}
		DOC "The directory where d3dx11.h resides")

	find_library (DirectX_D3D11_COMPILER d3dcompiler
		PATHS ${DirectX_LIB_SEARCH_PATH}
		DOC "The directory where d3d11 resides")

	find_library (DirectX_D3D11_LIBRARY d3d11
		PATHS ${DirectX_LIB_SEARCH_PATH}
		DOC "The directory where d3d11 resides")

	find_library (DirectX_D3DX11_LIBRARY d3dx11
		PATHS ${DirectX_LIB_SEARCH_PATH}
		DOC "The directory where d3dx11 resides")

	if (DirectX_D3D11_INCLUDE_DIR AND DirectX_D3D11_LIBRARY AND DirectX_D3D11_COMPILER )
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

	mark_as_advanced (
		DirectX_D3D11_1_INCLUDE_DIR
	)
endif ()


mark_as_advanced (
	DirectX_D3D_FOUND
	DirectX_D3DX_FOUND
	DirectX_D3D8_FOUND
	DirectX_D3DX8_FOUND
	DirectX_D3D9_FOUND
	DirectX_D3DX9_FOUND
	DirectX_D3D10_FOUND
	DirectX_D3DX10_FOUND
	DirectX_D3D10_1_FOUND
	DirectX_D3D11_FOUND
	DirectX_D3DX11_FOUND
	DirectX_D3D11_1_FOUND
	DirectX_D2D1_FOUND
)

