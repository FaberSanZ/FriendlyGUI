#pragma once

#ifndef FRIENDLYGUI_USE_FRIENDLY_INPUT
#define FRIENDLYGUI_USE_FRIENDLY_INPUT 1
#endif

#include "FriendlyControls.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>

namespace FyGUI
{
    enum class MouseButton : uint8_t
    {
        Left = 0,
        Right = 1,
        Middle = 2
    };

    enum class GamepadButton : uint16_t
    {
        None,
        FaceDown,
        FaceRight,
        FaceLeft,
        FaceUp,
        DPadUp,
        DPadDown,
        DPadLeft,
        DPadRight,
        LeftShoulder,
        RightShoulder,
        LeftTrigger,
        RightTrigger,
        LeftStick,
        RightStick,
        Back,
        Start,
        Guide,
        Count
    };

    enum class GamepadAxis : uint8_t
    {
        LeftX,
        LeftY,
        RightX,
        RightY,
        LeftTrigger,
        RightTrigger,
        Count
    };

    enum class GamepadGlyphStyle : uint8_t
    {
        Xbox,
        PlayStation,
        Switch,
        Generic
    };

    struct GamepadMapping
    {
        GamepadButton Accept = GamepadButton::FaceDown;
        GamepadButton Cancel = GamepadButton::FaceRight;
        GamepadButton Secondary = GamepadButton::FaceLeft;
        GamepadButton Details = GamepadButton::FaceUp;
        GamepadButton Back = GamepadButton::Back;
        GamepadButton Menu = GamepadButton::Start;
        GamepadButton NavigateUp = GamepadButton::DPadUp;
        GamepadButton NavigateDown = GamepadButton::DPadDown;
        GamepadButton NavigateLeft = GamepadButton::DPadLeft;
        GamepadButton NavigateRight = GamepadButton::DPadRight;
        GamepadButton FocusPrevious = GamepadButton::LeftShoulder;
        GamepadButton FocusNext = GamepadButton::RightShoulder;
        GamepadButton PageLeft = GamepadButton::LeftShoulder;
        GamepadButton PageRight = GamepadButton::RightShoulder;
    };

    struct InputRepeatSettings
    {
        float InitialDelay = 0.25f;
        float RepeatInterval = 0.075f;
        float StickDeadZone = 0.35f;
        float TriggerThreshold = 0.50f;
    };

    struct InputGlyph
    {
        std::string Text;
        std::string IconName;
    };

    class InputBuilder
    {
    public:
        InputBuilder()
        {
            Reset();
        }

        void BeginFrame(float deltaTime)
        {
            m_deltaTime = std::max(0.0f, deltaTime);
            m_wheelDelta = 0.0f;
            m_textInput.clear();
            m_current = {};
        }

        void SetPointerPosition(Vec2 position)
        {
            m_pointerPosition = position;
        }

        void SetMouseButton(MouseButton button, bool down)
        {
            const size_t index = static_cast<size_t>(button);
            if (index < 3)
                m_mouseDown[index] = down;
        }

        void AddWheelDelta(float delta)
        {
            m_wheelDelta += delta;
        }

        void SetKey(Key key, bool down)
        {
            const size_t index = static_cast<size_t>(key);
            if (index > 0 && index < KeyCount)
                m_keyDown[index] = down;
        }

        void AddTextInput(uint32_t codepoint)
        {
            if (codepoint != 0)
                m_textInput.push_back(codepoint);
        }

