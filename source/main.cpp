#include "software_renderer.h"
#include "types.h"

#include <windows.h>

static HWND window_handle = 0;
static HDC DIB_handle;
static HBITMAP DIB_bitmap;
static int DIB_width;
static int DIB_height;
static int DIB_row_byte_width;
static u32 *frame_buffer;
static bool running;

#define MAX_BUTTONS 165
static bool button_states[MAX_BUTTONS] = {};


bool button_state(u32 button)
{
  return button_states[button];
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

    case WM_KEYDOWN: 
    {
      button_states[wParam] = true;
      break;
    }
    break;

    case WM_KEYUP:
    {
      button_states[wParam] = false;
      break;
    }
    
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

  POINT ptZero = { 0 };
  HMONITOR hmonPrimary = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO monitorinfo = { 0 };
  const RECT &rcWork = monitorinfo.rcWork;
  monitorinfo.cbSize = sizeof(monitorinfo);
  GetMonitorInfo(hmonPrimary, &monitorinfo);
  LONG monitor_width = rcWork.right - rcWork.left;
  LONG monitor_height = rcWork.bottom - rcWork.top;

#define TRANSPARENT_WINDOW

  // Create the window
#ifdef TRANSPARENT_WINDOW
  window_handle = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST,    // Extended style
                                window_class.lpszClassName,        // Class name
                                "Software Renderer",               // Window name
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // Style of the window
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

  // Main loop
  while(running)
  {
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

    if(button_states[VK_ESCAPE])
    {
      PostQuitMessage(0);
      running = false;
    }

    clear_frame_buffer();
    render();
    
    HDC hdc = GetDC(window_handle);

#ifdef TRANSPARENT_WINDOW
    SIZE sizeSplash = { DIB_width, DIB_height };


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
    BitBlt(hdc, 0, 0, DIB_width, DIB_height, DIB_handle, 0, 0, SRCCOPY );
#endif


    ReleaseDC(window_handle, hdc);
  }

  return 0;
}
