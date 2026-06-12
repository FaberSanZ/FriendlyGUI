#pragma once

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Xinput.h>
#endif

#include "FriendlyInput.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace FyBackend
{
#if defined(_WIN32)
    class FriendlyInputWin32
    {
    public:
        void Attach(HWND hwnd)
        {
            m_hwnd = hwnd;
        }

        bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM)
        {
            m_hwnd = hwnd;
            switch (msg)
            {
            case WM_LBUTTONDOWN:
                m_mouseDown[0] = true;
                SetCapture(hwnd);
                return false;
            case WM_LBUTTONUP:
                m_mouseDown[0] = false;
                ReleaseCapture();
                return false;
            case WM_RBUTTONDOWN:
                m_mouseDown[1] = true;
                SetCapture(hwnd);
                return false;
            case WM_RBUTTONUP:
                m_mouseDown[1] = false;
                ReleaseCapture();
                return false;
            case WM_MBUTTONDOWN:
                m_mouseDown[2] = true;
                SetCapture(hwnd);
                return false;
            case WM_MBUTTONUP:
                m_mouseDown[2] = false;
                ReleaseCapture();
                return false;
            case WM_MOUSEWHEEL:
                m_wheelAccumulator += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
                return false;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                SetKey(MapKey(wParam), true);
                return false;
            case WM_KEYUP:
            case WM_SYSKEYUP:
                SetKey(MapKey(wParam), false);
                return false;
            case WM_CHAR:
                if (wParam >= 32)
                    m_textInput.push_back(static_cast<uint32_t>(wParam));
                return false;
            case WM_KILLFOCUS:
                m_keyDown.fill(false);
                m_mouseDown.fill(false);
                return false;
            default:
                return false;
            }
        }

        FyGUI::InputSnapshot Build(float deltaTime)
        {
            m_builder.BeginFrame(deltaTime);

            POINT cursor {};
            if (m_hwnd && GetCursorPos(&cursor) && ScreenToClient(m_hwnd, &cursor))
                m_builder.SetPointerPosition({ static_cast<float>(cursor.x), static_cast<float>(cursor.y) });

            for (size_t i = 0; i < m_mouseDown.size(); ++i)
                m_builder.SetMouseButton(static_cast<FyGUI::MouseButton>(i), m_mouseDown[i]);
            for (size_t i = 1; i < FyGUI::KeyCount; ++i)
                m_builder.SetKey(static_cast<FyGUI::Key>(i), m_keyDown[i]);

            m_builder.AddWheelDelta(m_wheelAccumulator * 48.0f);
            m_wheelAccumulator = 0.0f;
            for (uint32_t codepoint : m_textInput)
                m_builder.AddTextInput(codepoint);
            m_textInput.clear();

            UpdateXInput();
            return m_builder.EndFrame();
        }

        void Reset()
        {
            m_builder.Reset();
            m_mouseDown.fill(false);
            m_keyDown.fill(false);
            m_textInput.clear();
            m_wheelAccumulator = 0.0f;
        }

    private:
        static FyGUI::Key MapKey(WPARAM key)
        {
            if (key >= 'A' && key <= 'Z')
                return static_cast<FyGUI::Key>(static_cast<size_t>(FyGUI::Key::A) + (key - 'A'));
            if (key >= '0' && key <= '9')
                return static_cast<FyGUI::Key>(static_cast<size_t>(FyGUI::Key::Num0) + (key - '0'));
            switch (key)
            {
            case VK_TAB: return FyGUI::Key::Tab;
            case VK_RETURN: return FyGUI::Key::Enter;
            case VK_ESCAPE: return FyGUI::Key::Escape;
            case VK_SPACE: return FyGUI::Key::Space;
            case VK_BACK: return FyGUI::Key::Backspace;
            case VK_DELETE: return FyGUI::Key::DeleteKey;
            case VK_LEFT: return FyGUI::Key::Left;
            case VK_RIGHT: return FyGUI::Key::Right;
            case VK_UP: return FyGUI::Key::Up;
            case VK_DOWN: return FyGUI::Key::Down;
            case VK_HOME: return FyGUI::Key::Home;
            case VK_END: return FyGUI::Key::End;
            case VK_PRIOR: return FyGUI::Key::PageUp;
            case VK_NEXT: return FyGUI::Key::PageDown;
            case VK_SHIFT:
            case VK_LSHIFT:
            case VK_RSHIFT:
                return FyGUI::Key::Shift;
            case VK_CONTROL:
            case VK_LCONTROL:
            case VK_RCONTROL:
                return FyGUI::Key::Control;
            case VK_MENU:
            case VK_LMENU:
            case VK_RMENU:
                return FyGUI::Key::Alt;
            default:
                return FyGUI::Key::None;
            }
        }

        void SetKey(FyGUI::Key key, bool down)
        {
            const size_t index = static_cast<size_t>(key);
            if (index > 0 && index < FyGUI::KeyCount)
                m_keyDown[index] = down;
        }

        static float NormalizeStick(SHORT value, SHORT deadZone)
        {
            const int v = static_cast<int>(value);
            if (std::abs(v) < deadZone)
                return 0.0f;
            const float denom = v < 0 ? 32768.0f : 32767.0f;
            return std::clamp(static_cast<float>(v) / denom, -1.0f, 1.0f);
        }

        void ClearGamepadState()
        {
            m_builder.SetGamepadConnected(false);
            for (size_t i = 1; i < static_cast<size_t>(FyGUI::GamepadButton::Count); ++i)
                m_builder.SetGamepadButton(static_cast<FyGUI::GamepadButton>(i), false);
            for (size_t i = 0; i < static_cast<size_t>(FyGUI::GamepadAxis::Count); ++i)
                m_builder.SetGamepadAxis(static_cast<FyGUI::GamepadAxis>(i), 0.0f);
        }

        void SetGamepadButton(WORD buttons, WORD xinputButton, FyGUI::GamepadButton button)
        {
            m_builder.SetGamepadButton(button, (buttons & xinputButton) != 0);
        }

        void UpdateXInput()
        {
            ClearGamepadState();

            XINPUT_STATE state {};
            if (XInputGetState(0, &state) != ERROR_SUCCESS)
                return;

            const WORD buttons = state.Gamepad.wButtons;
            m_builder.SetGamepadConnected(true);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_A, FyGUI::GamepadButton::FaceDown);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_B, FyGUI::GamepadButton::FaceRight);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_X, FyGUI::GamepadButton::FaceLeft);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_Y, FyGUI::GamepadButton::FaceUp);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_DPAD_UP, FyGUI::GamepadButton::DPadUp);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_DPAD_DOWN, FyGUI::GamepadButton::DPadDown);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_DPAD_LEFT, FyGUI::GamepadButton::DPadLeft);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_DPAD_RIGHT, FyGUI::GamepadButton::DPadRight);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_LEFT_SHOULDER, FyGUI::GamepadButton::LeftShoulder);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_RIGHT_SHOULDER, FyGUI::GamepadButton::RightShoulder);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_LEFT_THUMB, FyGUI::GamepadButton::LeftStick);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_RIGHT_THUMB, FyGUI::GamepadButton::RightStick);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_BACK, FyGUI::GamepadButton::Back);
            SetGamepadButton(buttons, XINPUT_GAMEPAD_START, FyGUI::GamepadButton::Start);
            m_builder.SetGamepadButton(FyGUI::GamepadButton::LeftTrigger, state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
            m_builder.SetGamepadButton(FyGUI::GamepadButton::RightTrigger, state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
            m_builder.SetGamepadAxis(FyGUI::GamepadAxis::LeftX, NormalizeStick(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
            m_builder.SetGamepadAxis(FyGUI::GamepadAxis::LeftY, -NormalizeStick(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
            m_builder.SetGamepadAxis(FyGUI::GamepadAxis::RightX, NormalizeStick(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
            m_builder.SetGamepadAxis(FyGUI::GamepadAxis::RightY, -NormalizeStick(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
            m_builder.SetGamepadAxis(FyGUI::GamepadAxis::LeftTrigger, static_cast<float>(state.Gamepad.bLeftTrigger) / 255.0f);
            m_builder.SetGamepadAxis(FyGUI::GamepadAxis::RightTrigger, static_cast<float>(state.Gamepad.bRightTrigger) / 255.0f);
        }

        HWND m_hwnd = nullptr;
        FyGUI::InputBuilder m_builder;
        std::array<bool, 3> m_mouseDown {};
        std::array<bool, FyGUI::KeyCount> m_keyDown {};
        std::vector<uint32_t> m_textInput;
        float m_wheelAccumulator = 0.0f;
    };
#else
    class FriendlyInputWin32
    {
    public:
        void Attach(void*) {}
        FyGUI::InputSnapshot Build(float) { return {}; }
        void Reset() {}
    };
#endif
}
