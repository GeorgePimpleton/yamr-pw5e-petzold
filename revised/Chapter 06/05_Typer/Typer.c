/*--------------------------------------
   TYPER.C -- Typing Program
           (c) Charles Petzold, 1998
  --------------------------------------*/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <malloc.h>

#define BUFFER(x,y) *(pBuffer + y * cxBuffer + x)

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI wWinMain(_In_     HINSTANCE hInstance,
                    _In_opt_ HINSTANCE hPrevInstance,
                    _In_     PWSTR     pCmdLine,
                    _In_     int       nShowCmd)
{
   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(pCmdLine);

   static PCWSTR  appName = L"Typer";
   HWND           hwnd;
   MSG            msg;
   WNDCLASSW      wc;

   wc.style         = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc   = WndProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = (HICON)   LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
   wc.hCursor       = (HCURSOR) LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
   wc.hbrBackground = (HBRUSH)  (COLOR_WINDOW + 1);
   wc.lpszMenuName  = NULL;
   wc.lpszClassName = appName;

   if ( !RegisterClassW(&wc) )
   {
      MessageBoxW(NULL, L"This program requires Windows NT!",
                  appName, MB_ICONERROR);
      return 0;
   }

   hwnd = CreateWindowW(appName, L"Typing Program",
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, hInstance, NULL);

   ShowWindow(hwnd, nShowCmd);
   UpdateWindow(hwnd);

   while ( GetMessage(&msg, NULL, 0, 0) )
   {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
   }
   return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   static DWORD dwCharSet = DEFAULT_CHARSET;
   static int   cxChar;
   static int   cyChar;
   static int   cxClient;
   static int   cyClient;
   static int   cxBuffer;
   static int   cyBuffer;
   static int   xCaret;
   static int   yCaret;
   static PWSTR pBuffer   = NULL;
   HDC          hdc;
   int          x;
   int          y;
   int          i;
   PAINTSTRUCT  ps;
   TEXTMETRICW  tm;

   switch ( message )
   {
   case WM_INPUTLANGCHANGE:
      dwCharSet = (DWORD) wParam;
      // fall through

   case WM_CREATE:
      hdc = GetDC(hwnd);
      SelectObject(hdc, CreateFontW(0, 0, 0, 0, 0, 0, 0, 0,
                                    dwCharSet, 0, 0, 0, FIXED_PITCH, NULL));

      GetTextMetricsW(hdc, &tm);
      cxChar = tm.tmAveCharWidth;
      cyChar = tm.tmHeight;

      DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));
      ReleaseDC(hwnd, hdc);
      // fall through

   case WM_SIZE:
      // obtain window size in pixels
      if ( message == WM_SIZE )
      {
         cxClient = GET_X_LPARAM(lParam);
         cyClient = GET_Y_LPARAM(lParam);
      }

      // calculate window size in characters
      cxBuffer = max(1, cxClient / cxChar);
      cyBuffer = max(1, cyClient / cyChar);

      // allocate memory for buffer and clear it
      if ( pBuffer != NULL )
         free(pBuffer);

      pBuffer = (PWSTR) malloc(cxBuffer * cyBuffer * sizeof(WCHAR));

      for ( y = 0; y < cyBuffer; y++ )
         for ( x = 0; x < cxBuffer; x++ )
            BUFFER(x, y) = ' ';

      // set caret to upper left corner

      xCaret = 0;
      yCaret = 0;

      if ( hwnd == GetFocus() )
         SetCaretPos(xCaret * cxChar, yCaret * cyChar);

      InvalidateRect(hwnd, NULL, TRUE);
      return 0;

   case WM_SETFOCUS:
      // create and show the caret
      CreateCaret(hwnd, NULL, cxChar, cyChar);
      SetCaretPos(xCaret * cxChar, yCaret * cyChar);
      ShowCaret(hwnd);
      return 0;

   case WM_KILLFOCUS:
      // hide and destroy the caret
      HideCaret(hwnd);
      DestroyCaret();
      return 0;

   case WM_KEYDOWN:
      switch ( wParam )
      {
      case VK_HOME:
         xCaret = 0;
         break;

      case VK_END:
         xCaret = cxBuffer - 1;
         break;

      case VK_PRIOR:
         yCaret = 0;
         break;

      case VK_NEXT:
         yCaret = cyBuffer - 1;
         break;

      case VK_LEFT:
         xCaret = max(xCaret - 1, 0);
         break;

      case VK_RIGHT:
         xCaret = min(xCaret + 1, cxBuffer - 1);
         break;

      case VK_UP:
         yCaret = max(yCaret - 1, 0);
         break;

      case VK_DOWN:
         yCaret = min(yCaret + 1, cyBuffer - 1);
         break;

      case VK_DELETE:
         for ( x = xCaret; x < cxBuffer - 1; x++ )
            BUFFER(x, yCaret) = BUFFER(x + 1, yCaret);

         BUFFER(cxBuffer - 1, yCaret) = ' ';

         HideCaret(hwnd);
         hdc = GetDC(hwnd);

         SelectObject(hdc, CreateFontW(0, 0, 0, 0, 0, 0, 0, 0,
                                       dwCharSet, 0, 0, 0, FIXED_PITCH, NULL));

         TextOutW(hdc, xCaret * cxChar, yCaret * cyChar,
                  &BUFFER(xCaret, yCaret),
                  cxBuffer - xCaret);

         DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));
         ReleaseDC(hwnd, hdc);
         ShowCaret(hwnd);
         break;
      }
      SetCaretPos(xCaret * cxChar, yCaret * cyChar);
      return 0;

   case WM_CHAR:
      for ( i = 0; i < (int) LOWORD(lParam); i++ )
      {
         switch ( wParam )
         {
         case '\b':  // backspace
            if ( xCaret > 0 )
            {
               xCaret--;
               SendMessageW(hwnd, WM_KEYDOWN, VK_DELETE, 1);
            }
            break;

         case '\t':  // tab
            do
            {
               SendMessageW(hwnd, WM_CHAR, ' ', 1);
            } while ( xCaret % 8 != 0 );
            break;

         case '\n':  // line feed
            if ( ++yCaret == cyBuffer )
               yCaret = 0;
            break;

         case '\r':  // carriage return
            xCaret = 0;

            if ( ++yCaret == cyBuffer )
               yCaret = 0;
            break;

         case '\x1B':   // escape
            for ( y = 0; y < cyBuffer; y++ )
               for ( x = 0; x < cxBuffer; x++ )
                  BUFFER(x, y) = ' ';

            xCaret = 0;
            yCaret = 0;

            InvalidateRect(hwnd, NULL, FALSE);
            break;

         default:                      // character codes
            BUFFER(xCaret, yCaret) = (WCHAR) wParam;

            HideCaret(hwnd);
            hdc = GetDC(hwnd);

            SelectObject(hdc, CreateFontW(0, 0, 0, 0, 0, 0, 0, 0,
                                          dwCharSet, 0, 0, 0, FIXED_PITCH, NULL));

            TextOutW(hdc, xCaret* cxChar, yCaret* cyChar,
                     &BUFFER(xCaret, yCaret), 1);

            DeleteObject(
               SelectObject(hdc, GetStockObject(SYSTEM_FONT)));
            ReleaseDC(hwnd, hdc);
            ShowCaret(hwnd);

            if ( ++xCaret == cxBuffer )
            {
               xCaret = 0;

               if ( ++yCaret == cyBuffer )
                  yCaret = 0;
            }
            break;
         }
      }

      SetCaretPos(xCaret * cxChar, yCaret * cyChar);
      return 0;

   case WM_PAINT:
      hdc = BeginPaint(hwnd, &ps);

      SelectObject(hdc, CreateFontW(0, 0, 0, 0, 0, 0, 0, 0,
                                    dwCharSet, 0, 0, 0, FIXED_PITCH, NULL));

      for ( y = 0; y < cyBuffer; y++ )
         TextOutW(hdc, 0, y * cyChar, &BUFFER(0, y), cxBuffer);

      DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));
      EndPaint(hwnd, &ps);
      return 0;

   case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
   }

   return DefWindowProcW(hwnd, message, wParam, lParam);
}