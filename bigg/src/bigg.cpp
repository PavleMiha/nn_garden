/*
 * This is free and unencumbered software released into the public domain. 
 */

#include <bigg.hpp>

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#	define GLFW_EXPOSE_NATIVE_X11
#	define GLFW_EXPOSE_NATIVE_GLX
#elif BX_PLATFORM_OSX
#	define GLFW_EXPOSE_NATIVE_COCOA
#	define GLFW_EXPOSE_NATIVE_NSGL
#elif BX_PLATFORM_WINDOWS
#	define GLFW_EXPOSE_NATIVE_WIN32
#	define GLFW_EXPOSE_NATIVE_WGL
#endif // BX_PLATFORM_
#include <bx/math.h>
#include <bgfx/platform.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <fstream>

#include "bigg_assets.h"
#include "bigg_shaders.hpp"
#include "bigg_imgui.hpp"

// bgfx utils

const bgfx::Memory* bigg::loadMemory( const char* filename )
{
	std::ifstream file( filename, std::ios::binary | std::ios::ate );
	std::streamsize size = file.tellg();
	file.seekg( 0, std::ios::beg );
	const bgfx::Memory* mem = bgfx::alloc( uint32_t( size + 1 ) );
	if ( file.read( ( char* )mem->data, size ) )
	{
		mem->data[ mem->size - 1 ] = '\0';
		return mem;
	}
	return nullptr;
}

bgfx::ShaderHandle bigg::loadShader( const char* shader )
{
	return bgfx::createShader( loadMemory( shader ) );
}

bgfx::ProgramHandle bigg::loadProgram( const char* vsName, const char* fsName )
{
	bgfx::ShaderHandle vs = loadShader( vsName );
	bgfx::ShaderHandle fs = loadShader( fsName );
	return bgfx::createProgram( vs, fs, true );
}

// application

ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int key)
{
	switch (key)
	{
	case GLFW_KEY_TAB: return ImGuiKey_Tab;
	case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
	case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
	case GLFW_KEY_UP: return ImGuiKey_UpArrow;
	case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
	case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
	case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
	case GLFW_KEY_HOME: return ImGuiKey_Home;
	case GLFW_KEY_END: return ImGuiKey_End;
	case GLFW_KEY_INSERT: return ImGuiKey_Insert;
	case GLFW_KEY_DELETE: return ImGuiKey_Delete;
	case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
	case GLFW_KEY_SPACE: return ImGuiKey_Space;
	case GLFW_KEY_ENTER: return ImGuiKey_Enter;
	case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
	case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
	case GLFW_KEY_COMMA: return ImGuiKey_Comma;
	case GLFW_KEY_MINUS: return ImGuiKey_Minus;
	case GLFW_KEY_PERIOD: return ImGuiKey_Period;
	case GLFW_KEY_SLASH: return ImGuiKey_Slash;
	case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
	case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
	case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
	case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
	case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
	case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
	case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
	case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
	case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
	case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
	case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
	case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
	case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
	case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
	case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
	case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
	case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
	case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
	case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
	case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
	case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
	case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
	case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
	case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
	case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
	case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
	case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
	case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
	case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
	case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
	case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
	case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
	case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
	case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
	case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
	case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
	case GLFW_KEY_MENU: return ImGuiKey_Menu;
	case GLFW_KEY_0: return ImGuiKey_0;
	case GLFW_KEY_1: return ImGuiKey_1;
	case GLFW_KEY_2: return ImGuiKey_2;
	case GLFW_KEY_3: return ImGuiKey_3;
	case GLFW_KEY_4: return ImGuiKey_4;
	case GLFW_KEY_5: return ImGuiKey_5;
	case GLFW_KEY_6: return ImGuiKey_6;
	case GLFW_KEY_7: return ImGuiKey_7;
	case GLFW_KEY_8: return ImGuiKey_8;
	case GLFW_KEY_9: return ImGuiKey_9;
	case GLFW_KEY_A: return ImGuiKey_A;
	case GLFW_KEY_B: return ImGuiKey_B;
	case GLFW_KEY_C: return ImGuiKey_C;
	case GLFW_KEY_D: return ImGuiKey_D;
	case GLFW_KEY_E: return ImGuiKey_E;
	case GLFW_KEY_F: return ImGuiKey_F;
	case GLFW_KEY_G: return ImGuiKey_G;
	case GLFW_KEY_H: return ImGuiKey_H;
	case GLFW_KEY_I: return ImGuiKey_I;
	case GLFW_KEY_J: return ImGuiKey_J;
	case GLFW_KEY_K: return ImGuiKey_K;
	case GLFW_KEY_L: return ImGuiKey_L;
	case GLFW_KEY_M: return ImGuiKey_M;
	case GLFW_KEY_N: return ImGuiKey_N;
	case GLFW_KEY_O: return ImGuiKey_O;
	case GLFW_KEY_P: return ImGuiKey_P;
	case GLFW_KEY_Q: return ImGuiKey_Q;
	case GLFW_KEY_R: return ImGuiKey_R;
	case GLFW_KEY_S: return ImGuiKey_S;
	case GLFW_KEY_T: return ImGuiKey_T;
	case GLFW_KEY_U: return ImGuiKey_U;
	case GLFW_KEY_V: return ImGuiKey_V;
	case GLFW_KEY_W: return ImGuiKey_W;
	case GLFW_KEY_X: return ImGuiKey_X;
	case GLFW_KEY_Y: return ImGuiKey_Y;
	case GLFW_KEY_Z: return ImGuiKey_Z;
	case GLFW_KEY_F1: return ImGuiKey_F1;
	case GLFW_KEY_F2: return ImGuiKey_F2;
	case GLFW_KEY_F3: return ImGuiKey_F3;
	case GLFW_KEY_F4: return ImGuiKey_F4;
	case GLFW_KEY_F5: return ImGuiKey_F5;
	case GLFW_KEY_F6: return ImGuiKey_F6;
	case GLFW_KEY_F7: return ImGuiKey_F7;
	case GLFW_KEY_F8: return ImGuiKey_F8;
	case GLFW_KEY_F9: return ImGuiKey_F9;
	case GLFW_KEY_F10: return ImGuiKey_F10;
	case GLFW_KEY_F11: return ImGuiKey_F11;
	case GLFW_KEY_F12: return ImGuiKey_F12;
	default: return ImGuiKey_None;
	}
}

