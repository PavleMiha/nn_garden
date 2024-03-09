-- Just change solution/project name and project GUID

local PROJECT_DIR          = (path.getabsolute(".") .. "/")
local PROJECT_BUILD_DIR    = path.join(PROJECT_DIR, "build/")
local PROJECT_PROJECTS_DIR = path.join(PROJECT_DIR, "build/")
local PROJECT_RUNTIME_DIR  = path.join(PROJECT_BUILD_DIR, "bin/")
local DEPENDENCY_ROOT_DIR  = path.join(PROJECT_DIR, "3rdparty/")

BGFX_DIR        = path.join(DEPENDENCY_ROOT_DIR, "bgfx")
BX_DIR          = path.join(DEPENDENCY_ROOT_DIR, "bx")
BIMG_DIR        = path.join(DEPENDENCY_ROOT_DIR, "bimg")
GLFW_DIR 		= path.join(DEPENDENCY_ROOT_DIR, "glfw")
GLM_DIR			= path.join(DEPENDENCY_ROOT_DIR, "glm")
IMGUI_DIR		= path.join(BGFX_DIR, "3rdparty/dear-imgui")
CLIP_DIR		= path.join(DEPENDENCY_ROOT_DIR, "clip")

-- Required for bgfx and example-common
function copyLib()
end


