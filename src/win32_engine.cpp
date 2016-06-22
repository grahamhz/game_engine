
/* ========================================================================
   File: win32_engine
   Date: 6.15.2016
   Revision: 
   Creator: Graham Zuber 
   Notice: (C) Copyright 2015 by Graham Zuber All Rights Reserved.
   ======================================================================== */

#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static
#define local_persist static 
#define global_variable static 

// how many bytes per pixel
const int bytesPerPixel = 4;

struct win32_offscreen_buffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
};

struct win32_window_dimensions
{
    int width;
    int height;
};

// define some global variables
global_variable bool globalRunning;
global_variable win32_offscreen_buffer globalBackBuffer;

// NOTE(graham): Here, we're getting around having to statically link the xinput library
// 1. define a macro that creates a function signature
// 2. typedef the signature of XInputGetState
// 3. define a function stub with the XInputGetState signature
// 4. set a global variable function pointer to the stub
// 5. override the meaning of win32 functions to redirect to our pointers
// if we didn't have this, we'd have a linker error conflicting with win32 library
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// 6. do it again for XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


/*
 * attempts to dynamically load the xinput libraries and set the function pointers
 * for XInputGetState and XInputSetState.
 */
internal void
Win32LoadXInput()
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if(XInputLibrary)
    {
	XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
	XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}


/*
 * returns an object containing current window dimensions
 */
internal win32_window_dimensions
Win32GetWindowDimension(HWND hwnd)
{
    win32_window_dimensions dimensions;
    RECT clientRect;
    
    GetClientRect(hwnd, &clientRect);
    dimensions.width = clientRect.right - clientRect.left;
    dimensions.height = clientRect.bottom - clientRect.top;

    return(dimensions);
}


/*
 * Renders a gradient to the screen.
 * Iterates through the bitmapMem buffer,
 * writing 32 bit pixels.
 */
internal void
Win32RenderGradient(win32_offscreen_buffer *buffer, int xOffset, int yOffset)
{
    // init relevant vars
    uint8_t *row = (uint8_t *)buffer->memory; // starting point of buffer

    // loop through buffer
    for(int y = 0; y < buffer->height; ++y)
    {
	// initialize first pixel of row
	uint32_t *pixel = (uint32_t *)row;
	for(int x = 0; x < buffer->width; ++x)
	{
	    /*
	      Pixel in Memory: BB GG RR xx
	      Pixel when Moved Out of Memory: 0x xxRRGGBB
	      Windows was on little endian architecture, but
	      they wanted it to look like RGB in CPU register,
	      so they swapped it.
	    */
	    uint8_t blue = (x + xOffset);
	    uint8_t green = (y + yOffset);
	    *pixel++ = ((green << 8) | blue);
	}

	// move to next row
	row += buffer->pitch;
    }
}

/*
 * Creates a new back buffer with the provided width and height.
 * Clears and removes the old back buffer.
 */