void bigg::Application::updateKeyModifiers(GLFWwindow* window)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddKeyEvent(ImGuiMod_Ctrl, (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS));
	io.AddKeyEvent(ImGuiMod_Shift, (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS));
	io.AddKeyEvent(ImGuiMod_Alt, (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS));
	io.AddKeyEvent(ImGuiMod_Super, (glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS));
}

void bigg::Application::keyCallback( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	if (action != GLFW_PRESS && action != GLFW_RELEASE)
		return;

	ImGuiIO& io = ImGui::GetIO();

	updateKeyModifiers(window);

	ImGuiKey imgui_key = ImGui_ImplGlfw_KeyToImGuiKey(key);
	io.AddKeyEvent(imgui_key, (action == GLFW_PRESS));
	//io.SetKeyEventNativeData(imgui_key, key, scancode);

	if ( !io.WantCaptureKeyboard )
	{
		bigg::Application* app = ( bigg::Application* )glfwGetWindowUserPointer( window );
		app->onKey( key, scancode, action, mods );
	}

}

void bigg::Application::charCallback( GLFWwindow* window, unsigned int codepoint )
{
	bigg::Application* app = ( bigg::Application* )glfwGetWindowUserPointer( window );
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharacter( codepoint );
	app->onChar( codepoint );
}

void bigg::Application::charModsCallback( GLFWwindow* window, unsigned int codepoint, int mods )
{
	bigg::Application* app = ( bigg::Application* )glfwGetWindowUserPointer( window );
	app->onCharMods( codepoint, mods );
}

void bigg::Application::mouseButtonCallback( GLFWwindow* window, int button, int action, int mods )
{
	updateKeyModifiers(window);

	ImGuiIO& io = ImGui::GetIO();
	if (button >= 0 && button < ImGuiMouseButton_COUNT)
		io.AddMouseButtonEvent(button, action == GLFW_PRESS);

	if ( !io.WantCaptureMouse )
	{
		bigg::Application* app = ( bigg::Application* )glfwGetWindowUserPointer( window );
		app->onMouseButton( button, action, mods );
	}
}

void bigg::Application::cursorPosCallback( GLFWwindow* window, double xpos, double ypos )
{
	bigg::Application* app = ( bigg::Application* )glfwGetWindowUserPointer( window );
	
	ImGuiIO& io = ImGui::GetIO();
	io.AddMousePosEvent((float)xpos, (float)ypos);

	app->onCursorPos( xpos, ypos );
}

void bigg::Application::cursorEnterCallback( GLFWwindow* window, int entered )
{
	bigg::Application* app = ( bigg::Application* )glfwGetWindowUserPointer( window );
	app->onCursorEnter( entered );
}

void bigg::Application::scrollCallback( GLFWwindow* window, double xoffset, double yoffset )
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseWheelEvent((float)xoffset, (float)yoffset);

	if ( !io.WantCaptureMouse )
	{
		bigg::Application* app = ( bigg::Application* )glfwGetWindowUserPointer( window );
		app->mMouseWheelH += ( float )xoffset;
		app->mMouseWheel += ( float )yoffset;
		app->onScroll( xoffset, yoffset );
	}
}

void bigg::Application::dropCallback( GLFWwindow* window, int count, const char** paths )
{
	bigg::Application* app = ( bigg::Application* )glfwGetWindowUserPointer( window );
	app->onDrop( count, paths );
}