solution "app"
	language				"C++"
	configurations			{ "Debug", "Release" }
	platforms				{ "x64" }

	defines {
		"ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR=1",
		"BX_CONFIG_ENABLE_MSVC_LEVEL4_WARNINGS=1",
		"ENTRY_CONFIG_USE_GLFW=1",
		"ENTRY_CONFIG_IMPLEMENT_MAIN=1"
	}	

	configuration "Release"
		defines "BX_CONFIG_DEBUG=0"

	configuration "Debug"
		defines "BX_CONFIG_DEBUG=1"

	configuration "windows"
		defines {
			"BGFX_CONFIG_RENDERER_DIRECT3D11=1",
		}

	configuration {}
	dofile (path.join(BX_DIR, "scripts/toolchain.lua"))

	if not toolchain(PROJECT_PROJECTS_DIR, DEPENDENCY_ROOT_DIR) then
		return -- no action specified
	end

	project "glfw"
		kind "StaticLib"
		language "C"
		files
		{
			path.join(GLFW_DIR, "include/GLFW/*.h"),
			path.join(GLFW_DIR, "src/context.c"),
			path.join(GLFW_DIR, "src/egl_context.*"),
			path.join(GLFW_DIR, "src/init.c"),
			path.join(GLFW_DIR, "src/input.c"),
			path.join(GLFW_DIR, "src/internal.h"),
			path.join(GLFW_DIR, "src/monitor.c"),
			path.join(GLFW_DIR, "src/null*.*"),
			path.join(GLFW_DIR, "src/osmesa_context.*"),
			path.join(GLFW_DIR, "src/platform.c"),
			path.join(GLFW_DIR, "src/vulkan.c"),
			path.join(GLFW_DIR, "src/window.c"),
		}
		includedirs { path.join(GLFW_DIR, "include") }
		configuration "windows"
			defines "_GLFW_WIN32"
			files
			{
				path.join(GLFW_DIR, "src/win32_*.*"),
				path.join(GLFW_DIR, "src/wgl_context.*")
			}
		configuration "linux"
			defines "_GLFW_X11"
			files
			{
				path.join(GLFW_DIR, "src/glx_context.*"),
				path.join(GLFW_DIR, "src/linux*.*"),
				path.join(GLFW_DIR, "src/posix*.*"),
				path.join(GLFW_DIR, "src/x11*.*"),
				path.join(GLFW_DIR, "src/xkb*.*")
			}
		configuration "macosx"
			defines "_GLFW_COCOA"
			files
			{
				path.join(GLFW_DIR, "src/cocoa_*.*"),
				path.join(GLFW_DIR, "src/posix_thread.h"),
				path.join(GLFW_DIR, "src/nsgl_context.h"),
				path.join(GLFW_DIR, "src/egl_context.h"),
				path.join(GLFW_DIR, "src/osmesa_context.h"),

				path.join(GLFW_DIR, "src/posix_thread.c"),
				path.join(GLFW_DIR, "src/nsgl_context.m"),
				path.join(GLFW_DIR, "src/egl_context.c"),
				path.join(GLFW_DIR, "src/nsgl_context.m"),
				path.join(GLFW_DIR, "src/osmesa_context.c"),                       
			}

		configuration "vs*"
			defines "_CRT_SECURE_NO_WARNINGS"
			buildoptions {
				"/wd 4100",--unused parameter
				"/wd 4244"--conversion
			}

	
	--location				(path.join(PROJECT_PROJECTS_DIR, _ACTION))
	--objdir					(path.join(PROJECT_BUILD_DIR, _ACTION))

	dofile (path.join(BX_DIR,	"scripts/bx.lua"))
	dofile (path.join(BIMG_DIR, "scripts/bimg.lua"))
	dofile (path.join(BIMG_DIR, "scripts/bimg_decode.lua"))
	dofile (path.join(BGFX_DIR, "scripts/bgfx.lua"))

	bgfxProject("", "StaticLib", {})

	project "entry"
		kind "StaticLib"
		language "C++"
		files {
			path.join(BGFX_DIR, "examples/common/entry/*.h"),
			path.join(BGFX_DIR, "examples/common/entry/*.cpp"),
			path.join(BGFX_DIR, "examples/common/bgfx_utils.cpp"),
			path.join(BGFX_DIR, "3rdparty/meshoptimizer/src/*.cpp"),
			path.join(BGFX_DIR, "3rdparty/meshoptimizer/src/*.h"),
		}
		includedirs {
			path.join(GLFW_DIR, "include"),
			path.join(BGFX_DIR, "3rdparty"),
			path.join(BGFX_DIR, "include"),
			path.join(BX_DIR, 	"include"),
			path.join(BIMG_DIR, "include"),
			IMGUI_DIR
		}

	project "imgui"
		kind "StaticLib"
		language "C++"
		files {
			path.join(IMGUI_DIR, "*.cpp"),
			path.join(IMGUI_DIR, "*.h"),
			path.join(IMGUI_DIR, "imgui.cpp"),
			path.join(IMGUI_DIR, "imgui_draw.cpp"),
			path.join(IMGUI_DIR, "imgui_internal.h"),
			path.join(IMGUI_DIR, "imgui_widgets.cpp"),
			path.join(IMGUI_DIR, "imgui_tables.cpp"),
			path.join(IMGUI_DIR, "imstb_rectpack.h"),
			path.join(IMGUI_DIR, "imstb_textedit.h"),
			path.join(IMGUI_DIR, "imstb_truetype.h"),
			path.join(IMGUI_DIR, "imgui_demo.cpp"),
			path.join(BGFX_DIR, "examples/common/imgui/imgui.cpp")
		}

		includedirs {
			path.join(BGFX_DIR, "3rdparty"),
			path.join(BGFX_DIR, "include"),
			path.join(BX_DIR, 	"include"),
			path.join(BIMG_DIR, "include"),
		}

		defines {
			"IMGUI_DISABLE_DEFAULT_ALLOCATORS"
		}

	project "clip"
		kind "StaticLib"
		language "C++"
		
		files {
			path.join(CLIP_DIR, "clip.cpp"),
			path.join(CLIP_DIR, "clip_win.cpp"),
			path.join(CLIP_DIR, "image.cpp"),
		}

		links {
			"shlwapi",
		}

		defines {
			"CLIP_ENABLE_IMAGE=1"
		}

	startproject "nn_garden"
	
	project "nn_garden"
		--uuid				"e0ba3c4d-338b-4517-8bbd-b29311fd6830"
		kind				"WindowedApp"

		files {
							"./src/**.cpp",
							"./include/**.h",
							"./include/**.hpp",
		}
		
		includedirs {
							"./src/",
							"./include/",
							path.join(GLFW_DIR, "include"),
							path.join(BX_DIR, "include"),
							path.join(BX_DIR, "3rdparty"),
							path.join(BGFX_DIR, "include"),
							path.join(BGFX_DIR, "3rdparty"),
							path.join(BGFX_DIR, "examples/common"),
							path.join(BIMG_DIR, "include"),
							path.join(BIMG_DIR, "3rdparty"),
							CLIP_DIR,
							IMGUI_DIR,
							GLM_DIR
		}

		links {
							"glfw",
							"bgfx",
							"bimg",
							"bimg_decode",
							"bx",
							"imgui",
							"clip",
							"entry",
							"shaderc"--not a library, but still a dependency if we need shaders
		}
	
		configuration		"Debug"
			targetsuffix	"_d"
			flags			{ "Symbols" }
			
		configuration		"Release"
			targetsuffix	"_r"
			flags			{ "Optimize" }

		configuration {}
		debugdir			(PROJECT_RUNTIME_DIR)
		targetdir			(PROJECT_RUNTIME_DIR)

		matches = os.matchfiles(path.join(PROJECT_DIR, "assets/**.sc"))

		for i,file in pairs(matches) do
			if not string.find(file, "def.") then
				local shader_type = "vertex"
				if string.find(file, "fs_") then shader_type = "fragment" end
				custombuildtask {
					{ file, PROJECT_BUILD_DIR .. "/bin/shaders/dx11/" .. path.getbasename(file) .. ".bin", { },
						{"echo "..path.join(PROJECT_DIR, "scripts/build_shader_win.bat"),
						 "echo " .. " $(<) ",
						 --"call " .. path.join(PROJECT_DIR, "scripts/build_shader_win.bat") .. " $(<) " .. shader_type
-- see https://stackoverflow.com/questions/3686837/why-are-my-custom-build-steps-not-running-in-visual-studio for why we need "call"
						}
					}
				}
			end
    	end

		configuration { "vs*" }
		buildoptions
		{
			"/wd 4127", -- Disable 'Conditional expression is constant' for do {} while(0).
			"/wd 4201", -- Disable 'Nonstandard extension used: nameless struct/union'. Used for uniforms in the project.
			"/wd 4345", -- Disable 'An object of POD type constructed with an initializer of the form () will be default-initialized'. It's an obsolete warning.
		}
		linkoptions
		{
			"/ignore:4199", -- LNK4199: /DELAYLOAD:*.dll ignored; no imports found from *.dll
		}
		links
		{ -- this is needed only for testing with GLES2/3 on Windows with VS2008
			"DelayImp",
		}

	configuration "windows"
		links
		{
			"psapi",
		}

	configuration { "vs2010" }
		linkoptions
		{ -- this is needed only for testing with GLES2/3 on Windows with VS201x
			"/DELAYLOAD:\"libEGL.dll\"",
			"/DELAYLOAD:\"libGLESv2.dll\"",
		}

	configuration {}

	group "tools"
	dofile (path.join(BGFX_DIR, "scripts/shaderc.lua"))
	dofile (path.join(BGFX_DIR, "scripts/texturec.lua"))
	dofile (path.join(BGFX_DIR, "scripts/texturev.lua"))
	dofile (path.join(BGFX_DIR, "scripts/geometryc.lua"))
	dofile (path.join(BGFX_DIR, "scripts/geometryv.lua"))