        void AddTextInputUtf8(std::string_view text)
        {
            for (size_t i = 0; i < text.size();)
            {
                const unsigned char c0 = static_cast<unsigned char>(text[i]);
                if (c0 < 0x80)
                {
                    AddTextInput(c0);
                    ++i;
                    continue;
                }

                uint32_t cp = 0;
                size_t extra = 0;
                if ((c0 & 0xE0) == 0xC0)
                {
                    cp = c0 & 0x1F;
                    extra = 1;
                }
                else if ((c0 & 0xF0) == 0xE0)
                {
                    cp = c0 & 0x0F;
                    extra = 2;
                }
                else if ((c0 & 0xF8) == 0xF0)
                {
                    cp = c0 & 0x07;
                    extra = 3;
                }
                else
                {
                    ++i;
                    continue;
                }

                if (i + extra >= text.size())
                    break;

                bool valid = true;
                for (size_t j = 1; j <= extra; ++j)
                {
                    const unsigned char cx = static_cast<unsigned char>(text[i + j]);
                    if ((cx & 0xC0) != 0x80)
                    {
                        valid = false;
                        break;
                    }
                    cp = (cp << 6) | (cx & 0x3F);
                }

                if (valid)
                    AddTextInput(cp);
                i += extra + 1;
            }
        }

        void SetGamepadConnected(bool connected)
        {
            m_gamepadConnected = connected;
        }

        void SetGamepadButton(GamepadButton button, bool down)
        {
            const size_t index = static_cast<size_t>(button);
            if (index > 0 && index < static_cast<size_t>(GamepadButton::Count))
                m_gamepadDown[index] = down;
        }

        void SetGamepadAxis(GamepadAxis axis, float value)
        {
            const size_t index = static_cast<size_t>(axis);
            if (index < static_cast<size_t>(GamepadAxis::Count))
                m_gamepadAxis[index] = std::clamp(value, -1.0f, 1.0f);
        }

        void SetGamepadMapping(const GamepadMapping& mapping)
        {
            m_mapping = mapping;
        }

        void SetRepeatSettings(const InputRepeatSettings& settings)
        {
            m_repeat = settings;
            m_repeat.InitialDelay = std::max(0.0f, m_repeat.InitialDelay);
            m_repeat.RepeatInterval = std::max(0.001f, m_repeat.RepeatInterval);
            m_repeat.StickDeadZone = std::clamp(m_repeat.StickDeadZone, 0.0f, 1.0f);
            m_repeat.TriggerThreshold = std::clamp(m_repeat.TriggerThreshold, 0.0f, 1.0f);
        }

        const GamepadMapping& GetGamepadMapping() const
        {
            return m_mapping;
        }

        const InputRepeatSettings& GetRepeatSettings() const
        {
            return m_repeat;
        }

