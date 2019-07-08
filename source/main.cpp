#include "software_renderer.h"
#include "types.h"
#include "my_math.h" // v2
#include "logging.h"
#include "profiling.h"

#include <windows.h>
#include <time.h> // clock

#include <stdio.h> // file io for printing time

static HWND window_handle = 0;
static HDC DIB_handle;
static HBITMAP DIB_bitmap;
static int DIB_width;
static int DIB_height;
static int DIB_row_byte_width;
static u32 *frame_buffer;
static bool running;

#define MAX_BUTTONS 165
static bool key_states[MAX_BUTTONS] = {};
static bool mouse_states[8] = {};


bool key_state(u32 button)
{
  if(button < 0 || button > MAX_BUTTONS) return false;

  return key_states[button];
}

bool mouse_state(u32 button)
{
  if(button < 0 || button > 8) return false;

  return mouse_states[button];
}

v2 mouse_window_position()
{
  POINT p;
  if(GetCursorPos(&p))
  {
    ScreenToClient(window_handle, &p);
    return v2((float)p.x, (float)p.y);
  }

  return v2();
}




LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

  switch (message)
  {
    
    case WM_SIZE:
    {
    }
    break;
    
    case WM_DESTROY:
    {
      running = false;
      PostQuitMessage(0);
      return 0;
    }
    break;

    case WM_CLOSE: 
    {
      running = false;
      DestroyWindow(window);
      return 0;
    }  
    break;
    
    case WM_PAINT:
    {
      ValidateRect(window_handle, 0);
    }
    break;

    case WM_KEYDOWN: 
    {
      key_states[wParam] = true;
    }
    break;

    case WM_KEYUP:
    {
      key_states[wParam] = false;
    }
    break;

    case WM_LBUTTONDOWN:
    {
      mouse_states[0] = true;
    }
    break;

    case WM_LBUTTONUP:
    {
      mouse_states[0] = false;
    }
    break;
    
    default:
    {
      result = DefWindowProc(window, message, wParam, lParam);
    }
    break;
  }
  
  return result;
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  // Create the window class
  WNDCLASS window_class = {};

  window_class.style = CS_HREDRAW|CS_VREDRAW;
  window_class.lpfnWndProc = WindowProc;
  window_class.hInstance = hInstance;
  window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
  window_class.lpszClassName = "Windows Program Class";

  if(!RegisterClass(&window_class))
  {
    return 1;
  }

  
  u32 monitor_width = GetSystemMetrics(SM_CXSCREEN);
  u32 monitor_height = GetSystemMetrics(SM_CYSCREEN);

#define TRANSPARENT_WINDOWx

  // Create the window
#ifdef TRANSPARENT_WINDOW
  window_handle = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST,    // Extended style
                                window_class.lpszClassName,        // Class name
                                "Software Renderer",               // Window name
                                WS_POPUP | WS_VISIBLE,             // Style of the window
                                0,                                 // Initial X position
                                0,                                 // Initial Y position
                                monitor_width,                     // Initial width
                                monitor_height,                    // Initial height 
                                0,                                 // Handle to the window parent
                                0,                                 // Handle to a menu
                                hInstance,                         // Handle to an instance
                                0);                                // Pointer to a CLIENTCTREATESTRUCT
#else
  window_handle = CreateWindowEx(0,                                // Extended style
                                window_class.lpszClassName,        // Class name
                                "Software Renderer",               // Window name
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // Style of the window
                                CW_USEDEFAULT,                     // Initial X position
                                CW_USEDEFAULT,                     // Initial Y position
                                CW_USEDEFAULT,                     // Initial width
                                CW_USEDEFAULT,                     // Initial height 
                                0,                                 // Handle to the window parent
                                0,                                 // Handle to a menu
                                hInstance,                         // Handle to an instance
                                0);                                // Pointer to a CLIENTCTREATESTRUCT
#endif
  if(!window_handle)
  {
    return 1;
  }

  // Create DIB
  HDC hdc = GetDC(window_handle);
  RECT client_rect;
  GetClientRect(window_handle, &client_rect);
  DIB_width = client_rect.right;
  DIB_height = client_rect.bottom;
  int bitCount = 32;
  DIB_row_byte_width = ((DIB_width * (bitCount / 8) + 3) & -4);
  int totalBytes = DIB_row_byte_width * DIB_height;
  BITMAPINFO mybmi = {};
  mybmi.bmiHeader.biSize = sizeof(mybmi);
  mybmi.bmiHeader.biWidth = DIB_width;
  mybmi.bmiHeader.biHeight = DIB_height;
  mybmi.bmiHeader.biPlanes = 1;
  mybmi.bmiHeader.biBitCount = bitCount;
  mybmi.bmiHeader.biCompression = BI_RGB;
  mybmi.bmiHeader.biSizeImage = totalBytes;
  mybmi.bmiHeader.biXPelsPerMeter = 0;
  mybmi.bmiHeader.biYPelsPerMeter = 0;
  DIB_handle = CreateCompatibleDC(hdc);
  DIB_bitmap = CreateDIBSection(hdc, &mybmi, DIB_RGB_COLORS, (VOID **)&frame_buffer, NULL, 0);
  (HBITMAP)SelectObject(DIB_handle, DIB_bitmap);

  ReleaseDC(window_handle, hdc);
  running = true;

  init_renderer(frame_buffer, DIB_width, DIB_height);

  init_logging();

  // Main loop
  while(running)
  {
    time_block("0: main loop");
    float start_frame_time = (float)clock();

    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
      if(message.message == WM_QUIT)
      {
        running = false;
      }
      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    if(key_states[VK_ESCAPE])
    {
      PostQuitMessage(0);
      running = false;
    }

    clear_frame_buffer();
    render();
    
    HDC hdc = GetDC(window_handle);

#ifdef TRANSPARENT_WINDOW
    SIZE sizeSplash = { DIB_width, DIB_height };

    POINT ptZero = { 0 };
    POINT ptOrigin;
    ptOrigin.x = (monitor_width / 2) - (client_rect.right / 2);
    ptOrigin.y = (monitor_height / 2) - (client_rect.bottom / 2);

    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    bool result = UpdateLayeredWindow(window_handle, hdc, &ptOrigin, &sizeSplash,
                                      DIB_handle, &ptZero, RGB(0, 0, 0), &blend, ULW_ALPHA);
#else
    BitBlt(hdc, 0, 0, DIB_width, DIB_height, DIB_handle, 0, 0, SRCCOPY);
#endif


    ReleaseDC(window_handle, hdc);

    end_time_block();
  }

  dump_profile_info();

  return 0;
}