internal void
Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{
    // TODO(graham): Bulletproof this.
    // Maybe free after, then free first if that fails.

    // free our old buffer
    if(buffer->memory)
    {
	VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->bytesPerPixel = 4;
    
    // set the info for the buffer that we're going to create
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; // negative means it's a top-down representation
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemSize = (buffer->width * buffer->height) * buffer->bytesPerPixel;
    buffer->pitch = buffer->width * buffer->bytesPerPixel; // amount to add to move to next row in buffer

    // allocate our buffer
    buffer->memory = VirtualAlloc(0, bitmapMemSize, MEM_COMMIT, PAGE_READWRITE);
    
    // TODO(graham): clear this to black?
}

/*
 * Stretches the bits in the backbuffer from the dimensions of the previous buffer
 * to the size of the new buffer.
 */
internal void
Win32DisplayBufferInWindow(
    win32_offscreen_buffer *buffer,
    HDC deviceContext,
    int windowWidth,
    int windowHeight
    )
{
    // TODO(graham): aspect ratio fix
    // stretch the back 
    StretchDIBits(
	deviceContext,
	0, 0, windowWidth, windowHeight, // dest
	0, 0, buffer->width, buffer->height, // buffer
	buffer->memory,
	&buffer->info,
	DIB_RGB_COLORS,
	SRCCOPY);
}

/*
 * Main callback for handling window events.
 */
internal LRESULT CALLBACK
Win32MainWindowCallback(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    // default result to success
    LRESULT result = 0;

    // switch on message type
    switch(uMsg)
    {
	case WM_CLOSE:
	{
	    // TODO(graham): should this be handled like an error - respawn window?
	    globalRunning = false;
	}
	break;

	case WM_ACTIVATEAPP:
	{
	    OutputDebugStringA("WM_ACTIVATEAPP\n");
	}
	break;

	case WM_DESTROY:
	{
	    // TODO(graham): should this spawn a message to the user?
	    globalRunning = false;
	}
	break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
	    uint32_t vkCode = wParam;
	    bool wasDown = ((lParam & (1 << 30)) != 0);
	    bool isDown = ((lParam & (1 << 31)) == 0);
	    if(isDown != wasDown)
	    {
		if(vkCode == 'W')
		{
		}
		else if(vkCode == 'A')
		{
		}
		else if(vkCode == 'S')
		{
		}
		else if(vkCode == 'D')
		{
		}
		else if(vkCode == 'Q')
		{
		}
		else if(vkCode == 'E')
		{
		}
		else if(vkCode == VK_UP)
		{
		}
		else if(vkCode == VK_LEFT)
		{
		}
		else if(vkCode == VK_DOWN)
		{
		}
		else if(vkCode == VK_RIGHT)
		{
		}
		else if(vkCode == VK_ESCAPE)
		{
		}
		else if(vkCode == VK_SPACE)
		{
		}
	    }
	}
	break;
	
	case WM_PAINT:
	{
	    // TODO(graham): remove this test code
	    PAINTSTRUCT paint;
	    HDC deviceContext = BeginPaint(hwnd, &paint);
	    win32_window_dimensions dimensions = Win32GetWindowDimension(hwnd);
	    Win32DisplayBufferInWindow(
		&globalBackBuffer,
		deviceContext,
		dimensions.width,
		dimensions.height);
	    EndPaint(hwnd, &paint);
	}
	break;

	default:
	{
//	    OutputDebugStringA("WM DEFAULT\n");
	    result = DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	break;
    }

    return result;

}

/*
 * Program entry point. Creates and initializes window.
 * Begins intercepting windows messages.
 */
int CALLBACK
WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow
    )
{
    // try to load libraries
    Win32LoadXInput();
    
    // set up the window
    WNDCLASSA windowClass = {};

    // create the backbuffer
    Win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

    // initialize the window class
    windowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = "EngineWindowClass";

    if(!RegisterClassA(&windowClass))
    {
	// TODO(graham): Log for Debug
	goto Cleanup;
    }

    // create a new window
    HWND hwnd = CreateWindowExA(
	0,
	windowClass.lpszClassName,
	"Engine",
	WS_OVERLAPPEDWINDOW|WS_VISIBLE,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	0,
	0,
	hInstance,
	0);

    if(!hwnd)
    {
	// TODO(graham): Log for Debug
	goto Cleanup;
    }

    // NOTE(graham): we can just get one DeviceContext and use it forever
    // because we specified CS_OWNDC
    HDC deviceContext = GetDC(hwnd);
    
    // animation vars
    int xOffset = 0;
    int yOffset = 0;

    // start running
    globalRunning = true;
    
    // get messages
    while(globalRunning)
    {
	MSG message;
	while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
	    if(message.message == WM_QUIT)
	    {
		globalRunning = false;
	    }
	    TranslateMessage(&message);
	    DispatchMessageA(&message);
	}

	// TODO(graham): should we poll this more frequently?
	// poll the gamepads
	for(DWORD cntrlIndex = 0; cntrlIndex < XUSER_MAX_COUNT; ++cntrlIndex)
	{
	    XINPUT_STATE cntrlState;
	    if(XInputGetState(cntrlIndex, &cntrlState) != ERROR_SUCCESS)
	    {
		// NOTE(graham): controller is not available
	    }

	    // TODO(graham): look at cntrlState.dwPacketNumber to see if it increments too quickly
	    // get the controller button states
	    XINPUT_GAMEPAD *pad = &cntrlState.Gamepad;
	    bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
	    bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
	    bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
	    bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
	    bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
	    bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
	    bool leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
	    bool rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
	    bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
	    bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
	    bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
	    bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

	    int16_t stickX = pad->sThumbLX;
	    int16_t stickY = pad->sThumbLY;

	    if(aButton)
	    {
		XINPUT_VIBRATION vibe;
		vibe.wLeftMotorSpeed = 60000;
		vibe.wRightMotorSpeed = 60000;
		XInputSetState(cntrlIndex, &vibe);
	    }
	}

	Win32RenderGradient(&globalBackBuffer, xOffset, yOffset);

	win32_window_dimensions dimensions = Win32GetWindowDimension(hwnd);
	Win32DisplayBufferInWindow(
	    &globalBackBuffer,
	    deviceContext,
	    dimensions.width,
	    dimensions.height);
    }

Cleanup:
    
    OutputDebugStringA("Exiting Program.");
    return(0);
}