        InputSnapshot EndFrame()
        {
            InputSnapshot input {};
            input.PointerPosition = m_pointerPosition;
            input.PointerDelta = m_hasPreviousPointer ? Vec2 { m_pointerPosition.x - m_previousPointerPosition.x, m_pointerPosition.y - m_previousPointerPosition.y } : Vec2 {};
            input.WheelDelta = m_wheelDelta;

            for (size_t i = 0; i < 3; ++i)
            {
                input.MouseDown[i] = m_mouseDown[i];
                input.MousePressed[i] = m_mouseDown[i] && !m_prevMouseDown[i];
                input.MouseReleased[i] = !m_mouseDown[i] && m_prevMouseDown[i];
            }

            for (size_t i = 0; i < KeyCount; ++i)
            {
                input.KeyDown[i] = m_keyDown[i];
                input.KeyPressed[i] = m_keyDown[i] && !m_prevKeyDown[i];
                input.KeyReleased[i] = !m_keyDown[i] && m_prevKeyDown[i];
            }

            input.ShiftDown = input.KeyDown[static_cast<size_t>(Key::Shift)];
            input.ControlDown = input.KeyDown[static_cast<size_t>(Key::Control)];
            input.AltDown = input.KeyDown[static_cast<size_t>(Key::Alt)];
            input.TextInputCodepoints = m_textInput;
            input.NavigateUpPressed = RepeatDigital(input.KeyDown[static_cast<size_t>(Key::Up)], m_keyRepeatUp);
            input.NavigateDownPressed = RepeatDigital(input.KeyDown[static_cast<size_t>(Key::Down)], m_keyRepeatDown);
            input.NavigateLeftPressed = RepeatDigital(input.KeyDown[static_cast<size_t>(Key::Left)], m_keyRepeatLeft);
            input.NavigateRightPressed = RepeatDigital(input.KeyDown[static_cast<size_t>(Key::Right)], m_keyRepeatRight);
            input.FocusNextPressed = RepeatDigital(input.KeyDown[static_cast<size_t>(Key::Tab)] && !input.ShiftDown, m_keyRepeatFocusNext);
            input.FocusPreviousPressed = RepeatDigital(input.KeyDown[static_cast<size_t>(Key::Tab)] && input.ShiftDown, m_keyRepeatFocusPrevious);
            input.PageLeftPressed = RepeatDigital(input.KeyDown[static_cast<size_t>(Key::PageUp)], m_keyRepeatPageLeft);
            input.PageRightPressed = RepeatDigital(input.KeyDown[static_cast<size_t>(Key::PageDown)], m_keyRepeatPageRight);

            input.GamepadConnected = m_gamepadConnected;
            input.GamepadAcceptPressed = ButtonPressed(m_mapping.Accept);
            input.GamepadCancelPressed = ButtonPressed(m_mapping.Cancel);
            input.GamepadSecondaryPressed = ButtonPressed(m_mapping.Secondary);
            input.GamepadDetailsPressed = ButtonPressed(m_mapping.Details);
            input.GamepadBackPressed = ButtonPressed(m_mapping.Back);
            input.GamepadMenuPressed = ButtonPressed(m_mapping.Menu);
            input.GamepadLeftX = m_gamepadAxis[static_cast<size_t>(GamepadAxis::LeftX)];
            input.GamepadLeftY = m_gamepadAxis[static_cast<size_t>(GamepadAxis::LeftY)];
            input.GamepadRightX = m_gamepadAxis[static_cast<size_t>(GamepadAxis::RightX)];
            input.GamepadRightY = m_gamepadAxis[static_cast<size_t>(GamepadAxis::RightY)];

            const bool upDown = ButtonDown(m_mapping.NavigateUp) || AxisPressed(GamepadAxis::LeftY, -1.0f, m_repeat.StickDeadZone);
            const bool downDown = ButtonDown(m_mapping.NavigateDown) || AxisPressed(GamepadAxis::LeftY, 1.0f, m_repeat.StickDeadZone);
            const bool leftDown = ButtonDown(m_mapping.NavigateLeft) || AxisPressed(GamepadAxis::LeftX, -1.0f, m_repeat.StickDeadZone);
            const bool rightDown = ButtonDown(m_mapping.NavigateRight) || AxisPressed(GamepadAxis::LeftX, 1.0f, m_repeat.StickDeadZone);
            input.GamepadNavigateUpPressed = RepeatDigital(upDown, m_repeatUp);
            input.GamepadNavigateDownPressed = RepeatDigital(downDown, m_repeatDown);
            input.GamepadNavigateLeftPressed = RepeatDigital(leftDown, m_repeatLeft);
            input.GamepadNavigateRightPressed = RepeatDigital(rightDown, m_repeatRight);
            input.GamepadFocusPreviousPressed = RepeatDigital(ButtonDown(m_mapping.FocusPrevious), m_repeatFocusPrevious);
            input.GamepadFocusNextPressed = RepeatDigital(ButtonDown(m_mapping.FocusNext), m_repeatFocusNext);
            input.GamepadPageLeftPressed = RepeatDigital(ButtonDown(m_mapping.PageLeft), m_repeatPageLeft);
            input.GamepadPageRightPressed = RepeatDigital(ButtonDown(m_mapping.PageRight), m_repeatPageRight);
            PopulateGenericActions(input);

            m_current = input;
            m_previous = input;
            m_previousPointerPosition = m_pointerPosition;
            m_hasPreviousPointer = true;
            for (size_t i = 0; i < 3; ++i)
                m_prevMouseDown[i] = m_mouseDown[i];
            for (size_t i = 0; i < KeyCount; ++i)
                m_prevKeyDown[i] = m_keyDown[i];
            for (size_t i = 0; i < static_cast<size_t>(GamepadButton::Count); ++i)
                m_prevGamepadDown[i] = m_gamepadDown[i];
            m_wheelDelta = 0.0f;
            m_textInput.clear();
            return input;
        }

