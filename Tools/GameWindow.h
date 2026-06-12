#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#if defined(__linux__) && defined(__has_include)
#if __has_include(<xcb/xcb.h>)
#include <xcb/xcb.h>
#define FYBACKEND_HAS_XCB 1
#else
#define FYBACKEND_HAS_XCB 0
#endif
#else
#define FYBACKEND_HAS_XCB 0
#endif

namespace FyBackend
{
    struct GameWindowDesc
    {
        uint32_t Width = 1280;
        uint32_t Height = 720;
        std::wstring Title = L"Gallery";
        bool Resizable = true;
        bool Show = true;
    };

    class GameWindow
    {
    public:
        GameWindow() = default;
        GameWindow(uint32_t width, uint32_t height, const wchar_t* title)
        {
            m_desc.Width = width;
            m_desc.Height = height;
            m_desc.Title = title ? title : L"Gallery";
        }

        bool Create(const GameWindowDesc& desc);
        bool Initialize(void* nativeInstance);
        void Destroy();
        int Run();

        void SetOnUpdate(std::function<void()> func) { m_onUpdate = std::move(func); }
        void SetOnRender(std::function<void()> func) { m_onRender = std::move(func); }
        void SetOnResize(std::function<void(uint32_t, uint32_t)> func) { m_onResize = std::move(func); }

        uint32_t GetWidth() const { return m_width; }
        uint32_t GetHeight() const { return m_height; }
        void* GetNativeWindow() const { return m_nativeWindow; }
        void* GetNativeInstance() const { return m_nativeInstance; }

#if defined(_WIN32)
        HWND GetHWND() const { return static_cast<HWND>(m_nativeWindow); }
        HINSTANCE GetHINSTANCE() const { return static_cast<HINSTANCE>(m_nativeInstance); }
        void SetOnWin32Message(std::function<bool(HWND, UINT, WPARAM, LPARAM)> func) { m_onWin32Message = std::move(func); }
        void SetOnMessage(std::function<bool(HWND, UINT, WPARAM, LPARAM)> func) { SetOnWin32Message(std::move(func)); }
#endif

#if FYBACKEND_HAS_XCB
        xcb_connection_t* GetXcbConnection() const { return m_xcbConnection; }
        xcb_window_t GetXcbWindow() const { return m_xcbWindow; }
#endif

    private:
        void NotifyResize(uint32_t width, uint32_t height);

#if defined(_WIN32)
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleWin32Message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

        GameWindowDesc m_desc;
        void* m_nativeWindow = nullptr;
        void* m_nativeInstance = nullptr;
        uint32_t m_width = 1280;
        uint32_t m_height = 720;
        bool m_running = false;

        std::function<void()> m_onUpdate;
        std::function<void()> m_onRender;
        std::function<void(uint32_t, uint32_t)> m_onResize;

#if defined(_WIN32)
        std::function<bool(HWND, UINT, WPARAM, LPARAM)> m_onWin32Message;
#endif

#if FYBACKEND_HAS_XCB
        xcb_connection_t* m_xcbConnection = nullptr;
        xcb_screen_t* m_xcbScreen = nullptr;
        xcb_window_t m_xcbWindow = 0;
        xcb_atom_t m_wmProtocols = 0;
        xcb_atom_t m_wmDeleteWindow = 0;
#endif
    };
}
