#include "GameWindow.h"

#if defined(_WIN32)
#include <shellscalingapi.h>

namespace FyBackend
{
    namespace
    {
        constexpr const wchar_t* WindowClassName = L"FyGUI.GameWindow";

        void EnableDpiAwareness()
        {
            static bool enabled = false;
            if (enabled)
                return;
            enabled = true;

            HMODULE user32 = LoadLibraryA("user32.dll");
            if (user32)
            {
                using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
                auto setProcessDpiAwarenessContext = reinterpret_cast<SetProcessDpiAwarenessContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
                if (setProcessDpiAwarenessContext && setProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
                    return;

                using SetThreadDpiAwarenessContextFn = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
                auto setThreadDpiAwarenessContext = reinterpret_cast<SetThreadDpiAwarenessContextFn>(GetProcAddress(user32, "SetThreadDpiAwarenessContext"));
                if (setThreadDpiAwarenessContext && setThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
                    return;
            }

            HMODULE shcore = LoadLibraryA("shcore.dll");
            if (shcore)
            {
                using SetProcessDpiAwarenessFn = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);
                auto setProcessDpiAwareness = reinterpret_cast<SetProcessDpiAwarenessFn>(GetProcAddress(shcore, "SetProcessDpiAwareness"));
                if (setProcessDpiAwareness && SUCCEEDED(setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE)))
                    return;
            }

            SetProcessDPIAware();
        }

        DWORD WindowStyle(bool resizable)
        {
            DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            if (resizable)
                style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
            return style;
        }