        void Reset()
        {
            m_current = {};
            m_previous = {};
            m_pointerPosition = {};
            m_previousPointerPosition = {};
            m_hasPreviousPointer = false;
            m_deltaTime = 0.0f;
            m_wheelDelta = 0.0f;
            m_gamepadConnected = false;
            m_textInput.clear();
            std::fill(std::begin(m_mouseDown), std::end(m_mouseDown), false);
            std::fill(std::begin(m_prevMouseDown), std::end(m_prevMouseDown), false);
            std::fill(std::begin(m_keyDown), std::end(m_keyDown), false);
            std::fill(std::begin(m_prevKeyDown), std::end(m_prevKeyDown), false);
            std::fill(std::begin(m_gamepadDown), std::end(m_gamepadDown), false);
            std::fill(std::begin(m_prevGamepadDown), std::end(m_prevGamepadDown), false);
            std::fill(std::begin(m_gamepadAxis), std::end(m_gamepadAxis), 0.0f);
            m_repeatUp = m_repeatDown = m_repeatLeft = m_repeatRight = 0.0f;
            m_repeatFocusNext = m_repeatFocusPrevious = 0.0f;
            m_repeatPageLeft = m_repeatPageRight = 0.0f;
            m_keyRepeatUp = m_keyRepeatDown = m_keyRepeatLeft = m_keyRepeatRight = 0.0f;
            m_keyRepeatFocusNext = m_keyRepeatFocusPrevious = 0.0f;
            m_keyRepeatPageLeft = m_keyRepeatPageRight = 0.0f;
        }

    private:
        InputSnapshot m_current;
        InputSnapshot m_previous;
        bool m_mouseDown[3] = {};
        bool m_prevMouseDown[3] = {};
        bool m_keyDown[KeyCount] = {};
        bool m_prevKeyDown[KeyCount] = {};
        bool m_gamepadDown[static_cast<size_t>(GamepadButton::Count)] = {};
        bool m_prevGamepadDown[static_cast<size_t>(GamepadButton::Count)] = {};
        float m_gamepadAxis[static_cast<size_t>(GamepadAxis::Count)] = {};
        std::vector<uint32_t> m_textInput;
        Vec2 m_pointerPosition = {};
        Vec2 m_previousPointerPosition = {};
        bool m_hasPreviousPointer = false;
        bool m_gamepadConnected = false;
        float m_deltaTime = 0.0f;
        float m_wheelDelta = 0.0f;
        GamepadMapping m_mapping;
        InputRepeatSettings m_repeat;
        float m_repeatUp = 0.0f;
        float m_repeatDown = 0.0f;
        float m_repeatLeft = 0.0f;
        float m_repeatRight = 0.0f;
        float m_repeatFocusNext = 0.0f;
        float m_repeatFocusPrevious = 0.0f;
        float m_repeatPageLeft = 0.0f;
        float m_repeatPageRight = 0.0f;
        float m_keyRepeatUp = 0.0f;
        float m_keyRepeatDown = 0.0f;
        float m_keyRepeatLeft = 0.0f;
        float m_keyRepeatRight = 0.0f;
        float m_keyRepeatFocusNext = 0.0f;
        float m_keyRepeatFocusPrevious = 0.0f;
        float m_keyRepeatPageLeft = 0.0f;
        float m_keyRepeatPageRight = 0.0f;