void bigg::Application::windowSizeCallback( GLFWwindow* window, int width, int height )
{
	bigg::Application* app = ( bigg::Application* )glfwGetWindowUserPointer( window );
	app->mWidth = width;
	app->mHeight = height;
	app->reset( app->mReset );
	app->onWindowSize( width, height );
}

bigg::Application::Application( const char* title, uint32_t width, uint32_t height )
{
	mWidth = width;
	mHeight = height;
	mTitle = title;
}

int bigg::Application::run( int argc, char** argv, bgfx::RendererType::Enum type, uint16_t vendorId, uint16_t deviceId, bgfx::CallbackI* callback, bx::AllocatorI* allocator )
{
	// Initialize the glfw
	if ( !glfwInit() )
	{
		return -1;
	}

	// Create a window
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	mWindow = glfwCreateWindow( getWidth(), getHeight(), getTitle(), NULL, NULL );
	if ( !mWindow )
	{
		glfwTerminate();
		return -1;
	}

	// Setup input callbacks
	glfwSetWindowUserPointer( mWindow, this );
	glfwSetKeyCallback( mWindow, keyCallback );
	glfwSetCharCallback( mWindow, charCallback );
	glfwSetCharModsCallback( mWindow, charModsCallback );
	glfwSetMouseButtonCallback( mWindow, mouseButtonCallback );
	glfwSetCursorPosCallback( mWindow, cursorPosCallback );
	glfwSetCursorEnterCallback( mWindow, cursorEnterCallback );
	glfwSetScrollCallback( mWindow, scrollCallback );
	glfwSetDropCallback( mWindow, dropCallback );
	glfwSetWindowSizeCallback( mWindow, windowSizeCallback );

	// Setup bgfx
	bgfx::PlatformData platformData;
	memset( &platformData, 0, sizeof( platformData ) );
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
	platformData.nwh = ( void* )( uintptr_t )glfwGetX11Window( mWindow );
	platformData.ndt = glfwGetX11Display();
#elif BX_PLATFORM_OSX
	platformData.nwh = glfwGetCocoaWindow( mWindow );
#elif BX_PLATFORM_WINDOWS
	platformData.nwh = glfwGetWin32Window( mWindow );
#endif // BX_PLATFORM_
	bgfx::setPlatformData( platformData );

	// Init bgfx
	bgfx::Init init;
	init.type = type;
	init.vendorId = vendorId;
	init.deviceId = deviceId;
	init.callback = callback;
	init.allocator = allocator;
	bgfx::init( init );

	// Setup ImGui
	imguiInit( mWindow );

	// Initialize the application
	reset();
	initialize( argc, argv );

	// Loop until the user closes the window
	float lastTime = 0;
	float dt;
	float time;
	while ( !glfwWindowShouldClose( mWindow ) )
	{
		time = ( float )glfwGetTime();
		dt = time - lastTime;
		lastTime = time;

		glfwPollEvents();
		imguiEvents( dt );
		ImGui::NewFrame();
		update( dt );
		ImGui::Render();
		imguiRender( ImGui::GetDrawData() );
		bgfx::frame();
	}

	// Shutdown application and glfw
	int ret = shutdown();
	imguiShutdown();
	bgfx::shutdown();
	glfwTerminate();
	return ret;
}

void bigg::Application::reset( uint32_t flags )
{
	mReset = flags;
	bgfx::reset( mWidth, mHeight, mReset );
	imguiReset( uint16_t( getWidth() ), uint16_t( getHeight() ) );
	onReset();
}

uint32_t bigg::Application::getWidth() const
{
	return mWidth;
}

uint32_t bigg::Application::getHeight() const
{
	return mHeight;
}

void bigg::Application::setSize( int width, int height )
{
	glfwSetWindowSize( mWindow, width, height );
}

const char* bigg::Application::getTitle() const
{
	return mTitle;
}

void bigg::Application::setTitle( const char* title )
{
	mTitle = title;
	glfwSetWindowTitle( mWindow, title );
}

bool bigg::Application::isKeyDown( int key ) const
{
	ImGuiIO& io = ImGui::GetIO();
	if ( key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST || io.WantCaptureKeyboard )
	{
		return false;
	}

	return glfwGetKey( mWindow, key ) == GLFW_PRESS;
}

bool bigg::Application::isMouseButtonDown( int button ) const
{
	ImGuiIO& io = ImGui::GetIO();
	if ( button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST || io.WantCaptureMouse )
	{
		return false;
	}
	
	return glfwGetMouseButton( mWindow, button ) == GLFW_PRESS;
}

float bigg::Application::getMouseWheelH() const
{
	return mMouseWheelH;
}

float bigg::Application::getMouseWheel() const
{
	return mMouseWheel;
}