        RECT AdjustClientRect(uint32_t width, uint32_t height, DWORD style, DWORD exStyle, UINT dpi)
        {
            RECT rect { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0A00
            AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, dpi);
#else
            (void)dpi;
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);
#endif
            return rect;
        }
    }

    bool GameWindow::Create(const GameWindowDesc& desc)
    {
        m_desc = desc;
        return Initialize(GetModuleHandleW(nullptr));
    }

    bool GameWindow::Initialize(void* nativeInstance)
    {
        EnableDpiAwareness();

        m_nativeInstance = nativeInstance ? nativeInstance : GetModuleHandleW(nullptr);
        HINSTANCE instance = static_cast<HINSTANCE>(m_nativeInstance);

        WNDCLASSEXW wc {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = GameWindow::WindowProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wc.lpszClassName = WindowClassName;
        RegisterClassExW(&wc);

        const DWORD style = WindowStyle(m_desc.Resizable);
        const DWORD exStyle = 0;
        const UINT dpi = GetDpiForSystem();
        RECT rect = AdjustClientRect(m_desc.Width, m_desc.Height, style, exStyle, dpi);

        HWND hwnd = CreateWindowExW(
            exStyle,
            WindowClassName,
            m_desc.Title.c_str(),
            style,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            instance,
            this);

        if (!hwnd)
            return false;

        m_nativeWindow = hwnd;
        RECT client {};
        GetClientRect(hwnd, &client);
        m_width = static_cast<uint32_t>(client.right - client.left);
        m_height = static_cast<uint32_t>(client.bottom - client.top);

        if (m_desc.Show)
            ShowWindow(hwnd, SW_SHOW);

        return true;
    }

    void GameWindow::Destroy()
    {
        if (m_nativeWindow)
        {
            DestroyWindow(static_cast<HWND>(m_nativeWindow));
            m_nativeWindow = nullptr;
        }
    }

    int GameWindow::Run()
    {
        m_running = true;
        MSG msg {};
        while (m_running && msg.message != WM_QUIT)
        {
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
                if (msg.message == WM_QUIT)
                    break;
            }
            if (!m_running || msg.message == WM_QUIT)
                break;

            if (m_onUpdate)
                m_onUpdate();
            if (m_onRender)
                m_onRender();
        }
        return static_cast<int>(msg.wParam);
    }

    void GameWindow::NotifyResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return;
        if (m_width == width && m_height == height)
            return;
        m_width = width;
        m_height = height;
        if (m_onResize)
            m_onResize(width, height);
    }

    LRESULT CALLBACK GameWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        GameWindow* window = nullptr;
        if (msg == WM_NCCREATE)
        {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            window = reinterpret_cast<GameWindow*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        }
        else
        {
            window = reinterpret_cast<GameWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (!window)
            return DefWindowProcW(hwnd, msg, wParam, lParam);

        return window->HandleWin32Message(hwnd, msg, wParam, lParam);
    }

    LRESULT GameWindow::HandleWin32Message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (m_onWin32Message && m_onWin32Message(hwnd, msg, wParam, lParam))
            return 0;

        switch (msg)
        {
        case WM_SIZE:
            NotifyResize(static_cast<uint32_t>(LOWORD(lParam)), static_cast<uint32_t>(HIWORD(lParam)));
            return 0;

        case WM_DPICHANGED:
            if (RECT* suggested = reinterpret_cast<RECT*>(lParam))
            {
                SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;

        case WM_CLOSE:
            m_running = false;
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
#elif FYBACKEND_HAS_XCB
#include <cstring>
#include <cstdlib>

namespace FyBackend
{
    namespace
    {
        xcb_screen_t* FirstScreen(xcb_connection_t* connection)
        {
            xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
            return iter.data;
        }

        xcb_atom_t InternAtom(xcb_connection_t* connection, const char* name)
        {
            xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 0, static_cast<uint16_t>(std::strlen(name)), name);
            xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie, nullptr);
            if (!reply)
                return XCB_ATOM_NONE;
            xcb_atom_t atom = reply->atom;
            std::free(reply);
            return atom;
        }
    }

    bool GameWindow::Create(const GameWindowDesc& desc)
    {
        m_desc = desc;
        return Initialize(nullptr);
    }

    bool GameWindow::Initialize(void*)
    {
        m_xcbConnection = xcb_connect(nullptr, nullptr);
        if (!m_xcbConnection || xcb_connection_has_error(m_xcbConnection))
            return false;

        m_xcbScreen = FirstScreen(m_xcbConnection);
        if (!m_xcbScreen)
            return false;

        m_width = m_desc.Width;
        m_height = m_desc.Height;
        m_xcbWindow = xcb_generate_id(m_xcbConnection);

        const uint32_t values[] = {
            m_xcbScreen->black_pixel,
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS |
                XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                XCB_EVENT_MASK_POINTER_MOTION
        };

        xcb_create_window(
            m_xcbConnection,
            XCB_COPY_FROM_PARENT,
            m_xcbWindow,
            m_xcbScreen->root,
            0,
            0,
            static_cast<uint16_t>(m_width),
            static_cast<uint16_t>(m_height),
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            m_xcbScreen->root_visual,
            XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
            values);

        m_wmProtocols = InternAtom(m_xcbConnection, "WM_PROTOCOLS");
        m_wmDeleteWindow = InternAtom(m_xcbConnection, "WM_DELETE_WINDOW");
        if (m_wmProtocols != XCB_ATOM_NONE && m_wmDeleteWindow != XCB_ATOM_NONE)
            xcb_change_property(m_xcbConnection, XCB_PROP_MODE_REPLACE, m_xcbWindow, m_wmProtocols, XCB_ATOM_ATOM, 32, 1, &m_wmDeleteWindow);

        if (m_desc.Show)
            xcb_map_window(m_xcbConnection, m_xcbWindow);
        xcb_flush(m_xcbConnection);

        m_nativeWindow = reinterpret_cast<void*>(static_cast<uintptr_t>(m_xcbWindow));
        m_nativeInstance = m_xcbConnection;
        return true;
    }

    void GameWindow::Destroy()
    {
        if (m_xcbConnection && m_xcbWindow)
        {
            xcb_destroy_window(m_xcbConnection, m_xcbWindow);
            xcb_flush(m_xcbConnection);
        }
        if (m_xcbConnection)
            xcb_disconnect(m_xcbConnection);
        m_xcbConnection = nullptr;
        m_xcbScreen = nullptr;
        m_xcbWindow = 0;
        m_nativeWindow = nullptr;
        m_nativeInstance = nullptr;
    }

    int GameWindow::Run()
    {
        m_running = true;
        while (m_running)
        {
            xcb_generic_event_t* event = nullptr;
            while ((event = xcb_poll_for_event(m_xcbConnection)) != nullptr)
            {
                const uint8_t type = event->response_type & ~0x80;
                if (type == XCB_CONFIGURE_NOTIFY)
                {
                    auto* configure = reinterpret_cast<xcb_configure_notify_event_t*>(event);
                    NotifyResize(configure->width, configure->height);
                }
                else if (type == XCB_CLIENT_MESSAGE)
                {
                    auto* message = reinterpret_cast<xcb_client_message_event_t*>(event);
                    if (message->data.data32[0] == m_wmDeleteWindow)
                        m_running = false;
                }
                else if (type == XCB_DESTROY_NOTIFY)
                {
                    m_running = false;
                }
                std::free(event);
            }

            if (m_onUpdate)
                m_onUpdate();
            if (m_onRender)
                m_onRender();
            xcb_flush(m_xcbConnection);
        }
        return 0;
    }

    void GameWindow::NotifyResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return;
        if (m_width == width && m_height == height)
            return;
        m_width = width;
        m_height = height;
        if (m_onResize)
            m_onResize(width, height);
    }

}
#else
namespace FyBackend
{
    bool GameWindow::Create(const GameWindowDesc& desc)
    {
        m_desc = desc;
        m_width = desc.Width;
        m_height = desc.Height;
        return false;
    }

    bool GameWindow::Initialize(void*)
    {
        return false;
    }

    void GameWindow::Destroy()
    {
    }

    int GameWindow::Run()
    {
        return 0;
    }

    void GameWindow::NotifyResize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
        if (m_onResize)
            m_onResize(width, height);
    }
}
#endif