        bool ButtonPressed(GamepadButton button) const
        {
            const size_t index = static_cast<size_t>(button);
            return index > 0 && index < static_cast<size_t>(GamepadButton::Count) && m_gamepadDown[index] && !m_prevGamepadDown[index];
        }

        bool ButtonDown(GamepadButton button) const
        {
            const size_t index = static_cast<size_t>(button);
            return index > 0 && index < static_cast<size_t>(GamepadButton::Count) && m_gamepadDown[index];
        }

        bool AxisPressed(GamepadAxis axis, float direction, float deadZone) const
        {
            const size_t index = static_cast<size_t>(axis);
            if (index >= static_cast<size_t>(GamepadAxis::Count))
                return false;
            const float value = m_gamepadAxis[index];
            return direction < 0.0f ? value <= -deadZone : value >= deadZone;
        }

        bool RepeatDigital(bool down, float& timer)
        {
            if (!down)
            {
                timer = 0.0f;
                return false;
            }
            if (timer <= 0.0f)
            {
                timer = m_repeat.InitialDelay;
                return true;
            }
            timer -= m_deltaTime;
            if (timer <= 0.0f)
            {
                timer = m_repeat.RepeatInterval;
                return true;
            }
            return false;
        }
    };

    inline GamepadButton GetMappedGamepadButton(UIAction action, const GamepadMapping& mapping)
    {
        switch (action)
        {
        case UIAction::Accept: return mapping.Accept;
        case UIAction::Cancel: return mapping.Cancel;
        case UIAction::Secondary: return mapping.Secondary;
        case UIAction::Details: return mapping.Details;
        case UIAction::Back: return mapping.Back;
        case UIAction::Menu: return mapping.Menu;
        case UIAction::NavigateUp: return mapping.NavigateUp;
        case UIAction::NavigateDown: return mapping.NavigateDown;
        case UIAction::NavigateLeft: return mapping.NavigateLeft;
        case UIAction::NavigateRight: return mapping.NavigateRight;
        case UIAction::FocusPrevious: return mapping.FocusPrevious;
        case UIAction::FocusNext: return mapping.FocusNext;
        case UIAction::PageLeft:
        case UIAction::PageUp: return mapping.PageLeft;
        case UIAction::PageRight:
        case UIAction::PageDown: return mapping.PageRight;
        default: return GamepadButton::None;
        }
    }

    inline InputGlyph GetGamepadGlyph(GamepadButton button, GamepadGlyphStyle style)
    {
        if (style == GamepadGlyphStyle::PlayStation)
        {
            switch (button)
            {
            case GamepadButton::FaceDown: return { "Cross", "ps_cross" };
            case GamepadButton::FaceRight: return { "Circle", "ps_circle" };
            case GamepadButton::FaceLeft: return { "Square", "ps_square" };
            case GamepadButton::FaceUp: return { "Triangle", "ps_triangle" };
            case GamepadButton::LeftShoulder: return { "L1", "ps_l1" };
            case GamepadButton::RightShoulder: return { "R1", "ps_r1" };
            case GamepadButton::LeftTrigger: return { "L2", "ps_l2" };
            case GamepadButton::RightTrigger: return { "R2", "ps_r2" };
            case GamepadButton::Back: return { "Create", "ps_create" };
            case GamepadButton::Start: return { "Options", "ps_options" };
            default: break;
            }
        }
        if (style == GamepadGlyphStyle::Switch)
        {
            switch (button)
            {
            case GamepadButton::FaceDown: return { "B", "switch_b" };
            case GamepadButton::FaceRight: return { "A", "switch_a" };
            case GamepadButton::FaceLeft: return { "Y", "switch_y" };
            case GamepadButton::FaceUp: return { "X", "switch_x" };
            case GamepadButton::LeftShoulder: return { "L", "switch_l" };
            case GamepadButton::RightShoulder: return { "R", "switch_r" };
            case GamepadButton::LeftTrigger: return { "ZL", "switch_zl" };
            case GamepadButton::RightTrigger: return { "ZR", "switch_zr" };
            case GamepadButton::Back: return { "-", "switch_minus" };
            case GamepadButton::Start: return { "+", "switch_plus" };
            default: break;
            }
        }
        if (style == GamepadGlyphStyle::Xbox)
        {
            switch (button)
            {
            case GamepadButton::FaceDown: return { "A", "xbox_a" };
            case GamepadButton::FaceRight: return { "B", "xbox_b" };
            case GamepadButton::FaceLeft: return { "X", "xbox_x" };
            case GamepadButton::FaceUp: return { "Y", "xbox_y" };
            case GamepadButton::LeftShoulder: return { "LB", "xbox_lb" };
            case GamepadButton::RightShoulder: return { "RB", "xbox_rb" };
            case GamepadButton::LeftTrigger: return { "LT", "xbox_lt" };
            case GamepadButton::RightTrigger: return { "RT", "xbox_rt" };
            case GamepadButton::Back: return { "View", "xbox_view" };
            case GamepadButton::Start: return { "Menu", "xbox_menu" };
            default: break;
            }
        }

        switch (button)
        {
        case GamepadButton::FaceDown: return { "Accept", "generic_accept" };
        case GamepadButton::FaceRight: return { "Cancel", "generic_cancel" };
        case GamepadButton::FaceLeft: return { "Secondary", "generic_secondary" };
        case GamepadButton::FaceUp: return { "Details", "generic_details" };
        case GamepadButton::DPadUp: return { "Up", "generic_dpad_up" };
        case GamepadButton::DPadDown: return { "Down", "generic_dpad_down" };
        case GamepadButton::DPadLeft: return { "Left", "generic_dpad_left" };
        case GamepadButton::DPadRight: return { "Right", "generic_dpad_right" };
        case GamepadButton::LeftShoulder: return { "L", "generic_l" };
        case GamepadButton::RightShoulder: return { "R", "generic_r" };
        case GamepadButton::Back: return { "Back", "generic_back" };
        case GamepadButton::Start: return { "Menu", "generic_menu" };
        default: return { "", "" };
        }
    }

    inline InputGlyph GetGamepadGlyph(UIAction action, GamepadGlyphStyle style, const GamepadMapping& mapping)
    {
        return GetGamepadGlyph(GetMappedGamepadButton(action, mapping), style);
    }

    inline InputGlyph GetInputGlyph(UIAction action, InputDeviceKind device, GamepadGlyphStyle style, const GamepadMapping& mapping)
    {
        if (device == InputDeviceKind::Gamepad)
            return GetGamepadGlyph(action, style, mapping);

        switch (action)
        {
        case UIAction::Accept: return { "Enter", "keyboard_enter" };
        case UIAction::Cancel: return { "Esc", "keyboard_escape" };
        case UIAction::Secondary: return { "Shift", "keyboard_shift" };
        case UIAction::Details: return { "F1", "keyboard_f1" };
        case UIAction::Back: return { "Back", "keyboard_backspace" };
        case UIAction::Menu: return { "Menu", "keyboard_menu" };
        case UIAction::NavigateUp: return { "Up", "keyboard_up" };
        case UIAction::NavigateDown: return { "Down", "keyboard_down" };
        case UIAction::NavigateLeft: return { "Left", "keyboard_left" };
        case UIAction::NavigateRight: return { "Right", "keyboard_right" };
        case UIAction::FocusNext: return { "Tab", "keyboard_tab" };
        case UIAction::FocusPrevious: return { "Shift+Tab", "keyboard_shift_tab" };
        case UIAction::PageLeft:
        case UIAction::PageUp: return { "PageUp", "keyboard_page_up" };
        case UIAction::PageRight:
        case UIAction::PageDown: return { "PageDown", "keyboard_page_down" };
        case UIAction::Home: return { "Home", "keyboard_home" };
        case UIAction::End: return { "End", "keyboard_end" };
        default: return { "", "" };
        }
    }

#if FRIENDLYGUI_USE_IMGUI_INPUT
    inline InputSnapshot BuildInputFromImGuiAdvanced()
    {
        return Context::BuildInputFromImGui();
    }
#endif

    inline bool RunFriendlyInputSmokeTest()
    {
        InputBuilder builder;
        builder.BeginFrame(1.0f / 60.0f);
        builder.SetPointerPosition({ 10.0f, 20.0f });
        builder.SetMouseButton(MouseButton::Left, true);
        builder.SetKey(Key::A, true);
        builder.SetGamepadConnected(true);
        builder.SetGamepadButton(GamepadButton::FaceDown, true);
        builder.AddTextInputUtf8("A\xC3\xB1");
        InputSnapshot first = builder.EndFrame();
        if (!first.MousePressed[0] || !first.KeyPressed[static_cast<size_t>(Key::A)] || !first.GamepadAcceptPressed || !first.AcceptPressed)
            return false;
        if (first.LastInputDevice != InputDeviceKind::Gamepad)
            return false;
        if (first.TextInputCodepoints.size() != 2 || first.TextInputCodepoints[1] != 0xF1)
            return false;

        builder.BeginFrame(0.016f);
        builder.SetPointerPosition({ 15.0f, 25.0f });
        builder.SetMouseButton(MouseButton::Left, false);
        builder.SetKey(Key::A, false);
        builder.SetGamepadButton(GamepadButton::FaceDown, false);
        InputSnapshot second = builder.EndFrame();
        if (!second.MouseReleased[0] || !second.KeyReleased[static_cast<size_t>(Key::A)])
            return false;
        if (second.LastInputDevice != InputDeviceKind::MouseKeyboard)
            return false;
        if (second.PointerDelta.x != 5.0f || second.PointerDelta.y != 5.0f)
            return false;

        InputRepeatSettings repeat;
        repeat.InitialDelay = 0.05f;
        repeat.RepeatInterval = 0.025f;
        builder.SetRepeatSettings(repeat);
        builder.BeginFrame(0.016f);
        builder.SetGamepadButton(GamepadButton::DPadDown, true);
        InputSnapshot nav0 = builder.EndFrame();
        builder.BeginFrame(0.060f);
        builder.SetGamepadButton(GamepadButton::DPadDown, true);
        InputSnapshot nav1 = builder.EndFrame();
        builder.BeginFrame(0.030f);
        builder.SetGamepadButton(GamepadButton::DPadDown, true);
        InputSnapshot nav2 = builder.EndFrame();
        if (!nav0.GamepadNavigateDownPressed || !nav1.GamepadNavigateDownPressed || !nav2.GamepadNavigateDownPressed)
            return false;

        builder.BeginFrame(0.016f);
        builder.SetGamepadButton(GamepadButton::DPadDown, false);
        builder.SetKey(Key::Down, true);
        InputSnapshot keyNav0 = builder.EndFrame();
        builder.BeginFrame(0.060f);
        builder.SetKey(Key::Down, true);
        InputSnapshot keyNav1 = builder.EndFrame();
        if (!keyNav0.NavigateDownPressed || !keyNav1.NavigateDownPressed)
            return false;

        const GamepadMapping mapping;
        if (GetGamepadGlyph(UIAction::Accept, GamepadGlyphStyle::Xbox, mapping).Text != "A")
            return false;
        if (GetGamepadGlyph(UIAction::Cancel, GamepadGlyphStyle::PlayStation, mapping).Text != "Circle")
            return false;
        if (GetInputGlyph(UIAction::FocusPrevious, InputDeviceKind::MouseKeyboard, GamepadGlyphStyle::Xbox, mapping).Text != "Shift+Tab")
            return false;
        return true;
    }
}
