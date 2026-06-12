#ifndef FRIENDLY_CONTROLS_H
#define FRIENDLY_CONTROLS_H

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cfloat>
#include <cstdlib>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#if __has_include("imgui.h")
#include "imgui.h"
#else
#error "FriendlyControls.h requires Dear ImGui. Make imgui.h available before including this header."
#endif

#ifndef FRIENDLYGUI_USE_FRIENDLY_INPUT
#define FRIENDLYGUI_USE_FRIENDLY_INPUT 0
#endif

#ifndef FRIENDLYGUI_USE_IMGUI_INPUT
#define FRIENDLYGUI_USE_IMGUI_INPUT 1
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifndef FRIENDLYGUI_USE_GAME_CONTROLS
#define FRIENDLYGUI_USE_GAME_CONTROLS 1
#endif

#ifndef FRIENDLYGUI_USE_ADVANCED_EVENTS
#define FRIENDLYGUI_USE_ADVANCED_EVENTS 1
#endif

#ifndef FRIENDLYGUI_USE_DRAG_DROP
#define FRIENDLYGUI_USE_DRAG_DROP 1
#endif

#ifndef FRIENDLYGUI_USE_TOUCH_MANIPULATION
#define FRIENDLYGUI_USE_TOUCH_MANIPULATION 1
#endif

namespace FyGUI
{
    inline constexpr float AutoSize = -1.0f;
    inline constexpr float InfiniteSize = 100000.0f;
    inline constexpr uint32_t VersionMajor = 0;
    inline constexpr uint32_t VersionMinor = 0;
    inline constexpr uint32_t VersionPatch = 0;

    inline bool& FocusVisualsEnabledStorage()
    {
        static bool enabled = true;
        return enabled;
    }

    inline void SetFocusVisualsEnabled(bool enabled)
    {
        FocusVisualsEnabledStorage() = enabled;
    }

    inline bool GetFocusVisualsEnabled()
    {
        return FocusVisualsEnabledStorage();
    }

    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;

        Vec2() = default;
        Vec2(float px, float py) : x(px), y(py) {}
        Vec2(const ImVec2& value) : x(value.x), y(value.y) {}
        operator ImVec2() const { return ImVec2(x, y); }
    };

    inline Vec2 operator+(Vec2 a, Vec2 b) { return { a.x + b.x, a.y + b.y }; }
    inline Vec2 operator-(Vec2 a, Vec2 b) { return { a.x - b.x, a.y - b.y }; }
    inline Vec2 operator*(Vec2 a, float s) { return { a.x * s, a.y * s }; }

    struct Rect
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;

        float Left() const { return x; }
        float Top() const { return y; }
        float Right() const { return x + width; }
        float Bottom() const { return y + height; }
        bool Contains(Vec2 p) const { return p.x >= Left() && p.x <= Right() && p.y >= Top() && p.y <= Bottom(); }
    };

    struct Color
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;

        uint32_t ToU32(float inheritedOpacity = 1.0f) const
        {
            const float alpha = std::clamp(a * inheritedOpacity, 0.0f, 1.0f);
            const uint32_t rr = static_cast<uint32_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
            const uint32_t gg = static_cast<uint32_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
            const uint32_t bb = static_cast<uint32_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
            const uint32_t aa = static_cast<uint32_t>(alpha * 255.0f + 0.5f);
            return rr | (gg << 8) | (bb << 16) | (aa << 24);
        }

        operator ImU32() const { return ToU32(); }

        static Color FromU32(uint32_t color)
        {
            return {
                static_cast<float>(color & 0xFF) / 255.0f,
                static_cast<float>((color >> 8) & 0xFF) / 255.0f,
                static_cast<float>((color >> 16) & 0xFF) / 255.0f,
                static_cast<float>((color >> 24) & 0xFF) / 255.0f
            };
        }
    };

    using TextureId = ImTextureID;

    struct Thickness
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;

        Thickness() = default;
        explicit Thickness(float uniform) : left(uniform), top(uniform), right(uniform), bottom(uniform) {}
        Thickness(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}

        float Horizontal() const { return left + right; }
        float Vertical() const { return top + bottom; }
    };

    struct ImageSource
    {
        TextureId texture = 0;
        Vec2 size = {};
        Vec2 uv0 = { 0.0f, 0.0f };
        Vec2 uv1 = { 1.0f, 1.0f };
    };

    enum class VisualState
    {
        Normal,
        PointerOver,
        Pressed,
        Disabled,
        Focused,
        Checked,
        Selected
    };

    struct ImageSlice
    {
        TextureId Texture = 0;
        float Left = 0.0f;
        float Top = 0.0f;
        float Right = 0.0f;
        float Bottom = 0.0f;
        float TextureWidth = 0.0f;
        float TextureHeight = 0.0f;
        bool Enabled = false;
    };

    struct ControlPartStyle
    {
        Color BackgroundColor = Color{ 1.0f, 1.0f, 1.0f, 0.0f };
        Color BorderColor = Color{ 0.0f, 0.0f, 0.0f, 0.0f };
        Color ForegroundColor = Color{ 1.0f, 1.0f, 1.0f, 0.0f };
        float BorderThickness = 0.0f;
        float CornerRadius = 0.0f;
        float Opacity = 1.0f;
        TextureId Image = 0;
        TextureId HoverImage = 0;
        TextureId PressedImage = 0;
        TextureId DisabledImage = 0;
        TextureId FocusedImage = 0;
        TextureId CheckedImage = 0;
        TextureId SelectedImage = 0;
        ImageSlice NineSlice;
        bool UseImage = false;
        bool UseNineSlice = false;
        bool StretchImage = true;
    };

    inline Color MultiplyColor(Color a, Color b)
    {
        return { a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a };
    }

    inline Color WithOpacity(Color color, float opacity)
    {
        color.a *= opacity;
        return color;
    }

    inline Color LerpColor(Color a, Color b, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return {
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        };
    }

    inline Color BrightenColor(Color color, float amount)
    {
        amount = std::clamp(amount, -1.0f, 1.0f);
        if (amount >= 0.0f)
            return LerpColor(color, Color{ 1.0f, 1.0f, 1.0f, color.a }, amount);
        return LerpColor(color, Color{ 0.0f, 0.0f, 0.0f, color.a }, -amount);
    }

    inline bool EqualsIgnoreCaseAscii(std::string_view a, std::string_view b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
        {
            const char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
            const char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
            if (ca != cb)
                return false;
        }
        return true;
    }

    inline Color RarityAccentColor(std::string_view rarity)
    {
        if (EqualsIgnoreCaseAscii(rarity, "common"))
            return { 0.70f, 0.78f, 0.86f, 1.0f };
        if (EqualsIgnoreCaseAscii(rarity, "uncommon"))
            return { 0.50f, 0.92f, 0.62f, 1.0f };
        if (EqualsIgnoreCaseAscii(rarity, "rare"))
            return { 0.38f, 0.72f, 1.0f, 1.0f };
        if (EqualsIgnoreCaseAscii(rarity, "epic"))
            return { 0.82f, 0.58f, 1.0f, 1.0f };
        if (EqualsIgnoreCaseAscii(rarity, "legendary"))
            return { 1.0f, 0.72f, 0.30f, 1.0f };
        return { 0.76f, 0.90f, 1.0f, 1.0f };
    }

    inline Color ResolvePartForeground(const ControlPartStyle& style, Color fallback)
    {
        return style.ForegroundColor.a > 0.0f ? style.ForegroundColor : fallback;
    }

    inline float EaseOutCubic(float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        const float inverse = 1.0f - t;
        return 1.0f - inverse * inverse * inverse;
    }

    inline Rect ScaleRectFromCenter(Rect rect, float scale)
    {
        scale = std::max(0.0f, scale);
        const float width = rect.width * scale;
        const float height = rect.height * scale;
        return 
        {
            rect.x + (rect.width - width) * 0.5f,
            rect.y + (rect.height - height) * 0.5f,
            width,
            height
        };
    }

    inline Rect DeflateRect(Rect rect, Thickness thickness)
    {
        rect.x += thickness.left;
        rect.y += thickness.top;
        rect.width = std::max(0.0f, rect.width - thickness.Horizontal());
        rect.height = std::max(0.0f, rect.height - thickness.Vertical());
        return rect;
    }

    inline Rect InflateRect(Rect rect, Thickness thickness)
    {
        rect.x -= thickness.left;
        rect.y -= thickness.top;
        rect.width = std::max(0.0f, rect.width + thickness.Horizontal());
        rect.height = std::max(0.0f, rect.height + thickness.Vertical());
        return rect;
    }

    inline ControlPartStyle ApplyPartOpacity(ControlPartStyle style, float opacity)
    {
        style.Opacity *= std::clamp(opacity, 0.0f, 1.0f);
        return style;
    }

    inline bool HasStyledPartVisual(const ControlPartStyle& style)
    {
        return style.BackgroundColor.a > 0.0f || style.BorderColor.a > 0.0f ||
            style.UseImage || style.UseNineSlice || style.BorderThickness > 0.0f;
    }

    inline TextureId ResolveImageForState(const ControlPartStyle& style, VisualState state)
    {
        if (state == VisualState::Pressed && style.PressedImage)
            return style.PressedImage;
        if (state == VisualState::PointerOver && style.HoverImage)
            return style.HoverImage;
        if (state == VisualState::Disabled && style.DisabledImage)
            return style.DisabledImage;
        if (state == VisualState::Focused && style.FocusedImage)
            return style.FocusedImage;
        if (state == VisualState::Checked && style.CheckedImage)
            return style.CheckedImage;
        if (state == VisualState::Selected && style.SelectedImage)
            return style.SelectedImage;
        return style.Image;
    }

    inline void DrawNineSlice(
        ImDrawList& drawList,
        TextureId texture,
        const Rect& destination,
        float left,
        float top,
        float right,
        float bottom,
        Color tint,
        float textureWidth = 0.0f,
        float textureHeight = 0.0f)
    {
        if (!texture || destination.width <= 0.0f || destination.height <= 0.0f)
            return;

        const float safeWidth = std::max(0.0f, destination.width);
        const float safeHeight = std::max(0.0f, destination.height);
        const float sourceWidth = textureWidth > 0.0f ? textureWidth : safeWidth;
        const float sourceHeight = textureHeight > 0.0f ? textureHeight : safeHeight;
        const float sourceLeft = std::clamp(left, 0.0f, sourceWidth * 0.5f);
        const float sourceRight = std::clamp(right, 0.0f, sourceWidth * 0.5f);
        const float sourceTop = std::clamp(top, 0.0f, sourceHeight * 0.5f);
        const float sourceBottom = std::clamp(bottom, 0.0f, sourceHeight * 0.5f);
        const float l = std::clamp(sourceLeft, 0.0f, safeWidth * 0.5f);
        const float r = std::clamp(sourceRight, 0.0f, safeWidth * 0.5f);
        const float t = std::clamp(sourceTop, 0.0f, safeHeight * 0.5f);
        const float b = std::clamp(sourceBottom, 0.0f, safeHeight * 0.5f);

        const float x[4] = { destination.Left(), destination.Left() + l, destination.Right() - r, destination.Right() };
        const float y[4] = { destination.Top(), destination.Top() + t, destination.Bottom() - b, destination.Bottom() };

        const float uLeft = sourceWidth > 0.0f ? std::clamp(sourceLeft / sourceWidth, 0.0f, 0.5f) : 0.0f;
        const float uRight = sourceWidth > 0.0f ? std::clamp(1.0f - (sourceRight / sourceWidth), 0.5f, 1.0f) : 1.0f;
        const float vTop = sourceHeight > 0.0f ? std::clamp(sourceTop / sourceHeight, 0.0f, 0.5f) : 0.0f;
        const float vBottom = sourceHeight > 0.0f ? std::clamp(1.0f - (sourceBottom / sourceHeight), 0.5f, 1.0f) : 1.0f;
        const float u[4] = { 0.0f, uLeft, uRight, 1.0f };
        const float v[4] = { 0.0f, vTop, vBottom, 1.0f };

        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                if (x[col + 1] <= x[col] || y[row + 1] <= y[row])
                    continue;
                drawList.AddImage(texture,
                    Vec2{ x[col], y[row] },
                    Vec2{ x[col + 1], y[row + 1] },
                    Vec2{ u[col], v[row] },
                    Vec2{ u[col + 1], v[row + 1] },
                    tint.ToU32());
            }
        }
    }

    inline void DrawStyledPart(
        ImDrawList& drawList,
        const Rect& rect,
        const ControlPartStyle& style,
        VisualState state,
        Color optionalTint = Color{ 1.0f, 1.0f, 1.0f, 1.0f })
    {
        if (rect.width <= 0.0f || rect.height <= 0.0f)
            return;

        const Color baseTint = style.ForegroundColor.a > 0.0f ? style.ForegroundColor : Color{ 1.0f, 1.0f, 1.0f, 1.0f };
        const Color tint = MultiplyColor(baseTint, optionalTint);
        if (style.UseNineSlice && style.NineSlice.Enabled && style.NineSlice.Texture)
        {
            DrawNineSlice(drawList, style.NineSlice.Texture, rect, style.NineSlice.Left, style.NineSlice.Top, style.NineSlice.Right, style.NineSlice.Bottom, tint, style.NineSlice.TextureWidth, style.NineSlice.TextureHeight);
        }
        else if (style.UseImage)
        {
            TextureId image = ResolveImageForState(style, state);
            if (image)
            {
                drawList.AddImage(image, Vec2(rect.Left(), rect.Top()), Vec2(rect.Right(), rect.Bottom()), Vec2{ 0.0f, 0.0f }, Vec2{ 1.0f, 1.0f }, tint.ToU32(style.Opacity));
            }
            else if (style.BackgroundColor.a > 0.0f)
            {
                drawList.AddRectFilled(Vec2(rect.Left(), rect.Top()), Vec2(rect.Right(), rect.Bottom()), style.BackgroundColor.ToU32(style.Opacity), style.CornerRadius);
            }
        }
        else if (style.BackgroundColor.a > 0.0f)
        {
            drawList.AddRectFilled(Vec2(rect.Left(), rect.Top()), Vec2(rect.Right(), rect.Bottom()), style.BackgroundColor.ToU32(style.Opacity), style.CornerRadius);
        }

        if (style.BorderThickness > 0.0f && style.BorderColor.a > 0.0f)
            drawList.AddRect(Vec2(rect.Left(), rect.Top()), Vec2(rect.Right(), rect.Bottom()), style.BorderColor.ToU32(style.Opacity), style.CornerRadius, ImDrawFlags_None, style.BorderThickness);
    }

    struct ButtonStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_ContentPresenter;
        ControlPartStyle PART_Icon;
        ControlPartStyle PART_Text;
        ControlPartStyle PART_FocusVisual;
    };

    struct CheckBoxStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Box;
        ControlPartStyle PART_CheckMark;
        ControlPartStyle PART_ContentPresenter;
        ControlPartStyle PART_Text;
        ControlPartStyle PART_FocusVisual;
    };

    struct SliderStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Track;
        ControlPartStyle PART_FillTrack;
        ControlPartStyle PART_Thumb;
        ControlPartStyle PART_ThumbGrip;
        ControlPartStyle PART_TickBar;
    };

    struct ProgressBarStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Track;
        ControlPartStyle PART_Fill;
        ControlPartStyle PART_Overlay;
        ControlPartStyle PART_Text;
    };

    struct BorderStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_ContentPresenter;
    };

    struct ImageStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Image;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_Overlay;
    };

    struct TextBoxStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_TextPresenter;
        ControlPartStyle PART_PlaceholderText;
        ControlPartStyle PART_Caret;
        ControlPartStyle PART_FocusVisual;
    };

    struct ScrollBarStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Track;
        ControlPartStyle PART_Thumb;
        ControlPartStyle PART_ThumbGrip;
        ControlPartStyle PART_DecreaseButton;
        ControlPartStyle PART_IncreaseButton;
        ControlPartStyle PART_DecreaseGlyph;
        ControlPartStyle PART_IncreaseGlyph;
        ControlPartStyle PART_DecreasePageArea;
        ControlPartStyle PART_IncreasePageArea;
    };

    struct ScrollViewerStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_ContentPresenter;
        ControlPartStyle PART_VerticalScrollBar;
        ControlPartStyle PART_HorizontalScrollBar;
        ControlPartStyle PART_ScrollCorner;
        ScrollBarStyle VerticalScrollBarStyle;
        ScrollBarStyle HorizontalScrollBarStyle;
    };

    struct ListBoxItemStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Icon;
        ControlPartStyle PART_Text;
        ControlPartStyle PART_Selection;
        ControlPartStyle PART_Hover;
        ControlPartStyle PART_Badge;
    };

    struct ListBoxStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_ScrollViewer;
        ControlPartStyle PART_ItemsPresenter;
        ControlPartStyle PART_ItemContainer;
        ControlPartStyle PART_SelectionVisual;
        ControlPartStyle PART_HoverVisual;
        ListBoxItemStyle ItemStyle;
    };

    struct ComboBoxStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_SelectionBox;
        ControlPartStyle PART_DropDownButton;
        ControlPartStyle PART_DropDownGlyph;
        ControlPartStyle PART_Popup;
        ControlPartStyle PART_ItemsPresenter;
        ControlPartStyle PART_HeaderText;
        ControlPartStyle PART_Text;
        ListBoxItemStyle ItemStyle;
    };

    struct InventorySlotStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_ItemIcon;
        ControlPartStyle PART_RarityFrame;
        ControlPartStyle PART_QuantityText;
        ControlPartStyle PART_CooldownOverlay;
        ControlPartStyle PART_SelectionGlow;
        ControlPartStyle PART_HoverGlow;
        ControlPartStyle PART_DisabledOverlay;
        ControlPartStyle PART_DragGhost;
    };

    struct InventoryGridStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_ItemsPresenter;
        ControlPartStyle PART_DropPreview;
        InventorySlotStyle SlotStyle;
    };

    struct TreeViewStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_ItemsPresenter;
        ControlPartStyle PART_ItemContainer;
        ControlPartStyle PART_SelectionVisual;
        ControlPartStyle PART_HoverVisual;
        ControlPartStyle PART_ExpanderGlyph;
        ControlPartStyle PART_Text;
    };

    struct TabViewStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_HeaderPanel;
        ControlPartStyle PART_TabViewItem;
        ControlPartStyle PART_SelectedTabViewItem;
        ControlPartStyle PART_TabText;
        ControlPartStyle PART_ContentPresenter;
        ControlPartStyle PART_FocusVisual;
    };

    struct ExpanderStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Header;
        ControlPartStyle PART_ExpanderGlyph;
        ControlPartStyle PART_HeaderText;
        ControlPartStyle PART_ContentPresenter;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_FocusVisual;
    };

    struct ToggleSwitchStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_HeaderText;
        ControlPartStyle PART_Track;
        ControlPartStyle PART_Thumb;
        ControlPartStyle PART_Text;
        ControlPartStyle PART_FocusVisual;
    };

    struct InfoBarStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_Icon;
        ControlPartStyle PART_TitleText;
        ControlPartStyle PART_MessageText;
        ControlPartStyle PART_CloseButton;
        ControlPartStyle PART_CloseGlyph;
    };

    struct SeparatorStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Line;
    };

    struct CommandBarStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_ContentPresenter;
    };

    struct AppBarButtonStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_Icon;
        ControlPartStyle PART_Text;
        ControlPartStyle PART_FocusVisual;
    };

    struct WindowStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_Border;
        ControlPartStyle PART_TitleBar;
        ControlPartStyle PART_TitleText;
        ControlPartStyle PART_CloseButton;
        ControlPartStyle PART_ContentPresenter;
        ControlPartStyle PART_CommandSpace;
        ControlPartStyle PART_PrimaryButton;
        ControlPartStyle PART_SecondaryButton;
        ControlPartStyle PART_Shadow;
    };

    struct ContentDialogStyle : WindowStyle
    {
    };

    enum class ThemePreset : uint8_t
    {
        SciFi,
        Fantasy,
        Tactical,
        Minimal
    };

    struct StylePalette
    {
        Color Surface;
        Color SurfaceAlt;
        Color Accent;
        Color AccentAlt;
        Color Text;
        Color MutedText;
        Color Border;
        Color Danger;
        float Radius = 6.0f;
    };

    inline ControlPartStyle SolidPart(Color background, Color border = Color{ 0.0f, 0.0f, 0.0f, 0.0f }, float borderThickness = 0.0f, float radius = 0.0f, Color foreground = Color{ 1.0f, 1.0f, 1.0f, 0.0f })
    {
        ControlPartStyle part {};
        part.BackgroundColor = background;
        part.BorderColor = border;
        part.BorderThickness = borderThickness;
        part.CornerRadius = radius;
        part.ForegroundColor = foreground;
        return part;
    }

    inline StylePalette MakeStylePalette(ThemePreset preset)
    {
        switch (preset)
        {
        case ThemePreset::Fantasy:
            return 
            {
                Color{ 0.105f, 0.070f, 0.110f, 0.92f },
                Color{ 0.185f, 0.115f, 0.170f, 0.96f },
                Color{ 1.00f, 0.70f, 0.46f, 1.0f },
                Color{ 0.76f, 0.94f, 0.68f, 1.0f },
                Color{ 1.00f, 0.92f, 0.86f, 1.0f },
                Color{ 0.82f, 0.70f, 0.82f, 1.0f },
                Color{ 0.72f, 0.46f, 0.62f, 0.88f },
                Color{ 1.00f, 0.38f, 0.34f, 1.0f },
                13.0f
            };
        case ThemePreset::Tactical:
            return 
            {
                Color{ 0.050f, 0.070f, 0.065f, 0.94f },
                Color{ 0.080f, 0.115f, 0.105f, 0.97f },
                Color{ 0.58f, 0.92f, 0.62f, 1.0f },
                Color{ 0.90f, 0.80f, 0.46f, 1.0f },
                Color{ 0.90f, 0.98f, 0.92f, 1.0f },
                Color{ 0.68f, 0.80f, 0.74f, 1.0f },
                Color{ 0.34f, 0.58f, 0.48f, 0.88f },
                Color{ 1.00f, 0.42f, 0.36f, 1.0f },
                8.0f
            };
        case ThemePreset::Minimal:
            return 
            {
                Color{ 0.070f, 0.075f, 0.105f, 0.86f },
                Color{ 0.115f, 0.125f, 0.170f, 0.92f },
                Color{ 0.58f, 0.74f, 1.00f, 1.0f },
                Color{ 0.92f, 0.68f, 1.00f, 1.0f },
                Color{ 0.94f, 0.96f, 1.00f, 1.0f },
                Color{ 0.70f, 0.74f, 0.86f, 1.0f },
                Color{ 0.48f, 0.54f, 0.72f, 0.82f },
                Color{ 1.00f, 0.48f, 0.56f, 1.0f },
                14.0f
            };
        case ThemePreset::SciFi:
        default:
            return 
            {
                Color{ 0.025f, 0.040f, 0.070f, 0.90f },
                Color{ 0.060f, 0.090f, 0.140f, 0.94f },
                Color{ 0.42f, 0.88f, 1.00f, 1.0f },
                Color{ 1.00f, 0.74f, 0.92f, 1.0f },
                Color{ 0.90f, 0.97f, 1.00f, 1.0f },
                Color{ 0.64f, 0.76f, 0.90f, 1.0f },
                Color{ 0.34f, 0.62f, 0.88f, 0.86f },
                Color{ 1.00f, 0.42f, 0.46f, 1.0f },
                12.0f
            };
        }
    }

    inline ButtonStyle MakeButtonStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        ButtonStyle style {};
        style.PART_Background = SolidPart(p.SurfaceAlt, p.Border, 1.0f, p.Radius, p.Text);
        style.PART_Background.HoverImage = 0;
        style.PART_Border = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.Border, 1.0f, p.Radius);
        style.PART_Text.ForegroundColor = p.Text;
        style.PART_FocusVisual = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.AccentAlt, 2.0f, p.Radius + 1.0f);
        return style;
    }

    inline SliderStyle MakeSliderStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        SliderStyle style {};
        style.PART_Track = SolidPart(p.Surface, p.Border, 1.0f, p.Radius * 0.5f);
        style.PART_FillTrack = SolidPart(p.Accent, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius * 0.5f);
        style.PART_Thumb = SolidPart(p.SurfaceAlt, p.Accent, 1.0f, p.Radius + 3.0f);
        style.PART_ThumbGrip = SolidPart(Color{ 1.0f, 1.0f, 1.0f, 0.14f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius);
        return style;
    }

    inline ProgressBarStyle MakeProgressBarStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        ProgressBarStyle style {};
        style.PART_Background = SolidPart(p.Surface, p.Border, 1.0f, p.Radius);
        style.PART_Fill = SolidPart(p.Accent, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius);
        style.PART_Overlay = SolidPart(Color{ 1.0f, 1.0f, 1.0f, 0.06f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius);
        style.PART_Text.ForegroundColor = p.Text;
        return style;
    }

    inline ScrollBarStyle MakeScrollBarStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        ScrollBarStyle style {};
        style.PART_Track = SolidPart(p.Surface, p.Border, 1.0f, p.Radius * 0.5f);
        style.PART_Thumb = SolidPart(p.SurfaceAlt, p.Accent, 1.0f, p.Radius);
        style.PART_ThumbGrip = SolidPart(Color{ 1.0f, 1.0f, 1.0f, 0.16f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius);
        style.PART_DecreaseButton = SolidPart(p.SurfaceAlt, p.Border, 1.0f, p.Radius * 0.5f);
        style.PART_IncreaseButton = style.PART_DecreaseButton;
        style.PART_DecreaseGlyph.ForegroundColor = p.Text;
        style.PART_IncreaseGlyph.ForegroundColor = p.Text;
        style.PART_DecreasePageArea = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.12f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius * 0.5f);
        style.PART_IncreasePageArea = style.PART_DecreasePageArea;
        return style;
    }

    inline TextBoxStyle MakeTextBoxStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        TextBoxStyle style {};
        style.PART_Background = SolidPart(p.Surface, p.Border, 1.0f, p.Radius, p.Text);
        style.PART_TextPresenter.ForegroundColor = p.Text;
        style.PART_PlaceholderText.ForegroundColor = p.MutedText;
        style.PART_Caret.BackgroundColor = p.Accent;
        style.PART_FocusVisual = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.AccentAlt, 2.0f, p.Radius + 1.0f);
        return style;
    }

    inline ListBoxStyle MakeListBoxStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        ListBoxStyle style {};
        style.PART_Background = SolidPart(p.Surface, p.Border, 1.0f, p.Radius);
        style.PART_HoverVisual = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.16f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius * 0.5f);
        style.PART_SelectionVisual = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.42f }, p.AccentAlt, 1.0f, p.Radius * 0.5f);
        style.PART_ItemContainer.ForegroundColor = p.Text;
        return style;
    }

    inline ComboBoxStyle MakeComboBoxStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        ComboBoxStyle style {};
        style.PART_Background = SolidPart(p.Surface, p.Border, 1.0f, p.Radius, p.Text);
        style.PART_SelectionBox = SolidPart(Color{ p.SurfaceAlt.r, p.SurfaceAlt.g, p.SurfaceAlt.b, 0.76f }, p.Border, 1.0f, p.Radius);
        style.PART_DropDownButton = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.18f }, p.Accent, 1.0f, p.Radius);
        style.PART_DropDownGlyph.ForegroundColor = p.Text;
        style.PART_Popup = SolidPart(Color{ p.SurfaceAlt.r, p.SurfaceAlt.g, p.SurfaceAlt.b, 0.98f }, p.Border, 1.0f, p.Radius + 2.0f);
        style.PART_ItemsPresenter = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.72f });
        style.PART_HeaderText.ForegroundColor = p.Text;
        style.PART_Text.ForegroundColor = p.Text;
        style.ItemStyle.PART_Text.ForegroundColor = p.Text;
        style.ItemStyle.PART_Hover = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.18f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius * 0.5f);
        style.ItemStyle.PART_Selection = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.38f }, p.AccentAlt, 1.0f, p.Radius * 0.5f);
        return style;
    }

    inline TreeViewStyle MakeTreeViewStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        TreeViewStyle style {};
        style.PART_Background = SolidPart(p.Surface, p.Border, 1.0f, p.Radius);
        style.PART_ItemsPresenter = SolidPart(Color{ p.SurfaceAlt.r, p.SurfaceAlt.g, p.SurfaceAlt.b, 0.24f });
        style.PART_ItemContainer = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius * 0.5f);
        style.PART_HoverVisual = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.12f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius * 0.5f);
        style.PART_SelectionVisual = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.34f }, p.AccentAlt, 1.0f, p.Radius * 0.5f);
        style.PART_ExpanderGlyph.ForegroundColor = p.Accent;
        style.PART_Text.ForegroundColor = p.Text;
        return style;
    }

    inline BorderStyle MakePopupStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        BorderStyle style {};
        style.PART_Background = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.96f }, p.Border, 1.0f, p.Radius + 3.0f);
        style.PART_Border = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.Accent, 1.0f, p.Radius + 3.0f);
        style.PART_ContentPresenter = SolidPart(Color{ p.SurfaceAlt.r, p.SurfaceAlt.g, p.SurfaceAlt.b, 0.28f });
        return style;
    }

    inline ButtonStyle MakeMenuItemStyle(ThemePreset preset)
    {
        ButtonStyle style = MakeButtonStyle(preset);
        const StylePalette p = MakeStylePalette(preset);
        style.PART_Background = SolidPart(Color{ p.SurfaceAlt.r, p.SurfaceAlt.g, p.SurfaceAlt.b, 0.70f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius * 0.5f, p.Text);
        style.PART_Border = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, Color{ p.Border.r, p.Border.g, p.Border.b, 0.38f }, 1.0f, p.Radius * 0.5f);
        return style;
    }

    inline TabViewStyle MakeTabViewStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        TabViewStyle style {};
        style.PART_Background = SolidPart(p.Surface, p.Border, 1.0f, p.Radius);
        style.PART_HeaderPanel = SolidPart(p.SurfaceAlt, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius);
        style.PART_TabViewItem = SolidPart(p.SurfaceAlt, p.Border, 1.0f, p.Radius * 0.6f);
        style.PART_SelectedTabViewItem = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.38f }, p.Accent, 1.0f, p.Radius * 0.6f);
        style.PART_TabText.ForegroundColor = p.Text;
        style.PART_ContentPresenter = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.64f });
        style.PART_FocusVisual = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.AccentAlt, 2.0f, p.Radius + 1.0f);
        return style;
    }

    inline ExpanderStyle MakeExpanderStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        ExpanderStyle style {};
        style.PART_Header = SolidPart(p.SurfaceAlt, p.Border, 1.0f, p.Radius);
        style.PART_ExpanderGlyph.ForegroundColor = p.Accent;
        style.PART_HeaderText.ForegroundColor = p.Text;
        style.PART_ContentPresenter = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.42f });
        style.PART_Border = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.Border, 1.0f, p.Radius);
        style.PART_FocusVisual = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.AccentAlt, 2.0f, p.Radius + 1.0f);
        return style;
    }

    inline ToggleSwitchStyle MakeToggleSwitchStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        ToggleSwitchStyle style {};
        style.PART_Track = SolidPart(Color{ p.Border.r, p.Border.g, p.Border.b, 0.82f }, Color{ p.Border.r, p.Border.g, p.Border.b, 0.0f }, 0.0f, 10.0f);
        style.PART_Thumb = SolidPart(Color{ 1.0f, 1.0f, 1.0f, 1.0f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, 8.0f);
        style.PART_HeaderText.ForegroundColor = p.Text;
        style.PART_Text.ForegroundColor = p.Text;
        style.PART_FocusVisual = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.AccentAlt, 2.0f, 12.0f);
        return style;
    }

    inline InfoBarStyle MakeInfoBarStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        InfoBarStyle style {};
        style.PART_Background = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.96f }, p.Border, 1.0f, p.Radius);
        style.PART_Border = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.Border, 1.0f, p.Radius);
        style.PART_Icon = SolidPart(p.Accent, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, 8.0f);
        style.PART_TitleText.ForegroundColor = p.Text;
        style.PART_MessageText.ForegroundColor = Color{ p.Text.r, p.Text.g, p.Text.b, 0.76f };
        style.PART_CloseButton = SolidPart(Color{ p.SurfaceAlt.r, p.SurfaceAlt.g, p.SurfaceAlt.b, 0.0f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius * 0.5f);
        style.PART_CloseGlyph.ForegroundColor = p.Text;
        return style;
    }

    inline SeparatorStyle MakeSeparatorStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        SeparatorStyle style {};
        style.PART_Line.BackgroundColor = p.Border;
        style.PART_Line.BorderThickness = 1.0f;
        return style;
    }

    inline CommandBarStyle MakeCommandBarStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        CommandBarStyle style {};
        style.PART_Background = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.96f }, p.Border, 1.0f, p.Radius);
        style.PART_ContentPresenter = SolidPart(Color{ p.SurfaceAlt.r, p.SurfaceAlt.g, p.SurfaceAlt.b, 0.18f });
        return style;
    }

    inline AppBarButtonStyle MakeAppBarButtonStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        AppBarButtonStyle style {};
        style.PART_Background = SolidPart(Color{ p.SurfaceAlt.r, p.SurfaceAlt.g, p.SurfaceAlt.b, 0.0f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius * 0.5f, p.Text);
        style.PART_Border = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, Color{ p.Border.r, p.Border.g, p.Border.b, 0.0f }, 0.0f, p.Radius * 0.5f);
        style.PART_Icon.ForegroundColor = p.Accent;
        style.PART_Text.ForegroundColor = p.Text;
        style.PART_FocusVisual = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.AccentAlt, 2.0f, p.Radius);
        return style;
    }

    inline InventorySlotStyle MakeInventorySlotStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        InventorySlotStyle style {};
        style.PART_Background = SolidPart(p.Surface, p.Border, 1.0f, p.Radius);
        style.PART_Border = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.Border, 1.0f, p.Radius);
        style.PART_QuantityText.ForegroundColor = p.Text;
        style.PART_CooldownOverlay = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.56f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius);
        style.PART_SelectionGlow = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, p.AccentAlt, 2.0f, p.Radius);
        style.PART_HoverGlow = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.12f }, p.Accent, 1.0f, p.Radius);
        style.PART_DragGhost = SolidPart(Color{ p.SurfaceAlt.r, p.SurfaceAlt.g, p.SurfaceAlt.b, 0.86f }, p.Accent, 2.0f, p.Radius);
        return style;
    }

    enum class Visibility : uint8_t { Visible, Collapsed };
    enum class HorizontalAlignment : uint8_t { Left, Center, Right, Stretch };
    enum class VerticalAlignment : uint8_t { Top, Center, Bottom, Stretch };
    enum class Orientation : uint8_t { Vertical, Horizontal };
    enum class TextWrappingMode : uint8_t { NoWrap, Wrap, WrapWholeWords };
    enum class ExpandDirection : uint8_t { Down, Up, Left, Right };
    enum class ScrollBarVisibility : uint8_t { Disabled, Auto, Visible, Hidden };
    enum class FlowDirection : uint8_t { LeftToRight, RightToLeft };
    enum class Cursor : uint8_t { Arrow, Hand, IBeam, Crosshair, SizeWE, SizeNS, SizeAll, NotAllowed };
    enum class Easing : uint8_t { Linear, EaseOut, EaseInOut, EaseOutCubic };

    inline float ApplyEasing(Easing easing, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        switch (easing)
        {
        case Easing::EaseOut:
            return 1.0f - (1.0f - t) * (1.0f - t);
        case Easing::EaseInOut:
            return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
        case Easing::EaseOutCubic:
            return EaseOutCubic(t);
        case Easing::Linear:
        default:
            return t;
        }
    }

    struct TweenFloat
    {
        float Value = 0.0f;
        float Target = 0.0f;
        float Speed = 12.0f;
        Easing Ease = Easing::EaseOut;

        TweenFloat() = default;
        explicit TweenFloat(float value) : Value(value), Target(value) {}

        void SetTarget(float target) { Target = target; }

        void Snap(float value)
        {
            Value = value;
            Target = value;
        }

        void Update(float deltaTime)
        {
            const float t = ApplyEasing(Ease, std::clamp(Speed * deltaTime, 0.0f, 1.0f));
            Value += (Target - Value) * t;
            if (std::fabs(Target - Value) < 0.0001f)
                Value = Target;
        }
    };

    inline Rect AlignRect(Vec2 desiredSize, Rect slot, HorizontalAlignment horizontal, VerticalAlignment vertical)
    {
        const float width = horizontal == FyGUI::HorizontalAlignment::Stretch ? slot.width : std::min(slot.width, desiredSize.x);
        const float height = vertical == FyGUI::VerticalAlignment::Stretch ? slot.height : std::min(slot.height, desiredSize.y);
        float x = slot.x;
        float y = slot.y;
        if (horizontal == FyGUI::HorizontalAlignment::Center)
            x += (slot.width - width) * 0.5f;
        else if (horizontal == FyGUI::HorizontalAlignment::Right)
            x += slot.width - width;
        if (vertical == FyGUI::VerticalAlignment::Center)
            y += (slot.height - height) * 0.5f;
        else if (vertical == FyGUI::VerticalAlignment::Bottom)
            y += slot.height - height;
        return { x, y, std::max(0.0f, width), std::max(0.0f, height) };
    }

    enum class EventKind : uint16_t
    {
        None,
        Initialized, Loaded, Unloaded,
        AttachedToVisualTree, DetachedFromVisualTree,
        AttachedToLogicalTree, DetachedFromLogicalTree,
        LayoutUpdated, SizeChanged, EffectiveViewportChanged,
        RequestBringIntoView, PropertyChanged, DataContextChanged,
        ResourcesChanged, ThemeChanged,
        GotFocus, LostFocus, FocusEngaged, FocusDisengaged, FocusChanged,
        PointerPressed, PointerReleased, PointerMoved, PointerEntered,
        PointerExited, PointerWheelChanged, PointerCaptureChanged,
        PointerCaptureLost,
        KeyDown, KeyUp, TextInput,
        TextCompositionStarted, TextCompositionUpdated, TextCompositionCompleted,
        Tapped, DoubleTapped, Holding, ContextRequested,
        DragStarted, DragEnter, DragLeave, DragOver, Drop, DragCompleted,
        ManipulationStarted, ManipulationDelta, ManipulationInertiaStarted,
        ManipulationCompleted,
        Click, CheckedChanged, SelectionChanged, TextChanged, ValueChanged,
        ScrollChanged, Opened, Closed, Expanded, Collapsed,
        NavigationSubmit, NavigationCancel, NavigationBack, NavigationMove,
        NavigationFocusChanged
    };

    enum class RoutingStrategy : uint8_t { Direct, Bubble, Tunnel };
    enum class EventPhase : uint8_t { Tunnel, Target, Bubble };
    enum class PointerType : uint8_t { Mouse, Touch, Pen, GamepadCursor };
    enum class PointerButton : uint8_t { None, Left, Right, Middle, X1, X2, TouchContact, PenTip, PenBarrel, GamepadPrimary };
    enum class PointerButtons : uint32_t
    {
        None = 0,
        Left = 1 << 0,
        Right = 1 << 1,
        Middle = 1 << 2,
        X1 = 1 << 3,
        X2 = 1 << 4,
        TouchContact = 1 << 5,
        PenTip = 1 << 6,
        PenBarrel = 1 << 7,
        GamepadPrimary = 1 << 8
    };

    inline PointerButtons operator|(PointerButtons a, PointerButtons b)
    {
        return static_cast<PointerButtons>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline bool HasButton(PointerButtons buttons, PointerButtons button)
    {
        return (static_cast<uint32_t>(buttons) & static_cast<uint32_t>(button)) != 0;
    }

    enum class KeyModifiers : uint32_t
    {
        None = 0,
        Shift = 1 << 0,
        Control = 1 << 1,
        Alt = 1 << 2,
        Super = 1 << 3
    };

    enum class Key : uint16_t
    {
        None = 0,
        Tab,
        Enter,
        Escape,
        Space,
        Backspace,
        DeleteKey,
        Left,
        Right,
        Up,
        Down,
        Home,
        End,
        PageUp,
        PageDown,
        Shift,
        Control,
        Alt,
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Count
    };

    inline constexpr size_t KeyCount = static_cast<size_t>(Key::Count);

    enum class InputDeviceKind : uint8_t
    {
        None,
        MouseKeyboard,
        Gamepad,
        Touch
    };

    struct InputSnapshot
    {
        Vec2 PointerPosition = {};
        Vec2 PointerDelta = {};
        float WheelDelta = 0.0f;
        bool MouseDown[3] = {};
        bool MousePressed[3] = {};
        bool MouseReleased[3] = {};
        bool KeyDown[KeyCount] = {};
        bool KeyPressed[KeyCount] = {};
        bool KeyReleased[KeyCount] = {};
        std::vector<uint32_t> TextInputCodepoints;
        bool ShiftDown = false;
        bool ControlDown = false;
        bool AltDown = false;
        InputDeviceKind LastInputDevice = InputDeviceKind::None;
        bool AcceptPressed = false;
        bool CancelPressed = false;
        bool BackPressed = false;
        bool MenuPressed = false;
        bool SecondaryPressed = false;
        bool DetailsPressed = false;
        bool NavigateUpPressed = false;
        bool NavigateDownPressed = false;
        bool NavigateLeftPressed = false;
        bool NavigateRightPressed = false;
        bool FocusNextPressed = false;
        bool FocusPreviousPressed = false;
        bool PageLeftPressed = false;
        bool PageRightPressed = false;
        bool GamepadConnected = false;
        bool GamepadAcceptPressed = false;
        bool GamepadCancelPressed = false;
        bool GamepadSecondaryPressed = false;
        bool GamepadDetailsPressed = false;
        bool GamepadBackPressed = false;
        bool GamepadMenuPressed = false;
        bool GamepadNavigateUpPressed = false;
        bool GamepadNavigateDownPressed = false;
        bool GamepadNavigateLeftPressed = false;
        bool GamepadNavigateRightPressed = false;
        bool GamepadFocusNextPressed = false;
        bool GamepadFocusPreviousPressed = false;
        bool GamepadPageLeftPressed = false;
        bool GamepadPageRightPressed = false;
        float GamepadLeftX = 0.0f;
        float GamepadLeftY = 0.0f;
        float GamepadRightX = 0.0f;
        float GamepadRightY = 0.0f;
    };

    inline bool InputKeyPressed(const InputSnapshot& input, Key key)
    {
        const size_t index = static_cast<size_t>(key);
        return index < KeyCount && input.KeyPressed[index];
    }

    inline void PopulateGenericActions(InputSnapshot& input)
    {
        input.AcceptPressed = input.AcceptPressed || InputKeyPressed(input, Key::Enter) || InputKeyPressed(input, Key::Space) || input.GamepadAcceptPressed;
        input.CancelPressed = input.CancelPressed || InputKeyPressed(input, Key::Escape) || input.GamepadCancelPressed;
        input.BackPressed = input.BackPressed || input.GamepadBackPressed;
        input.MenuPressed = input.MenuPressed || input.GamepadMenuPressed;
        input.SecondaryPressed = input.SecondaryPressed || input.GamepadSecondaryPressed;
        input.DetailsPressed = input.DetailsPressed || input.GamepadDetailsPressed;
        input.NavigateUpPressed = input.NavigateUpPressed || InputKeyPressed(input, Key::Up) || input.GamepadNavigateUpPressed;
        input.NavigateDownPressed = input.NavigateDownPressed || InputKeyPressed(input, Key::Down) || input.GamepadNavigateDownPressed;
        input.NavigateLeftPressed = input.NavigateLeftPressed || InputKeyPressed(input, Key::Left) || input.GamepadNavigateLeftPressed;
        input.NavigateRightPressed = input.NavigateRightPressed || InputKeyPressed(input, Key::Right) || input.GamepadNavigateRightPressed;
        input.FocusNextPressed = input.FocusNextPressed || (InputKeyPressed(input, Key::Tab) && !input.ShiftDown) || input.GamepadFocusNextPressed;
        input.FocusPreviousPressed = input.FocusPreviousPressed || (InputKeyPressed(input, Key::Tab) && input.ShiftDown) || input.GamepadFocusPreviousPressed;
        input.PageLeftPressed = input.PageLeftPressed || InputKeyPressed(input, Key::PageUp) || input.GamepadPageLeftPressed;
        input.PageRightPressed = input.PageRightPressed || InputKeyPressed(input, Key::PageDown) || input.GamepadPageRightPressed;

        const bool gamepadActivity = input.GamepadAcceptPressed || input.GamepadCancelPressed || input.GamepadSecondaryPressed || input.GamepadDetailsPressed ||
            input.GamepadBackPressed || input.GamepadMenuPressed || input.GamepadNavigateUpPressed || input.GamepadNavigateDownPressed ||
            input.GamepadNavigateLeftPressed || input.GamepadNavigateRightPressed || input.GamepadFocusNextPressed || input.GamepadFocusPreviousPressed ||
            input.GamepadPageLeftPressed || input.GamepadPageRightPressed || std::fabs(input.GamepadLeftX) > 0.20f || std::fabs(input.GamepadLeftY) > 0.20f ||
            std::fabs(input.GamepadRightX) > 0.20f || std::fabs(input.GamepadRightY) > 0.20f;

        bool mouseKeyboardActivity = input.PointerDelta.x != 0.0f || input.PointerDelta.y != 0.0f || input.WheelDelta != 0.0f || !input.TextInputCodepoints.empty();
        for (int i = 0; i < 3; ++i)
            mouseKeyboardActivity = mouseKeyboardActivity || input.MousePressed[i] || input.MouseReleased[i];
        for (size_t i = 0; i < KeyCount; ++i)
            mouseKeyboardActivity = mouseKeyboardActivity || input.KeyPressed[i] || input.KeyReleased[i];

        if (gamepadActivity)
            input.LastInputDevice = InputDeviceKind::Gamepad;
        else if (mouseKeyboardActivity)
            input.LastInputDevice = InputDeviceKind::MouseKeyboard;
    }

    inline bool HasInputActivity(const InputSnapshot& input)
    {
        if (input.PointerDelta.x != 0.0f || input.PointerDelta.y != 0.0f || input.WheelDelta != 0.0f || !input.TextInputCodepoints.empty())
            return true;
        for (int i = 0; i < 3; ++i)
            if (input.MousePressed[i] || input.MouseReleased[i])
                return true;
        for (size_t i = 0; i < KeyCount; ++i)
            if (input.KeyPressed[i] || input.KeyReleased[i])
                return true;
        return input.AcceptPressed || input.CancelPressed || input.BackPressed || input.MenuPressed || input.SecondaryPressed || input.DetailsPressed ||
            input.NavigateUpPressed || input.NavigateDownPressed || input.NavigateLeftPressed || input.NavigateRightPressed ||
            input.FocusNextPressed || input.FocusPreviousPressed || input.PageLeftPressed || input.PageRightPressed ||
            input.GamepadAcceptPressed || input.GamepadCancelPressed || input.GamepadSecondaryPressed || input.GamepadDetailsPressed ||
            input.GamepadBackPressed || input.GamepadMenuPressed || input.GamepadNavigateUpPressed || input.GamepadNavigateDownPressed ||
            input.GamepadNavigateLeftPressed || input.GamepadNavigateRightPressed || input.GamepadFocusNextPressed ||
            input.GamepadFocusPreviousPressed || input.GamepadPageLeftPressed || input.GamepadPageRightPressed;
    }

    enum class UIAction : uint16_t
    {
        None,
        Accept,
        Cancel,
        Secondary,
        Details,
        Back,
        Menu,
        NavigateUp,
        NavigateDown,
        NavigateLeft,
        NavigateRight,
        FocusNext,
        FocusPrevious,
        PageUp,
        PageDown,
        PageLeft,
        PageRight,
        Home,
        End
    };

    using PointerId = uint32_t;
    class UIElement;

    struct EventArgs
    {
        EventKind kind = EventKind::None;
        UIElement* source = nullptr;
        UIElement* originalSource = nullptr;
        RoutingStrategy route = RoutingStrategy::Bubble;
        EventPhase phase = EventPhase::Target;
        bool handled = false;

        virtual ~EventArgs() = default;
    };

    struct PointerEventArgs final : EventArgs
    {
        PointerId pointerId = 0;
        PointerType pointerType = PointerType::Mouse;
        Vec2 position = {};
        Vec2 screenPosition = {};
        Vec2 delta = {};
        Vec2 wheelDelta = {};
        PointerButton changedButton = PointerButton::None;
        PointerButtons buttons = PointerButtons::None;
        KeyModifiers modifiers = KeyModifiers::None;
        bool isPrimary = true;
        bool isInContact = false;
        bool isSynthesized = false;
    };

    struct KeyEventArgs final : EventArgs
    {
        Key key = Key::None;
        KeyModifiers modifiers = KeyModifiers::None;
        bool repeat = false;
    };

    struct TextInputEventArgs final : EventArgs
    {
        uint32_t codepoint = 0;
    };

    struct ClickEventArgs final : EventArgs
    {
        PointerType pointerType = PointerType::Mouse;
        KeyModifiers modifiers = KeyModifiers::None;
        bool fromKeyboard = false;
        bool fromGamepad = false;
        bool fromPointer = true;
    };

    enum class CheckedState : uint8_t
    {
        Unchecked,
        Checked,
        Indeterminate
    };

    struct CheckedChangedEventArgs final : EventArgs
    {
        CheckedState oldState = CheckedState::Unchecked;
        CheckedState newState = CheckedState::Unchecked;
    };

    struct SelectionChangedEventArgs final : EventArgs
    {
        int32_t oldIndex = -1;
        int32_t newIndex = -1;
        void* oldItem = nullptr;
        void* newItem = nullptr;
    };

    struct TextChangedEventArgs final : EventArgs
    {
        std::string oldText;
        std::string newText;
    };

    struct ValueChangedEventArgs final : EventArgs
    {
        double oldValue = 0.0;
        double newValue = 0.0;
    };

    struct ScrollChangedEventArgs final : EventArgs
    {
        Vec2 oldOffset = {};
        Vec2 newOffset = {};
        Vec2 extent = {};
        Vec2 viewport = {};
    };

    struct DragDropEventArgs final : EventArgs
    {
        UIElement* dragSource = nullptr;
        UIElement* dropTarget = nullptr;
        Vec2 position = {};
        std::string payload;
    };

    using EventHandler = std::function<void(UIElement&, EventArgs&)>;

    struct HandlerEntry
    {
        EventKind kind = EventKind::None;
        RoutingStrategy route = RoutingStrategy::Bubble;
        bool handledEventsToo = false;
        EventHandler handler;
    };

    inline float ClampDimension(float value, float minValue, float maxValue)
    {
        if (maxValue < minValue)
            return minValue;
        return std::clamp(value, minValue, maxValue);
    }

    inline float RoundIf(float value, bool enabled)
    {
        return enabled ? std::floor(value + 0.5f) : value;
    }

    inline Vec2 RoundIf(Vec2 value, bool enabled)
    {
        return { RoundIf(value.x, enabled), RoundIf(value.y, enabled) };
    }

    inline Rect RoundIf(Rect value, bool enabled)
    {
        return {
            RoundIf(value.x, enabled),
            RoundIf(value.y, enabled),
            RoundIf(value.width, enabled),
            RoundIf(value.height, enabled)
        };
    }

    inline bool NearlyEqual(float a, float b, float epsilon = 0.001f)
    {
        if (std::isinf(a) || std::isinf(b))
            return a == b;
        return std::fabs(a - b) <= epsilon;
    }

    inline bool SameSize(Vec2 a, Vec2 b)
    {
        return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y);
    }

    inline bool SameRect(Rect a, Rect b)
    {
        return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y) &&
            NearlyEqual(a.width, b.width) && NearlyEqual(a.height, b.height);
    }

    inline ImFont*& DefaultFontStorage()
    {
        static ImFont* font = nullptr;
        return font;
    }

    inline void SetDefaultFont(ImFont* font) { DefaultFontStorage() = font; }
    inline ImFont* GetDefaultFont()
    {
        if (DefaultFontStorage())
            return DefaultFontStorage();
        return ImGui::GetCurrentContext() ? ImGui::GetFont() : nullptr;
    }

    inline float GetFontSize()
    {
        return ImGui::GetCurrentContext() ? ImGui::GetFontSize() : 16.0f;
    }

    inline Vec2 MeasureText(std::string_view text, float wrapWidth = 0.0f)
    {
        ImFont* font = GetDefaultFont();
        if (!font || text.empty())
            return {};
        const float size = GetFontSize();
        const ImVec2 measured = font->CalcTextSizeA(size, FLT_MAX, wrapWidth, text.data(), text.data() + text.size());
        return { measured.x, measured.y };
    }

    inline Vec2 MeasureText(std::string_view text, float wrapWidth, float fontSize)
    {
        ImFont* font = GetDefaultFont();
        if (!font || text.empty())
            return {};
        const ImVec2 measured = font->CalcTextSizeA(fontSize > 0.0f ? fontSize : GetFontSize(), FLT_MAX, wrapWidth, text.data(), text.data() + text.size());
        return { measured.x, measured.y };
    }

    inline void DrawTextInRect(ImDrawList& drawList, Rect rect, std::string_view text, Color color, float opacity = 1.0f, HorizontalAlignment horizontal = FyGUI::HorizontalAlignment::Left, VerticalAlignment vertical = FyGUI::VerticalAlignment::Center, float wrapWidth = 0.0f, float fontSize = 0.0f)
    {
        if (text.empty() || rect.width <= 0.0f || rect.height <= 0.0f)
            return;
        fontSize = fontSize > 0.0f ? fontSize : GetFontSize();
        const float effectiveWrap = wrapWidth > 0.0f ? wrapWidth : (horizontal == FyGUI::HorizontalAlignment::Stretch ? rect.width : 0.0f);
        const Vec2 measured = MeasureText(text, effectiveWrap, fontSize);
        float x = rect.x;
        float y = rect.y;
        if (horizontal == FyGUI::HorizontalAlignment::Center)
            x += (rect.width - measured.x) * 0.5f;
        else if (horizontal == FyGUI::HorizontalAlignment::Right)
            x += rect.width - measured.x;
        if (vertical == FyGUI::VerticalAlignment::Center)
            y += (rect.height - measured.y) * 0.5f;
        else if (vertical == FyGUI::VerticalAlignment::Bottom)
            y += rect.height - measured.y;
        drawList.PushClipRect(Vec2(rect.Left(), rect.Top()), Vec2(rect.Right(), rect.Bottom()), true);
        drawList.AddText(GetDefaultFont(), fontSize, Vec2(std::floor(x), std::floor(y)), color.ToU32(opacity), text.data(), text.data() + text.size(), effectiveWrap);
        drawList.PopClipRect();
    }

    inline void DrawImageInRect(ImDrawList& drawList, const ImageSource& source, Rect rect, Color tint = Color{ 1.0f, 1.0f, 1.0f, 1.0f }, float opacity = 1.0f)
    {
        if (!source.texture || rect.width <= 0.0f || rect.height <= 0.0f)
            return;
        drawList.AddImage(source.texture, Vec2(rect.Left(), rect.Top()), Vec2(rect.Right(), rect.Bottom()), source.uv0, source.uv1, tint.ToU32(opacity));
    }

    inline void DrawFocusVisual(ImDrawList& drawList, Rect rect, float radius, float opacity = 1.0f, Color primary = Color{ 0.0f, 120.0f / 255.0f, 212.0f / 255.0f, 1.0f }, Color secondary = Color{ 1.0f, 1.0f, 1.0f, 135.0f / 255.0f })
    {
        if (!GetFocusVisualsEnabled() || rect.width <= 0.0f || rect.height <= 0.0f)
            return;
        drawList.AddRect(Vec2(rect.Left() + 1.0f, rect.Top() + 1.0f), Vec2(rect.Right() - 1.0f, rect.Bottom() - 1.0f), primary.ToU32(0.80f * opacity), std::max(1.0f, radius), ImDrawFlags_None, 1.5f);
        drawList.AddRect(Vec2(rect.Left() + 3.0f, rect.Top() + 3.0f), Vec2(rect.Right() - 3.0f, rect.Bottom() - 3.0f), secondary.ToU32(0.70f * opacity), std::max(1.0f, radius - 1.0f), ImDrawFlags_None, 1.0f);
    }

    inline float& TimeSecondsStorage()
    {
        static float seconds = 0.0f;
        return seconds;
    }

    inline float GetTimeSeconds() { return TimeSecondsStorage(); }
    inline Color ColorFromBytes(int r, int g, int b, int a = 255)
    {
        return { static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, static_cast<float>(a) / 255.0f };
    }

    class UIElement : public std::enable_shared_from_this<UIElement>
    {
    public:
        virtual ~UIElement() = default;

        void Measure(Vec2 availableSize)
        {
            if (Visibility == FyGUI::Visibility::Collapsed)
            {
                DesiredSize = {};
                m_measureValid = true;
                m_arrangeValid = false;
                m_lastMeasureAvailable = availableSize;
                return;
            }

            if (m_measureValid && SameSize(m_lastMeasureAvailable, availableSize))
                return;

            Vec2 innerAvailable = {
                std::max(0.0f, availableSize.x - Margin.Horizontal()),
                std::max(0.0f, availableSize.y - Margin.Vertical())
            };

            if (Width >= 0.0f)
                innerAvailable.x = Width;
            if (Height >= 0.0f)
                innerAvailable.y = Height;

            Vec2 measured = MeasureOverride(innerAvailable);
            measured.x = Width >= 0.0f ? Width : measured.x;
            measured.y = Height >= 0.0f ? Height : measured.y;
            measured.x = ClampDimension(measured.x, MinWidth, MaxWidth);
            measured.y = ClampDimension(measured.y, MinHeight, MaxHeight);

            Vec2 desired = RoundIf(Vec2{ measured.x + Margin.Horizontal(), measured.y + Margin.Vertical() }, UseLayoutRounding);
            if (!SameSize(DesiredSize, desired))
                m_arrangeValid = false;
            DesiredSize = desired;
            m_lastMeasureAvailable = availableSize;
            m_measureValid = true;
        }

        void Arrange(Rect finalRect)
        {
            if (Visibility == FyGUI::Visibility::Collapsed)
            {
                Bounds = {};
                m_arrangeValid = true;
                m_lastArrangeRect = finalRect;
                return;
            }

            if (m_arrangeValid && SameRect(m_lastArrangeRect, finalRect))
                return;

            Rect slot = {
                finalRect.x + Margin.left,
                finalRect.y + Margin.top,
                std::max(0.0f, finalRect.width - Margin.Horizontal()),
                std::max(0.0f, finalRect.height - Margin.Vertical())
            };

            Vec2 desired = {
                std::max(0.0f, DesiredSize.x - Margin.Horizontal()),
                std::max(0.0f, DesiredSize.y - Margin.Vertical())
            };

            float width = HorizontalAlignment == FyGUI::HorizontalAlignment::Stretch ? slot.width : std::min(slot.width, desired.x);
            float height = VerticalAlignment == FyGUI::VerticalAlignment::Stretch ? slot.height : std::min(slot.height, desired.y);

            if (Width >= 0.0f)
                width = std::min(slot.width, Width);
            if (Height >= 0.0f)
                height = std::min(slot.height, Height);

            float x = slot.x;
            float y = slot.y;
            if (HorizontalAlignment == FyGUI::HorizontalAlignment::Center)
                x += (slot.width - width) * 0.5f;
            else if (HorizontalAlignment == FyGUI::HorizontalAlignment::Right)
                x += slot.width - width;

            if (VerticalAlignment == FyGUI::VerticalAlignment::Center)
                y += (slot.height - height) * 0.5f;
            else if (VerticalAlignment == FyGUI::VerticalAlignment::Bottom)
                y += slot.height - height;

            Bounds = RoundIf({ x, y, std::max(0.0f, width), std::max(0.0f, height) }, UseLayoutRounding);
            ArrangeOverride(Bounds);
            m_lastArrangeRect = finalRect;
            m_arrangeValid = true;
        }

        virtual void Update(float deltaTime)
        {
            UpdateVisualTweens(deltaTime);
        }

        void Render(ImDrawList& drawList)
        {
            if (Visibility != FyGUI::Visibility::Visible)
                return;

            const bool clip = ClipToBounds;
            if (clip)
                drawList.PushClipRect(Vec2(Bounds.Left(), Bounds.Top()), Vec2(Bounds.Right(), Bounds.Bottom()), true);

            RenderOverride(drawList);

            if (m_isFocused && UseSystemFocusVisuals && GetFocusVisualsEnabled())
                DrawFocusVisual(drawList, Bounds, CornerRadius, Opacity);

            if (clip)
                drawList.PopClipRect();
        }

        virtual void RenderOverlay(ImDrawList&) {}

        virtual UIElement* HitTestOverlay(Vec2)
        {
            return nullptr;
        }

        virtual UIElement* HitTest(Vec2 point)
        {
            if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible)
                return nullptr;
            return Bounds.Contains(point) ? this : nullptr;
        }

        virtual bool OnAction(UIAction) { return false; }
        virtual void VisitChildren(const std::function<void(UIElement*)>&) {}

        void SetVisualTweenTargets(bool hover, bool pressed, bool focused, bool selected = false, bool open = false)
        {
            HoverTween.SetTarget(hover && IsEnabled ? 1.0f : 0.0f);
            PressedTween.SetTarget(pressed && IsEnabled ? 1.0f : 0.0f);
            FocusTween.SetTarget(focused && IsEnabled ? 1.0f : 0.0f);
            SelectedTween.SetTarget(selected && IsEnabled ? 1.0f : 0.0f);
            OpenTween.SetTarget(open && IsEnabled ? 1.0f : 0.0f);
        }

        void UpdateVisualTweens(float deltaTime)
        {
            HoverTween.Update(deltaTime);
            PressedTween.Update(deltaTime);
            FocusTween.Update(deltaTime);
            SelectedTween.Update(deltaTime);
            OpenTween.Update(deltaTime);
            OpacityTween.Update(deltaTime);
            ScaleTween.Update(deltaTime);
        }

        float HoverTransition() const { return HoverTween.Value; }
        float PressedTransition() const { return PressedTween.Value; }
        float FocusTransition() const { return FocusTween.Value; }
        float SelectedTransition() const { return SelectedTween.Value; }
        float OpenTransition() const { return OpenTween.Value; }
        float EffectiveOpacity() const { return Opacity * OpacityTween.Value; }

        Rect AnimatedVisualBounds(float hoverGrow = 1.5f, float pressedShrink = 1.0f) const
        {
            const float grow = HoverTween.Value * hoverGrow - PressedTween.Value * pressedShrink;
            Rect animated {
                Bounds.x - grow,
                Bounds.y - grow,
                std::max(0.0f, Bounds.width + grow * 2.0f),
                std::max(0.0f, Bounds.height + grow * 2.0f)
            };
            return ScaleRectFromCenter(animated, std::max(0.0f, ScaleTween.Value));
        }

        UIElement* FindName(std::string_view name)
        {
            if (!name.empty() && Name == name)
                return this;

            UIElement* found = nullptr;
            VisitChildren([&](UIElement* child)
            {
                if (!found && child)
                    found = child->FindName(name);
            });
            return found;
        }

        template<class T>
        T* FindName(std::string_view name)
        {
            return dynamic_cast<T*>(FindName(name));
        }

        void AddHandler(EventKind kind, EventHandler handler, RoutingStrategy route = RoutingStrategy::Bubble, bool handledEventsToo = false)
        {
            m_handlers.push_back({ kind, route, handledEventsToo, std::move(handler) });
        }

        void ClearHandlers(EventKind kind)
        {
            m_handlers.erase(std::remove_if(m_handlers.begin(), m_handlers.end(),
                [kind](const HandlerEntry& entry) { return entry.kind == kind; }), m_handlers.end());
        }

        void RaiseEvent(EventArgs& args)
        {
            if (args.kind == EventKind::None)
                return;

            if (!args.originalSource)
                args.originalSource = this;

            std::vector<UIElement*> route;
            for (UIElement* current = args.originalSource; current; current = current->m_parent)
                route.push_back(current);

            if (route.empty())
                route.push_back(this);

            if (args.route == RoutingStrategy::Direct)
            {
                UIElement* target = args.originalSource;
                args.phase = EventPhase::Target;
                target->InvokeHandlers(args, RoutingStrategy::Direct);
                return;
            }

            if (args.route == RoutingStrategy::Tunnel)
            {
                args.phase = EventPhase::Tunnel;
                for (auto it = route.rbegin(); it != route.rend(); ++it)
                    (*it)->InvokeHandlers(args, RoutingStrategy::Tunnel);
            }

            args.phase = EventPhase::Target;
            route.front()->InvokeHandlers(args, RoutingStrategy::Bubble);

            if (args.route == RoutingStrategy::Bubble)
            {
                args.phase = EventPhase::Bubble;
                for (size_t i = 1; i < route.size(); ++i)
                    route[i]->InvokeHandlers(args, RoutingStrategy::Bubble);
            }
        }

        void CapturePointer(PointerId pointerId) { m_capturedPointers[pointerId] = true; }
        void ReleasePointer(PointerId pointerId) { m_capturedPointers.erase(pointerId); }
        bool IsPointerCaptured(PointerId pointerId) const { return m_capturedPointers.find(pointerId) != m_capturedPointers.end(); }

        void SetIsTabStop(bool value) { IsTabStop = value; }
        bool GetIsTabStop() const { return IsTabStop; }
        void SetFocused(bool focused) { m_isFocused = focused; }
        bool IsFocused() const { return m_isFocused; }

        UIElement* GetParent() const { return m_parent; }
        void SetName(std::string_view name) { Name.assign(name.data(), name.size()); }

        void InvalidateMeasure()
        {
            m_measureValid = false;
            m_arrangeValid = false;
            if (m_parent)
                m_parent->InvalidateMeasure();
        }

        void InvalidateArrange()
        {
            m_arrangeValid = false;
            if (m_parent)
                m_parent->InvalidateArrange();
        }

        bool IsMeasureValidFor(Vec2 availableSize) const
        {
            return m_measureValid && SameSize(m_lastMeasureAvailable, availableSize);
        }

        bool IsArrangeValidFor(Rect finalRect) const
        {
            return m_arrangeValid && SameRect(m_lastArrangeRect, finalRect);
        }

        virtual VisualState GetVisualState() const
        {
            if (!IsEnabled)
                return VisualState::Disabled;
            if (IsFocused())
                return VisualState::Focused;
            return VisualState::Normal;
        }

        Rect Bounds = {};
        Vec2 DesiredSize = {};
        float Width = AutoSize;
        float Height = AutoSize;
        float MinWidth = 0.0f;
        float MinHeight = 0.0f;
        float MaxWidth = InfiniteSize;
        float MaxHeight = InfiniteSize;
        Thickness Margin = {};
        Thickness Padding = {};
        FyGUI::HorizontalAlignment HorizontalAlignment = FyGUI::HorizontalAlignment::Stretch;
        FyGUI::VerticalAlignment VerticalAlignment = FyGUI::VerticalAlignment::Stretch;
        FyGUI::Visibility Visibility = FyGUI::Visibility::Visible;
        bool IsEnabled = true;
        bool IsHitTestVisible = true;
        bool IsTabStop = false;
        bool UseSystemFocusVisuals = true;
        bool IsDragSource = false;
        bool AllowDrop = false;
        TweenFloat HoverTween;
        TweenFloat PressedTween;
        TweenFloat FocusTween;
        TweenFloat SelectedTween;
        TweenFloat OpenTween;
        TweenFloat OpacityTween { 1.0f };
        TweenFloat ScaleTween { 1.0f };
        Color Background = { 0.0f, 0.0f, 0.0f, 0.0f };
        Color Foreground = ColorFromBytes(32, 31, 30, 255);
        Color BorderBrush = { 0.0f, 0.0f, 0.0f, 0.0f };
        Thickness BorderThickness = {};
        float Opacity = 1.0f;
        float CornerRadius = 0.0f;
        std::string Name;
        void* Tag = nullptr;
        void* DataContext = nullptr;
        std::string DragPayload;
        std::string ToolTip;
        std::string ToolTipTitle;
        Color ToolTipAccent = { 0.42f, 0.88f, 1.0f, 1.0f };
        bool UseLayoutRounding = true;
        bool ClipToBounds = false;
        FyGUI::FlowDirection FlowDirection = FyGUI::FlowDirection::LeftToRight;
        Cursor ProtectedCursor = Cursor::Arrow;

    protected:
        virtual Vec2 MeasureOverride(Vec2 availableSize)
        {
            return {
                Width >= 0.0f ? Width : std::min(availableSize.x, Padding.Horizontal()),
                Height >= 0.0f ? Height : std::min(availableSize.y, Padding.Vertical())
            };
        }

        virtual void ArrangeOverride(Rect) {}

        virtual void RenderOverride(ImDrawList& drawList)
        {
            DrawBackgroundAndBorder(drawList);
        }

        virtual void OnEvent(EventArgs&) {}

        void AdoptChild(const std::shared_ptr<UIElement>& child)
        {
            if (!child)
                return;
            child->m_parent = this;
            child->InvalidateMeasure();
        }

        void DetachChild(UIElement* child)
        {
            if (child && child->m_parent == this)
            {
                child->m_parent = nullptr;
                child->InvalidateMeasure();
            }
        }

        void DrawBackgroundAndBorder(ImDrawList& drawList)
        {
            if (Background.a > 0.0f)
                drawList.AddRectFilled(Vec2(Bounds.Left(), Bounds.Top()), Vec2(Bounds.Right(), Bounds.Bottom()), Background.ToU32(Opacity), CornerRadius);

            const float left = BorderThickness.left;
            if (left > 0.0f && BorderBrush.a > 0.0f)
                drawList.AddRect(Vec2(Bounds.Left(), Bounds.Top()), Vec2(Bounds.Right(), Bounds.Bottom()), BorderBrush.ToU32(Opacity), CornerRadius, ImDrawFlags_None, left);
        }

    private:
        void InvokeHandlers(EventArgs& args, RoutingStrategy route)
        {
            args.source = this;
            OnEvent(args);

            for (auto& entry : m_handlers)
            {
                if (entry.kind != args.kind || entry.route != route || !entry.handler)
                    continue;
                if (args.handled && !entry.handledEventsToo)
                    continue;
                entry.handler(*this, args);
            }
        }

        UIElement* m_parent = nullptr;
        bool m_isFocused = false;
        bool m_measureValid = false;
        bool m_arrangeValid = false;
        Vec2 m_lastMeasureAvailable = {};
        Rect m_lastArrangeRect = {};
        std::vector<HandlerEntry> m_handlers;
        std::unordered_map<PointerId, bool> m_capturedPointers;

        friend class Panel;
        friend class Border;
        friend class ControlExample;
        friend class MenuItem;
        friend class TabView;
        friend class Context;
    };

    inline VisualState ResolveVisualState(const UIElement& element)
    {
        return element.GetVisualState();
    }

    class Control : public UIElement
    {
    public:
        Color BackgroundNormal = { 0.15f, 0.16f, 0.20f, 1.0f };
        Color BackgroundHover = { 0.22f, 0.24f, 0.30f, 1.0f };
        Color BackgroundPressed = { 0.10f, 0.12f, 0.16f, 1.0f };
        bool IsHovered = false;
        bool IsPressed = false;
        float VisualTransitionSpeed = 12.0f;

        VisualState GetVisualState() const override
        {
            if (!IsEnabled)
                return VisualState::Disabled;
            if (IsPressed)
                return VisualState::Pressed;
            if (IsHovered)
                return VisualState::PointerOver;
            if (IsFocused())
                return VisualState::Focused;
            return VisualState::Normal;
        }

        void Update(float deltaTime) override
        {
            HoverTween.Speed = VisualTransitionSpeed;
            PressedTween.Speed = VisualTransitionSpeed;
            FocusTween.Speed = VisualTransitionSpeed;
            SetVisualTweenTargets(IsHovered, IsPressed, IsFocused());
            UpdateVisualTweens(deltaTime);
        }

    protected:
        ControlPartStyle ApplyStateTransition(ControlPartStyle style) const
        {
            if (style.BackgroundColor.a > 0.0f)
            {
                style.BackgroundColor = LerpColor(style.BackgroundColor, BrightenColor(style.BackgroundColor, 0.12f), HoverTransition());
                style.BackgroundColor = LerpColor(style.BackgroundColor, BrightenColor(style.BackgroundColor, -0.10f), PressedTransition());
            }
            if (style.BorderColor.a > 0.0f)
                style.BorderColor = LerpColor(style.BorderColor, BrightenColor(style.BorderColor, 0.18f), std::max(HoverTransition(), FocusTransition()));
            return style;
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            const Color previousBackground = Background;
            Background = LerpColor(BackgroundNormal, BackgroundHover, HoverTransition());
            Background = LerpColor(Background, BackgroundPressed, PressedTransition());
            DrawBackgroundAndBorder(drawList);
            Background = previousBackground;
        }
    };

    class ContentControl : public Control
    {
    public:
        std::shared_ptr<UIElement> ContentElement;
        FyGUI::HorizontalAlignment HorizontalContentAlignment = FyGUI::HorizontalAlignment::Stretch;
        FyGUI::VerticalAlignment VerticalContentAlignment = FyGUI::VerticalAlignment::Stretch;

        void SetContent(std::shared_ptr<UIElement> content)
        {
            if (ContentElement)
                DetachChild(ContentElement.get());
            ContentElement = std::move(content);
            if (ContentElement)
                AdoptChild(ContentElement);
            InvalidateMeasure();
        }

        void VisitChildren(const std::function<void(UIElement*)>& visitor) override
        {
            if (ContentElement)
                visitor(ContentElement.get());
        }

    protected:
        Vec2 MeasureContent(Vec2 available)
        {
            if (!ContentElement)
                return {};
            ContentElement->Measure(available);
            return ContentElement->DesiredSize;
        }

        void ArrangeContent(Rect finalRect)
        {
            if (!ContentElement)
                return;
            const Rect slot = DeflateRect(finalRect, Padding);
            ContentElement->Arrange(AlignRect(ContentElement->DesiredSize, slot, HorizontalContentAlignment, VerticalContentAlignment));
        }

        void Update(float deltaTime) override
        {
            Control::Update(deltaTime);
            if (ContentElement)
                ContentElement->Update(deltaTime);
        }

        UIElement* HitTest(Vec2 point) override
        {
            if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible)
                return nullptr;
            if (ContentElement)
            {
                if (UIElement* hit = ContentElement->HitTest(point))
                    return hit;
            }
            return Bounds.Contains(point) ? this : nullptr;
        }

        UIElement* HitTestOverlay(Vec2 point) override
        {
            if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible)
                return nullptr;
            return ContentElement ? ContentElement->HitTestOverlay(point) : nullptr;
        }
    };

    class HeaderedContentControl : public ContentControl
    {
    public:
        std::string Header;
    };

    class RangeBase : public Control
    {
    public:
        double Minimum = 0.0;
        double Maximum = 1.0;
        double Value = 0.0;
        double AnimatedValue = 0.0;
        bool AnimateValueChanges = true;
        float ValueTransitionSpeed = 14.0f;
        std::function<void(double)> OnValueChanged;

        virtual void SetValue(double value)
        {
            const double old = Value;
            Value = std::clamp(value, Minimum, Maximum);
            if (old == Value)
                return;
            if (OnValueChanged)
                OnValueChanged(Value);
            ValueChangedEventArgs args;
            args.kind = EventKind::ValueChanged;
            args.route = RoutingStrategy::Bubble;
            args.originalSource = this;
            args.oldValue = old;
            args.newValue = Value;
            RaiseEvent(args);
        }

        double NormalizedValue() const
        {
            const double range = std::max(0.0001, Maximum - Minimum);
            return std::clamp((Value - Minimum) / range, 0.0, 1.0);
        }

        void Update(float deltaTime) override
        {
            Control::Update(deltaTime);
            if (!AnimateValueChanges)
            {
                AnimatedValue = Value;
                return;
            }
            const float step = std::clamp(deltaTime * ValueTransitionSpeed, 0.0f, 1.0f);
            AnimatedValue += (Value - AnimatedValue) * static_cast<double>(step);
            if (std::abs(AnimatedValue - Value) < 0.0001)
                AnimatedValue = Value;
        }
    };

    class SelectorBase : public Control
    {
    public:
        int32_t SelectedIndex = -1;
        std::function<void(int32_t)> OnSelectionChanged;

        virtual void SetSelectedIndex(int32_t index)
        {
            if (SelectedIndex == index)
                return;
            const int32_t old = SelectedIndex;
            SelectedIndex = index;
            if (OnSelectionChanged)
                OnSelectionChanged(SelectedIndex);
            SelectionChangedEventArgs args;
            args.kind = EventKind::SelectionChanged;
            args.route = RoutingStrategy::Bubble;
            args.originalSource = this;
            args.oldIndex = old;
            args.newIndex = SelectedIndex;
            RaiseEvent(args);
        }
    };

    class Panel : public UIElement
    {
    public:
        std::vector<std::shared_ptr<UIElement>> Children;

        void AddChild(std::shared_ptr<UIElement> child)
        {
            if (!child)
                return;
            AdoptChild(child);
            Children.push_back(std::move(child));
            InvalidateMeasure();
        }

        void RemoveChild(UIElement* child)
        {
            const size_t previousCount = Children.size();
            Children.erase(std::remove_if(Children.begin(), Children.end(),
                [this, child](const std::shared_ptr<UIElement>& item)
                {
                    if (item.get() == child)
                    {
                        DetachChild(item.get());
                        return true;
                    }
                    return false;
                }), Children.end());
            if (Children.size() != previousCount)
            {
                InvalidateMeasure();
            }
        }

        void VisitChildren(const std::function<void(UIElement*)>& visitor) override
        {
            for (auto& child : Children)
                if (child)
                    visitor(child.get());
        }

        void Update(float deltaTime) override
        {
            for (auto& child : Children)
                child->Update(deltaTime);
        }

        UIElement* HitTest(Vec2 point) override
        {
            if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible)
                return nullptr;
            const bool contains = Bounds.Contains(point);
            if (ClipToBounds && !contains)
                return nullptr;

            for (auto it = Children.rbegin(); it != Children.rend(); ++it)
            {
                if (UIElement* hit = (*it)->HitTestOverlay(point))
                    return hit;
            }

            for (auto it = Children.rbegin(); it != Children.rend(); ++it)
            {
                if (UIElement* hit = (*it)->HitTest(point))
                    return hit;
            }

            return contains ? this : nullptr;
        }

        UIElement* HitTestOverlay(Vec2 point) override
        {
            if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible)
                return nullptr;
            if (ClipToBounds && !Bounds.Contains(point))
                return nullptr;
            for (auto it = Children.rbegin(); it != Children.rend(); ++it)
            {
                if (UIElement* hit = (*it)->HitTestOverlay(point))
                    return hit;
            }
            return nullptr;
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            Vec2 maxDesired = {};
            Vec2 childAvailable = {
                std::max(0.0f, availableSize.x - Padding.Horizontal()),
                std::max(0.0f, availableSize.y - Padding.Vertical())
            };

            for (auto& child : Children)
            {
                child->Measure(childAvailable);
                maxDesired.x = std::max(maxDesired.x, child->DesiredSize.x);
                maxDesired.y = std::max(maxDesired.y, child->DesiredSize.y);
            }

            return { maxDesired.x + Padding.Horizontal(), maxDesired.y + Padding.Vertical() };
        }

        void ArrangeOverride(Rect finalRect) override
        {
            Rect childRect = {
                finalRect.x + Padding.left,
                finalRect.y + Padding.top,
                std::max(0.0f, finalRect.width - Padding.Horizontal()),
                std::max(0.0f, finalRect.height - Padding.Vertical())
            };

            for (auto& child : Children)
                child->Arrange(childRect);
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            DrawBackgroundAndBorder(drawList);
            for (auto& child : Children)
                child->Render(drawList);
            for (auto& child : Children)
                child->RenderOverlay(drawList);
        }
    };

    class Border : public UIElement
    {
    public:
        std::shared_ptr<UIElement> Child;
        BorderStyle Style;

        void SetChild(std::shared_ptr<UIElement> child)
        {
            if (Child)
                DetachChild(Child.get());
            Child = std::move(child);
            if (Child)
                AdoptChild(Child);
            InvalidateMeasure();
        }

        UIElement* HitTest(Vec2 point) override
        {
            if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible)
                return nullptr;
            const bool contains = Bounds.Contains(point);
            if (ClipToBounds && !contains)
                return nullptr;
            if (Child)
            {
                if (auto* hit = Child->HitTest(point))
                    return hit;
            }
            return contains ? this : nullptr;
        }

        UIElement* HitTestOverlay(Vec2 point) override
        {
            if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible)
                return nullptr;
            if (ClipToBounds && !Bounds.Contains(point))
                return nullptr;
            return Child ? Child->HitTestOverlay(point) : nullptr;
        }

        void Update(float deltaTime) override
        {
            if (Child)
                Child->Update(deltaTime);
        }

        void VisitChildren(const std::function<void(UIElement*)>& visitor) override
        {
            if (Child)
                visitor(Child.get());
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            Vec2 childAvailable = {
                std::max(0.0f, availableSize.x - Padding.Horizontal() - BorderThickness.Horizontal()),
                std::max(0.0f, availableSize.y - Padding.Vertical() - BorderThickness.Vertical())
            };

            Vec2 desired = {};
            if (Child)
            {
                Child->Measure(childAvailable);
                desired = Child->DesiredSize;
            }

            return {
                desired.x + Padding.Horizontal() + BorderThickness.Horizontal(),
                desired.y + Padding.Vertical() + BorderThickness.Vertical()
            };
        }

        void ArrangeOverride(Rect finalRect) override
        {
            if (!Child)
                return;

            Rect childRect = {
                finalRect.x + Padding.left + BorderThickness.left,
                finalRect.y + Padding.top + BorderThickness.top,
                std::max(0.0f, finalRect.width - Padding.Horizontal() - BorderThickness.Horizontal()),
                std::max(0.0f, finalRect.height - Padding.Vertical() - BorderThickness.Vertical())
            };
            Child->Arrange(childRect);
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            VisualState state = GetVisualState();
            if (HasStyledPartVisual(Style.PART_Background) || HasStyledPartVisual(Style.PART_Border) || HasStyledPartVisual(Style.PART_Root))
            {
                DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
                ControlPartStyle background = Style.PART_Background;
                if (!HasStyledPartVisual(background))
                {
                    background.BackgroundColor = Background;
                    background.CornerRadius = CornerRadius;
                    background.Opacity = Opacity;
                }
                DrawStyledPart(drawList, Bounds, background, state);

                ControlPartStyle border = Style.PART_Border;
                if (!HasStyledPartVisual(border))
                {
                    border.BorderColor = BorderBrush;
                    border.BorderThickness = BorderThickness.left;
                    border.CornerRadius = CornerRadius;
                    border.Opacity = Opacity;
                }
                DrawStyledPart(drawList, Bounds, border, state);
            }
            else
            {
                DrawBackgroundAndBorder(drawList);
            }
            if (Child)
            {
                Child->Render(drawList);
                Child->RenderOverlay(drawList);
            }
        }
    };

    class ControlExample : public UIElement
    {
    public:
        std::string HeaderText = "Control example";
        std::string SourceCodeText = "Source code";
        std::shared_ptr<UIElement> Example;
        std::shared_ptr<UIElement> Options;
        std::shared_ptr<UIElement> Output;
        bool ShowSourceCode = true;
        float MinExampleHeight = 96.0f;

        ControlExample()
        {
            Width = AutoSize;
            Foreground = ColorFromBytes(50, 49, 48);
        }

        void SetExample(std::shared_ptr<UIElement> child)
        {
            SetSlot(Example, std::move(child));
        }

        void SetOptions(std::shared_ptr<UIElement> child)
        {
            SetSlot(Options, std::move(child));
        }

        void SetOutput(std::shared_ptr<UIElement> child)
        {
            SetSlot(Output, std::move(child));
        }

        void Update(float deltaTime) override
        {
            if (Example)
                Example->Update(deltaTime);
            if (Options)
                Options->Update(deltaTime);
            if (Output)
                Output->Update(deltaTime);
        }

        UIElement* HitTest(Vec2 point) override
        {
            if (!UIElement::HitTest(point))
                return nullptr;
            if (Output)
            {
                if (auto* hit = Output->HitTest(point))
                    return hit;
            }
            if (Options)
            {
                if (auto* hit = Options->HitTest(point))
                    return hit;
            }
            if (Example)
            {
                if (auto* hit = Example->HitTest(point))
                    return hit;
            }
            return this;
        }

        UIElement* HitTestOverlay(Vec2 point) override
        {
            if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible || !Bounds.Contains(point))
                return nullptr;
            if (Output)
            {
                if (auto* hit = Output->HitTestOverlay(point))
                    return hit;
            }
            if (Options)
            {
                if (auto* hit = Options->HitTestOverlay(point))
                    return hit;
            }
            if (Example)
            {
                if (auto* hit = Example->HitTestOverlay(point))
                    return hit;
            }
            return nullptr;
        }

        void VisitChildren(const std::function<void(UIElement*)>& visitor) override
        {
            if (Example)
                visitor(Example.get());
            if (Options)
                visitor(Options.get());
            if (Output)
                visitor(Output.get());
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            const float width = Width >= 0.0f ? Width : std::min(availableSize.x, 1320.0f);
            const float innerWidth = std::max(0.0f, width - 32.0f);
            const float sideWidth = HasSideContent() ? 260.0f : 0.0f;
            const float exampleWidth = HasSideContent() ? std::max(0.0f, innerWidth - sideWidth - 28.0f) : innerWidth;

            Vec2 exampleSize = {};
            Vec2 optionsSize = {};
            Vec2 outputSize = {};
            if (Example)
            {
                Example->Measure({ exampleWidth, InfiniteSize });
                exampleSize = Example->DesiredSize;
            }
            if (Options)
            {
                Options->Measure({ sideWidth, InfiniteSize });
                optionsSize = Options->DesiredSize;
            }
            if (Output)
            {
                Output->Measure({ sideWidth > 0.0f ? sideWidth : innerWidth, InfiniteSize });
                outputSize = Output->DesiredSize;
            }

            const float sideHeight = optionsSize.y + (Options && Output ? 12.0f : 0.0f) + outputSize.y;
            const float cardHeight = std::max(MinExampleHeight, std::max(exampleSize.y, sideHeight)) + 32.0f;
            const float sourceHeight = ShowSourceCode ? 48.0f : 0.0f;
            const float sourceGap = ShowSourceCode ? 12.0f : 0.0f;
            return { width, HeaderHeight() + 10.0f + cardHeight + sourceGap + sourceHeight };
        }

        void ArrangeOverride(Rect finalRect) override
        {
            const Rect card = CardRect(finalRect);
            const Rect inner = { card.x + 16.0f, card.y + 16.0f, std::max(0.0f, card.width - 32.0f), std::max(0.0f, card.height - 32.0f) };
            const float sideWidth = HasSideContent() ? 260.0f : 0.0f;
            const float exampleWidth = HasSideContent() ? std::max(0.0f, inner.width - sideWidth - 28.0f) : inner.width;

            if (Example)
            {
                const float desiredWidth = std::max(0.0f, Example->DesiredSize.x);
                const float desiredHeight = std::max(0.0f, Example->DesiredSize.y);
                const float arrangedWidth = Example->HorizontalAlignment == FyGUI::HorizontalAlignment::Stretch ? exampleWidth : std::min(exampleWidth, desiredWidth);
                const float arrangedHeight = Example->VerticalAlignment == FyGUI::VerticalAlignment::Stretch ? std::min(inner.height, desiredHeight) : std::min(inner.height, desiredHeight);
                float exampleX = inner.x;
                float exampleY = inner.y;
                if (Example->HorizontalAlignment == FyGUI::HorizontalAlignment::Center)
                    exampleX += std::max(0.0f, exampleWidth - arrangedWidth) * 0.5f;
                else if (Example->HorizontalAlignment == FyGUI::HorizontalAlignment::Right)
                    exampleX += std::max(0.0f, exampleWidth - arrangedWidth);
                if (Example->VerticalAlignment == FyGUI::VerticalAlignment::Center)
                    exampleY += std::max(0.0f, inner.height - arrangedHeight) * 0.5f;
                else if (Example->VerticalAlignment == FyGUI::VerticalAlignment::Bottom)
                    exampleY += std::max(0.0f, inner.height - arrangedHeight);
                Example->Arrange({ exampleX, exampleY, arrangedWidth, arrangedHeight });
            }

            float sideY = inner.y;
            const float sideX = inner.Right() - sideWidth;
            if (Options)
            {
                Options->Arrange({ sideX, sideY, sideWidth, Options->DesiredSize.y });
                sideY += Options->DesiredSize.y + 12.0f;
            }
            if (Output)
                Output->Arrange({ sideX, sideY, sideWidth > 0.0f ? sideWidth : inner.width, Output->DesiredSize.y });
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            drawList.AddText(GetDefaultFont(), GetFontSize(), { Bounds.x, Bounds.y }, Foreground.ToU32(Opacity), HeaderText.c_str(), nullptr, Bounds.width);

            const Rect card = CardRect(Bounds);
            drawList.AddRectFilled({ card.Left(), card.Top() }, { card.Right(), card.Bottom() }, ColorFromBytes(243, 243, 243, 255), 6.0f);
            drawList.AddRect({ card.Left(), card.Top() }, { card.Right(), card.Bottom() }, ColorFromBytes(225, 225, 225, 255), 6.0f, ImDrawFlags_None, 1.0f);

            if (Example)
                Example->Render(drawList);
            if (Options)
                Options->Render(drawList);
            if (Output)
                Output->Render(drawList);

            if (ShowSourceCode)
            {
                const Rect source = SourceRect(Bounds);
                drawList.AddRectFilled({ source.Left(), source.Top() }, { source.Right(), source.Bottom() }, ColorFromBytes(255, 255, 255, 255), 4.0f);
                drawList.AddRect({ source.Left(), source.Top() }, { source.Right(), source.Bottom() }, ColorFromBytes(225, 225, 225, 255), 4.0f, ImDrawFlags_None, 1.0f);
                drawList.AddText(GetDefaultFont(), GetFontSize(), { source.x + 12.0f, source.y + (source.height - GetFontSize()) * 0.5f }, ColorFromBytes(66, 66, 66).ToU32(Opacity), SourceCodeText.c_str(), nullptr, source.width - 24.0f);
            }
        }

        void RenderOverlay(ImDrawList& drawList) override
        {
            if (Visibility != FyGUI::Visibility::Visible)
                return;
            if (Example)
                Example->RenderOverlay(drawList);
            if (Options)
                Options->RenderOverlay(drawList);
            if (Output)
                Output->RenderOverlay(drawList);
        }

    private:
        bool HasSideContent() const
        {
            return Options != nullptr || Output != nullptr;
        }

        static float HeaderHeight()
        {
            return 20.0f;
        }

        Rect CardRect(Rect rect) const
        {
            const float y = rect.y + HeaderHeight() + 10.0f;
            const float bottomReserve = ShowSourceCode ? 60.0f : 0.0f;
            return { rect.x, y, rect.width, std::max(0.0f, rect.height - HeaderHeight() - 10.0f - bottomReserve) };
        }

        Rect SourceRect(Rect rect) const
        {
            return { rect.x, rect.Bottom() - 48.0f, rect.width, 38.0f };
        }

        void SetSlot(std::shared_ptr<UIElement>& slot, std::shared_ptr<UIElement> child)
        {
            if (slot)
                DetachChild(slot.get());
            slot = std::move(child);
            if (slot)
                AdoptChild(slot);
            InvalidateMeasure();
        }
    };

    class Canvas : public Panel
    {
    public:
        using Panel::AddChild;

        void AddChild(std::shared_ptr<UIElement> child, float left, float top)
        {
            AddChild(std::move(child), left, top, AutoSize, AutoSize, 0);
        }

        void AddChild(std::shared_ptr<UIElement> child, float left, float top, float right, float bottom)
        {
            AddChild(std::move(child), left, top, right, bottom, 0);
        }

        void AddChild(std::shared_ptr<UIElement> child, float left, float top, float right, float bottom, int32_t zIndex)
        {
            if (!child)
                return;

            UIElement* rawChild = child.get();
            Panel::AddChild(std::move(child));
            SetChildPlacement(rawChild, left, top, right, bottom, zIndex);
        }

        void SetChildPlacement(UIElement* element, float left, float top, float right = AutoSize, float bottom = AutoSize, int32_t zIndex = 0)
        {
            auto& placement = PlacementFor(element);
            placement.left = left;
            placement.top = top;
            placement.right = right;
            placement.bottom = bottom;
            placement.zIndex = zIndex;
            InvalidateArrange();
        }

        static void SetLeft(UIElement* element, float value) { PlacementFor(element).left = value; if (element) element->InvalidateArrange(); }
        static void SetTop(UIElement* element, float value) { PlacementFor(element).top = value; if (element) element->InvalidateArrange(); }
        static void SetRight(UIElement* element, float value) { PlacementFor(element).right = value; if (element) element->InvalidateArrange(); }
        static void SetBottom(UIElement* element, float value) { PlacementFor(element).bottom = value; if (element) element->InvalidateArrange(); }
        static void SetZIndex(UIElement* element, int32_t value) { PlacementFor(element).zIndex = value; if (element) element->InvalidateArrange(); }
        static float GetLeft(UIElement* element) { return PlacementFor(element).left; }
        static float GetTop(UIElement* element) { return PlacementFor(element).top; }
        static int32_t GetZIndex(UIElement* element) { return PlacementFor(element).zIndex; }

        UIElement* HitTest(Vec2 point) override
        {
            if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible)
                return nullptr;
            const bool contains = Bounds.Contains(point);
            if (ClipToBounds && !contains)
                return nullptr;

            std::vector<std::shared_ptr<UIElement>> sorted = Children;
            std::stable_sort(sorted.begin(), sorted.end(), [](const std::shared_ptr<UIElement>& a, const std::shared_ptr<UIElement>& b)
            {
                return PlacementFor(a.get()).zIndex < PlacementFor(b.get()).zIndex;
            });

            for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
            {
                if (UIElement* hit = (*it)->HitTest(point))
                    return hit;
            }

            return contains ? this : nullptr;
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            Vec2 desired = {};
            for (auto& child : Children)
            {
                child->Measure({ InfiniteSize, InfiniteSize });
                Placement p = PlacementFor(child.get());
                desired.x = std::max(desired.x, p.left + child->DesiredSize.x);
                desired.y = std::max(desired.y, p.top + child->DesiredSize.y);
            }
            return {
                Width >= 0.0f ? Width : std::min(availableSize.x, desired.x + Padding.Horizontal()),
                Height >= 0.0f ? Height : std::min(availableSize.y, desired.y + Padding.Vertical())
            };
        }

        void ArrangeOverride(Rect finalRect) override
        {
            Rect inner = {
                finalRect.x + Padding.left,
                finalRect.y + Padding.top,
                std::max(0.0f, finalRect.width - Padding.Horizontal()),
                std::max(0.0f, finalRect.height - Padding.Vertical())
            };

            for (auto& child : Children)
            {
                Placement p = PlacementFor(child.get());
                float width = std::max(0.0f, child->DesiredSize.x);
                float height = std::max(0.0f, child->DesiredSize.y);
                float x = p.right >= 0.0f ? inner.Right() - p.right - width : inner.x + p.left;
                float y = p.bottom >= 0.0f ? inner.Bottom() - p.bottom - height : inner.y + p.top;
                child->Arrange({ x, y, width, height });
            }
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            DrawBackgroundAndBorder(drawList);
            std::vector<std::shared_ptr<UIElement>> sorted = Children;
            std::stable_sort(sorted.begin(), sorted.end(), [](const std::shared_ptr<UIElement>& a, const std::shared_ptr<UIElement>& b)
            {
                return PlacementFor(a.get()).zIndex < PlacementFor(b.get()).zIndex;
            });
            for (auto& child : sorted)
                child->Render(drawList);
            for (auto& child : sorted)
                child->RenderOverlay(drawList);
        }

    private:
        struct Placement
        {
            float left = 0.0f;
            float top = 0.0f;
            float right = AutoSize;
            float bottom = AutoSize;
            int32_t zIndex = 0;
        };

        static Placement& PlacementFor(UIElement* element)
        {
            static std::unordered_map<UIElement*, Placement> placements;
            return placements[element];
        }
    };

    class Shape : public UIElement
    {
    public:
        Color Fill = { 0.0f, 0.0f, 0.0f, 0.0f };
        Color Stroke = { 0.0f, 0.0f, 0.0f, 0.0f };
        float StrokeThickness = 1.0f;

    protected:
        Vec2 MeasureOverride(Vec2) override
        {
            return { Width >= 0.0f ? Width : 0.0f, Height >= 0.0f ? Height : 0.0f };
        }
    };

    class Rectangle : public Shape
    {
    public:
        float RadiusX = 0.0f;
        float RadiusY = 0.0f;

    protected:
        void RenderOverride(ImDrawList& drawList) override
        {
            const float rounding = std::max(RadiusX, RadiusY);
            if (Fill.a > 0.0f)
                drawList.AddRectFilled(Vec2(Bounds.Left(), Bounds.Top()), Vec2(Bounds.Right(), Bounds.Bottom()), WithOpacity(Fill, Opacity), rounding);
            if (StrokeThickness > 0.0f && Stroke.a > 0.0f)
                drawList.AddRect(Vec2(Bounds.Left(), Bounds.Top()), Vec2(Bounds.Right(), Bounds.Bottom()), WithOpacity(Stroke, Opacity), rounding, ImDrawFlags_None, StrokeThickness);
        }
    };

    class Ellipse : public Shape
    {
    protected:
        void RenderOverride(ImDrawList& drawList) override
        {
            const Vec2 center = { Bounds.x + Bounds.width * 0.5f, Bounds.y + Bounds.height * 0.5f };
            const float rx = std::max(0.0f, Bounds.width * 0.5f);
            const float ry = std::max(0.0f, Bounds.height * 0.5f);
            if (rx <= 0.0f || ry <= 0.0f)
                return;

            const int segments = std::clamp(static_cast<int>((rx + ry) * 0.35f), 24, 96);
            if (Fill.a > 0.0f)
            {
                drawList.PathClear();
                for (int i = 0; i < segments; ++i)
                {
                    const float a = (static_cast<float>(i) / static_cast<float>(segments)) * 6.28318530718f;
                    drawList.PathLineTo({ center.x + std::cos(a) * rx, center.y + std::sin(a) * ry });
                }
                drawList.PathFillConvex(WithOpacity(Fill, Opacity));
            }
            if (StrokeThickness > 0.0f && Stroke.a > 0.0f)
            {
                drawList.PathClear();
                for (int i = 0; i < segments; ++i)
                {
                    const float a = (static_cast<float>(i) / static_cast<float>(segments)) * 6.28318530718f;
                    drawList.PathLineTo({ center.x + std::cos(a) * rx, center.y + std::sin(a) * ry });
                }
                drawList.PathStroke(WithOpacity(Stroke, Opacity), ImDrawFlags_Closed, StrokeThickness);
            }
        }
    };

    class Line : public Shape
    {
    public:
        float X1 = 0.0f;
        float Y1 = 0.0f;
        float X2 = 0.0f;
        float Y2 = 0.0f;

    protected:
        Vec2 MeasureOverride(Vec2) override
        {
            const float desiredW = std::max(X1, X2) + StrokeThickness;
            const float desiredH = std::max(Y1, Y2) + StrokeThickness;
            return { Width >= 0.0f ? Width : desiredW, Height >= 0.0f ? Height : desiredH };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            const Color color = Stroke.a > 0.0f ? Stroke : Fill;
            if (StrokeThickness > 0.0f && color.a > 0.0f)
                drawList.AddLine({ Bounds.x + X1, Bounds.y + Y1 }, { Bounds.x + X2, Bounds.y + Y2 }, WithOpacity(color, Opacity), StrokeThickness);
        }
    };

    class StackPanel : public Panel
    {
    public:
        FyGUI::Orientation Orientation = FyGUI::Orientation::Vertical;
        float Spacing = 4.0f;

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            Vec2 inner = {
                std::max(0.0f, availableSize.x - Padding.Horizontal()),
                std::max(0.0f, availableSize.y - Padding.Vertical())
            };

            Vec2 desired = {};
            int visibleCount = 0;
            for (auto& child : Children)
            {
                if (child->Visibility == FyGUI::Visibility::Collapsed)
                    continue;

                if (Orientation == FyGUI::Orientation::Vertical)
                    child->Measure({ inner.x, InfiniteSize });
                else
                    child->Measure({ InfiniteSize, inner.y });

                ++visibleCount;
                if (Orientation == FyGUI::Orientation::Vertical)
                {
                    desired.x = std::max(desired.x, child->DesiredSize.x);
                    desired.y += child->DesiredSize.y;
                }
                else
                {
                    desired.x += child->DesiredSize.x;
                    desired.y = std::max(desired.y, child->DesiredSize.y);
                }
            }

            if (visibleCount > 1)
            {
                if (Orientation == FyGUI::Orientation::Vertical)
                    desired.y += Spacing * static_cast<float>(visibleCount - 1);
                else
                    desired.x += Spacing * static_cast<float>(visibleCount - 1);
            }

            return { desired.x + Padding.Horizontal(), desired.y + Padding.Vertical() };
        }

        void ArrangeOverride(Rect finalRect) override
        {
            Rect inner = {
                finalRect.x + Padding.left,
                finalRect.y + Padding.top,
                std::max(0.0f, finalRect.width - Padding.Horizontal()),
                std::max(0.0f, finalRect.height - Padding.Vertical())
            };

            float cursor = Orientation == FyGUI::Orientation::Vertical ? inner.y : inner.x;
            for (auto& child : Children)
            {
                if (child->Visibility == FyGUI::Visibility::Collapsed)
                    continue;

                if (Orientation == FyGUI::Orientation::Vertical)
                {
                    float width = child->HorizontalAlignment == FyGUI::HorizontalAlignment::Stretch ? inner.width : std::min(inner.width, std::max(0.0f, child->DesiredSize.x));
                    float height = std::max(0.0f, child->DesiredSize.y);
                    float x = inner.x;
                    if (child->HorizontalAlignment == FyGUI::HorizontalAlignment::Center)
                        x += std::max(0.0f, inner.width - width) * 0.5f;
                    else if (child->HorizontalAlignment == FyGUI::HorizontalAlignment::Right)
                        x += std::max(0.0f, inner.width - width);
                    child->Arrange({ x, cursor, width, height });
                    cursor += height + Spacing;
                }
                else
                {
                    float width = std::max(0.0f, child->DesiredSize.x);
                    float height = child->VerticalAlignment == FyGUI::VerticalAlignment::Stretch ? inner.height : std::min(inner.height, std::max(0.0f, child->DesiredSize.y));
                    float y = inner.y;
                    if (child->VerticalAlignment == FyGUI::VerticalAlignment::Center)
                        y += std::max(0.0f, inner.height - height) * 0.5f;
                    else if (child->VerticalAlignment == FyGUI::VerticalAlignment::Bottom)
                        y += std::max(0.0f, inner.height - height);
                    child->Arrange({ cursor, y, width, height });
                    cursor += width + Spacing;
                }
            }
        }
    };

    enum class GridUnitType : uint8_t
    {
        Auto,
        Pixel,
        Star
    };

    class GridLength
    {
    public:
        GridLength() = default;
        explicit GridLength(float value, GridUnitType type = GridUnitType::Pixel) : m_value(value), m_type(type) {}

        static GridLength Auto() { return GridLength(1.0f, GridUnitType::Auto); }
        static GridLength Star(float value = 1.0f) { return GridLength(value, GridUnitType::Star); }

        float Value() const { return m_value; }
        GridUnitType Type() const { return m_type; }
        bool IsAuto() const { return m_type == GridUnitType::Auto; }
        bool IsPixel() const { return m_type == GridUnitType::Pixel; }
        bool IsStar() const { return m_type == GridUnitType::Star; }

    private:
        float m_value = 1.0f;
        GridUnitType m_type = GridUnitType::Star;
    };

    struct RowDefinition
    {
        GridLength Height = GridLength::Star();
        float ActualHeight = 0.0f;
        float MinHeight = 0.0f;
        float MaxHeight = InfiniteSize;
    };

    struct ColumnDefinition
    {
        GridLength Width = GridLength::Star();
        float ActualWidth = 0.0f;
        float MinWidth = 0.0f;
        float MaxWidth = InfiniteSize;
    };

    class Grid : public Panel
    {
    public:
        std::vector<RowDefinition> Rows;
        std::vector<ColumnDefinition> Columns;

        void AddRow(RowDefinition row) { Rows.push_back(row); }
        void AddColumn(ColumnDefinition column) { Columns.push_back(column); }

        void AddChild(std::shared_ptr<UIElement> child, int row, int column, int rowSpan = 1, int columnSpan = 1)
        {
            UIElement* raw = child.get();
            Panel::AddChild(std::move(child));
            m_cells[raw] = { row, column, std::max(1, rowSpan), std::max(1, columnSpan) };
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            EnsureDefinitions();
            ResolveDefinitions(availableSize);

            for (auto& child : Children)
            {
                Cell cell = CellFor(child.get());
                Vec2 cellSize = GetCellSize(cell);
                child->Measure(cellSize);
            }

            Vec2 desired = { Padding.Horizontal(), Padding.Vertical() };
            for (const auto& column : Columns)
                desired.x += column.ActualWidth;
            for (const auto& row : Rows)
                desired.y += row.ActualHeight;
            return desired;
        }

        void ArrangeOverride(Rect finalRect) override
        {
            EnsureDefinitions();
            ResolveDefinitions({ finalRect.width, finalRect.height });

            for (auto& child : Children)
            {
                Cell cell = CellFor(child.get());
                Rect bounds = GetCellRect(finalRect, cell);
                child->Arrange(bounds);
            }
        }

    private:
        struct Cell
        {
            int row = 0;
            int column = 0;
            int rowSpan = 1;
            int columnSpan = 1;
        };

        void EnsureDefinitions()
        {
            if (Rows.empty())
                Rows.push_back({});
            if (Columns.empty())
                Columns.push_back({});
        }

        Cell CellFor(UIElement* element) const
        {
            auto it = m_cells.find(element);
            return it == m_cells.end() ? Cell{} : it->second;
        }

        Vec2 GetCellSize(Cell cell) const
        {
            Vec2 size = {};
            for (int c = cell.column; c < cell.column + cell.columnSpan && c < static_cast<int>(Columns.size()); ++c)
                size.x += Columns[static_cast<size_t>(c)].ActualWidth;
            for (int r = cell.row; r < cell.row + cell.rowSpan && r < static_cast<int>(Rows.size()); ++r)
                size.y += Rows[static_cast<size_t>(r)].ActualHeight;
            return size;
        }

        Rect GetCellRect(Rect finalRect, Cell cell) const
        {
            Rect inner = {
                finalRect.x + Padding.left,
                finalRect.y + Padding.top,
                std::max(0.0f, finalRect.width - Padding.Horizontal()),
                std::max(0.0f, finalRect.height - Padding.Vertical())
            };

            for (int c = 0; c < cell.column && c < static_cast<int>(Columns.size()); ++c)
                inner.x += Columns[static_cast<size_t>(c)].ActualWidth;
            for (int r = 0; r < cell.row && r < static_cast<int>(Rows.size()); ++r)
                inner.y += Rows[static_cast<size_t>(r)].ActualHeight;

            Vec2 size = GetCellSize(cell);
            inner.width = size.x;
            inner.height = size.y;
            return inner;
        }

        void ResolveDefinitions(Vec2 availableSize)
        {
            const float innerW = std::max(0.0f, availableSize.x - Padding.Horizontal());
            const float innerH = std::max(0.0f, availableSize.y - Padding.Vertical());
            ResolveColumns(innerW);
            ResolveRows(innerH);
        }

        void ResolveColumns(float width)
        {
            float fixed = 0.0f;
            float star = 0.0f;
            for (auto& column : Columns)
            {
                if (column.Width.IsPixel())
                {
                    column.ActualWidth = std::clamp(column.Width.Value(), column.MinWidth, column.MaxWidth);
                    fixed += column.ActualWidth;
                }
                else if (column.Width.IsAuto())
                {
                    column.ActualWidth = column.MinWidth;
                    fixed += column.ActualWidth;
                }
                else
                {
                    star += std::max(0.0f, column.Width.Value());
                }
            }

            const float remaining = std::max(0.0f, width - fixed);
            for (auto& column : Columns)
            {
                if (column.Width.IsStar())
                {
                    float share = star > 0.0f ? column.Width.Value() / star : 0.0f;
                    column.ActualWidth = std::clamp(remaining * share, column.MinWidth, column.MaxWidth);
                }
            }
        }

        void ResolveRows(float height)
        {
            float fixed = 0.0f;
            float star = 0.0f;
            for (auto& row : Rows)
            {
                if (row.Height.IsPixel())
                {
                    row.ActualHeight = std::clamp(row.Height.Value(), row.MinHeight, row.MaxHeight);
                    fixed += row.ActualHeight;
                }
                else if (row.Height.IsAuto())
                {
                    row.ActualHeight = row.MinHeight;
                    fixed += row.ActualHeight;
                }
                else
                {
                    star += std::max(0.0f, row.Height.Value());
                }
            }

            const float remaining = std::max(0.0f, height - fixed);
            for (auto& row : Rows)
            {
                if (row.Height.IsStar())
                {
                    float share = star > 0.0f ? row.Height.Value() / star : 0.0f;
                    row.ActualHeight = std::clamp(remaining * share, row.MinHeight, row.MaxHeight);
                }
            }
        }

        std::unordered_map<UIElement*, Cell> m_cells;
    };

    enum class Dock : uint8_t
    {
        Left,
        Top,
        Right,
        Bottom
    };

    class DockPanel : public Panel
    {
    public:
        bool LastChildFill = true;

        void AddChild(std::shared_ptr<UIElement> child, Dock dock)
        {
            UIElement* raw = child.get();
            Panel::AddChild(std::move(child));
            m_docks[raw] = dock;
        }

    protected:
        void ArrangeOverride(Rect finalRect) override
        {
            Rect remaining = {
                finalRect.x + Padding.left,
                finalRect.y + Padding.top,
                std::max(0.0f, finalRect.width - Padding.Horizontal()),
                std::max(0.0f, finalRect.height - Padding.Vertical())
            };

            for (size_t i = 0; i < Children.size(); ++i)
            {
                auto& child = Children[i];
                bool fill = LastChildFill && i == Children.size() - 1;
                Dock dock = DockFor(child.get());
                Rect rect = remaining;

                if (!fill)
                {
                    if (dock == Dock::Left || dock == Dock::Right)
                        rect.width = std::min(rect.width, child->DesiredSize.x);
                    else
                        rect.height = std::min(rect.height, child->DesiredSize.y);
                }

                if (dock == Dock::Right)
                    rect.x = remaining.Right() - rect.width;
                else if (dock == Dock::Bottom)
                    rect.y = remaining.Bottom() - rect.height;

                child->Arrange(rect);

                if (fill)
                    continue;
                if (dock == Dock::Left)
                {
                    remaining.x += rect.width;
                    remaining.width = std::max(0.0f, remaining.width - rect.width);
                }
                else if (dock == Dock::Right)
                    remaining.width = std::max(0.0f, remaining.width - rect.width);
                else if (dock == Dock::Top)
                {
                    remaining.y += rect.height;
                    remaining.height = std::max(0.0f, remaining.height - rect.height);
                }
                else
                    remaining.height = std::max(0.0f, remaining.height - rect.height);
            }
        }

    private:
        Dock DockFor(UIElement* element) const
        {
            auto it = m_docks.find(element);
            return it == m_docks.end() ? Dock::Left : it->second;
        }

        std::unordered_map<UIElement*, Dock> m_docks;
    };

    class WrapPanel : public Panel
    {
    public:
        FyGUI::Orientation Orientation = FyGUI::Orientation::Horizontal;
        float Spacing = 8.0f;
        float LineSpacing = 8.0f;
        FyGUI::HorizontalAlignment HorizontalContentAlignment = FyGUI::HorizontalAlignment::Left;
        FyGUI::VerticalAlignment VerticalContentAlignment = FyGUI::VerticalAlignment::Top;

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            Vec2 inner = {
                std::max(0.0f, availableSize.x - Padding.Horizontal()),
                std::max(0.0f, availableSize.y - Padding.Vertical())
            };
            float extent = 0.0f;
            float thickness = 0.0f;
            float totalThickness = 0.0f;
            float maxExtent = 0.0f;
            const float limit = Orientation == FyGUI::Orientation::Horizontal ? inner.x : inner.y;

            for (auto& child : Children)
            {
                child->Measure(inner);
                float childExtent = Orientation == FyGUI::Orientation::Horizontal ? child->DesiredSize.x : child->DesiredSize.y;
                float childThickness = Orientation == FyGUI::Orientation::Horizontal ? child->DesiredSize.y : child->DesiredSize.x;
                if (extent > 0.0f && extent + Spacing + childExtent > limit)
                {
                    maxExtent = std::max(maxExtent, extent);
                    totalThickness += thickness + LineSpacing;
                    extent = 0.0f;
                    thickness = 0.0f;
                }
                if (extent > 0.0f)
                    extent += Spacing;
                extent += childExtent;
                thickness = std::max(thickness, childThickness);
            }

            maxExtent = std::max(maxExtent, extent);
            totalThickness += thickness;
            if (Orientation == FyGUI::Orientation::Horizontal)
                return { maxExtent + Padding.Horizontal(), totalThickness + Padding.Vertical() };
            return { totalThickness + Padding.Horizontal(), maxExtent + Padding.Vertical() };
        }

        void ArrangeOverride(Rect finalRect) override
        {
            Rect inner = {
                finalRect.x + Padding.left,
                finalRect.y + Padding.top,
                std::max(0.0f, finalRect.width - Padding.Horizontal()),
                std::max(0.0f, finalRect.height - Padding.Vertical())
            };
            struct WrapLine
            {
                size_t first = 0;
                size_t last = 0;
                float extent = 0.0f;
                float thickness = 0.0f;
            };

            std::vector<WrapLine> lines;
            size_t lineFirst = 0;
            float lineExtent = 0.0f;
            float lineThickness = 0.0f;
            const float limit = Orientation == FyGUI::Orientation::Horizontal ? inner.width : inner.height;

            auto flushLine = [&](size_t last)
            {
                if (last <= lineFirst)
                    return;
                lines.push_back({ lineFirst, last, lineExtent, lineThickness });
                lineFirst = last;
                lineExtent = 0.0f;
                lineThickness = 0.0f;
            };

            for (size_t i = 0; i < Children.size(); ++i)
            {
                Vec2 size = Children[i]->DesiredSize;
                const float childExtent = Orientation == FyGUI::Orientation::Horizontal ? size.x : size.y;
                const float childThickness = Orientation == FyGUI::Orientation::Horizontal ? size.y : size.x;
                if (lineExtent > 0.0f && lineExtent + Spacing + childExtent > limit)
                    flushLine(i);
                if (lineExtent > 0.0f)
                    lineExtent += Spacing;
                lineExtent += childExtent;
                lineThickness = std::max(lineThickness, childThickness);
            }
            flushLine(Children.size());

            if (Orientation == FyGUI::Orientation::Horizontal)
            {
                float totalThickness = 0.0f;
                for (const WrapLine& line : lines)
                    totalThickness += line.thickness;
                if (!lines.empty())
                    totalThickness += LineSpacing * static_cast<float>(lines.size() - 1);

                float y = inner.y;
                const float freeY = std::max(0.0f, inner.height - totalThickness);
                if (VerticalContentAlignment == FyGUI::VerticalAlignment::Center)
                    y += freeY * 0.5f;
                else if (VerticalContentAlignment == FyGUI::VerticalAlignment::Bottom)
                    y += freeY;

                for (const WrapLine& line : lines)
                {
                    float x = inner.x;
                    const float free = std::max(0.0f, inner.width - line.extent);
                    if (HorizontalContentAlignment == FyGUI::HorizontalAlignment::Center)
                        x += free * 0.5f;
                    else if (HorizontalContentAlignment == FyGUI::HorizontalAlignment::Right)
                        x += free;
                    for (size_t i = line.first; i < line.last; ++i)
                    {
                        Vec2 size = Children[i]->DesiredSize;
                        Children[i]->Arrange({ x, y, size.x, size.y });
                        x += size.x + Spacing;
                    }
                    y += line.thickness + LineSpacing;
                }
            }
            else
            {
                float totalThickness = 0.0f;
                for (const WrapLine& line : lines)
                    totalThickness += line.thickness;
                if (!lines.empty())
                    totalThickness += LineSpacing * static_cast<float>(lines.size() - 1);

                float x = inner.x;
                const float freeX = std::max(0.0f, inner.width - totalThickness);
                if (HorizontalContentAlignment == FyGUI::HorizontalAlignment::Center)
                    x += freeX * 0.5f;
                else if (HorizontalContentAlignment == FyGUI::HorizontalAlignment::Right)
                    x += freeX;

                for (const WrapLine& line : lines)
                {
                    float y = inner.y;
                    const float free = std::max(0.0f, inner.height - line.extent);
                    if (VerticalContentAlignment == FyGUI::VerticalAlignment::Center)
                        y += free * 0.5f;
                    else if (VerticalContentAlignment == FyGUI::VerticalAlignment::Bottom)
                        y += free;
                    for (size_t i = line.first; i < line.last; ++i)
                    {
                        Vec2 size = Children[i]->DesiredSize;
                        Children[i]->Arrange({ x, y, size.x, size.y });
                        y += size.y + Spacing;
                    }
                    x += line.thickness + LineSpacing;
                }
            }
        }
    };

}


// ---- Begin inlined TextBlock control ----
// Inlined after Control and text helpers are declared.
namespace FyGUI
{
    class TextBlock : public Control
    {
    public:
        std::string Text;
        TextWrappingMode TextWrapping = TextWrappingMode::Wrap;
        float FontSize = 0.0f;

        explicit TextBlock(std::string text = {});

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override;
        void RenderOverride(ImDrawList& drawList) override;
        float EffectiveFontSize() const;
    };
}

namespace FyGUI
{
    TextBlock::TextBlock(std::string text) : Text(std::move(text))
    {
        BackgroundNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
        BackgroundHover = BackgroundNormal;
        BackgroundPressed = BackgroundNormal;
        BorderBrush = { 0.0f, 0.0f, 0.0f, 0.0f };
        IsHitTestVisible = false;
    }

    Vec2 TextBlock::MeasureOverride(Vec2 availableSize)
    {
        const float wrapWidth = TextWrapping == TextWrappingMode::NoWrap ? 0.0f : std::max(0.0f, availableSize.x - Padding.Horizontal());
        Vec2 size = MeasureText(Text, wrapWidth, EffectiveFontSize());
        return { size.x + Padding.Horizontal(), size.y + Padding.Vertical() };
    }

    void TextBlock::RenderOverride(ImDrawList& drawList)
    {
        DrawBackgroundAndBorder(drawList);
        const Vec2 pos(Bounds.x + Padding.left, Bounds.y + Padding.top);
        const float wrapWidth = TextWrapping == TextWrappingMode::NoWrap ? 0.0f : std::max(0.0f, Bounds.width - Padding.Horizontal());
        drawList.AddText(GetDefaultFont(), EffectiveFontSize(), pos, Foreground.ToU32(Opacity), Text.c_str(), nullptr, wrapWidth);
    }

    float TextBlock::EffectiveFontSize() const
    {
        return FontSize > 0.0f ? FontSize : GetFontSize();
    }
}

// ---- End inlined TextBlock control ----


namespace FyGUI
{

}


// ---- Begin inlined TextBox control ----
// Inlined after Control, TextBoxStyle and event helpers are declared.
namespace FyGUI
{
    class TextBox : public Control
    {
    public:
        std::string Header;
        std::string Text;
        std::string PlaceholderText;
        bool IsReadOnly = false;
        bool AcceptsReturn = false;
        bool TextWrapping = false;
        int32_t MaxLength = -1;
        int32_t SelectionStart = 0;
        int32_t SelectionLength = 0;
        int32_t CaretIndex = 0;
        float FontSize = 0.0f;
        std::function<void(const std::string&)> OnTextChanged;
        std::function<void(int32_t, int32_t)> OnSelectionChanged;
        std::function<void()> OnSubmit;
        std::function<void()> OnCancel;
        std::function<void()> OnGotFocus;
        std::function<void()> OnLostFocus;
        TextBoxStyle Style;

        explicit TextBox(std::string text = {});

        void SetText(std::string text);
        void SetSelection(int32_t start, int32_t length);

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override;
        void RenderOverride(ImDrawList& drawList) override;
        void OnEvent(EventArgs& args) override;
        float EffectiveFontSize() const;

    private:
        struct TextLine
        {
            int32_t start = 0;
            int32_t end = 0;
            std::string text;
        };

        int32_t m_caretIndex = 0;
        int32_t m_selectionAnchor = 0;
        bool m_isSelecting = false;
        float m_textScrollY = 0.0f;

        int32_t TextLength() const;
        int32_t SelectionBegin() const;
        int32_t SelectionEnd() const;
        bool HasSelection() const;
        void ClampEditingState();
        void SetCaretIndex(int32_t index, bool extendSelection);
        void RaiseSelectionChangedIfNeeded(int32_t oldStart, int32_t oldLength);
        bool DeleteSelection();
        void ReplaceSelection(std::string_view replacement);
        void EnsureCaretVisible(float fontSize);
        Rect BodyRect() const;
        float LineHeight(float fontSize) const;
        std::vector<TextLine> BuildTextLines(std::string_view text, float wrapWidth, float fontSize) const;
        int32_t HitTestTextIndex(Vec2 point, Rect bodyRect, float fontSize) const;
    };
}

namespace FyGUI
{
    TextBox::TextBox(std::string text) : Text(std::move(text))
    {
        Width = 220.0f;
        Height = AutoSize;
        Padding = Thickness(10.0f, 6.0f, 10.0f, 6.0f);
        BorderThickness = Thickness(1.0f);
        CornerRadius = 4.0f;
        BackgroundNormal = ColorFromBytes(255, 255, 255);
        BackgroundHover = ColorFromBytes(255, 255, 255);
        BackgroundPressed = ColorFromBytes(255, 255, 255);
        BorderBrush = ColorFromBytes(138, 136, 134);
        Foreground = ColorFromBytes(32, 31, 30);
        VerticalAlignment = FyGUI::VerticalAlignment::Top;
        ProtectedCursor = Cursor::IBeam;
        IsTabStop = true;
        UseSystemFocusVisuals = false;
    }

    void TextBox::SetText(std::string text)
    {
        if (Text == text)
            return;

        TextChangedEventArgs args;
        args.kind = EventKind::TextChanged;
        args.route = RoutingStrategy::Bubble;
        args.originalSource = this;
        args.oldText = Text;
        args.newText = text;
        Text = std::move(text);
        ClampEditingState();

        if (OnTextChanged)
            OnTextChanged(Text);
        RaiseEvent(args);
    }

    Vec2 TextBox::MeasureOverride(Vec2)
    {
        const float headerHeight = Header.empty() ? 0.0f : 20.0f;
        const float headerGap = Header.empty() ? 0.0f : 6.0f;
        const float bodyHeight = AcceptsReturn ? 96.0f : std::max(32.0f, EffectiveFontSize() + Padding.Vertical());
        return { Width >= 0.0f ? Width : 220.0f, headerHeight + headerGap + (Height >= 0.0f ? Height : bodyHeight) };
    }

    int32_t TextBox::TextLength() const
    {
        return static_cast<int32_t>(Text.size());
    }

    int32_t TextBox::SelectionBegin() const
    {
        return std::clamp(SelectionStart, 0, TextLength());
    }

    int32_t TextBox::SelectionEnd() const
    {
        return std::clamp(SelectionStart + SelectionLength, 0, TextLength());
    }

    bool TextBox::HasSelection() const
    {
        return SelectionEnd() > SelectionBegin();
    }

    void TextBox::SetSelection(int32_t start, int32_t length)
    {
        const int32_t oldStart = SelectionStart;
        const int32_t oldLength = SelectionLength;
        const int32_t textLength = TextLength();
        SelectionStart = std::clamp(start, 0, textLength);
        SelectionLength = std::clamp(length, 0, textLength - SelectionStart);
        m_selectionAnchor = SelectionStart;
        m_caretIndex = SelectionStart + SelectionLength;
        CaretIndex = m_caretIndex;
        EnsureCaretVisible(EffectiveFontSize());
        RaiseSelectionChangedIfNeeded(oldStart, oldLength);
    }

    void TextBox::ClampEditingState()
    {
        const int32_t length = TextLength();
        m_caretIndex = std::clamp(m_caretIndex, 0, length);
        CaretIndex = m_caretIndex;
        m_selectionAnchor = std::clamp(m_selectionAnchor, 0, length);
        SelectionStart = std::clamp(SelectionStart, 0, length);
        SelectionLength = std::clamp(SelectionLength, 0, length - SelectionStart);
    }

    void TextBox::SetCaretIndex(int32_t index, bool extendSelection)
    {
        index = std::clamp(index, 0, TextLength());
        const int32_t oldStart = SelectionStart;
        const int32_t oldLength = SelectionLength;
        if (!extendSelection)
        {
            m_caretIndex = index;
            CaretIndex = m_caretIndex;
            m_selectionAnchor = index;
            SelectionStart = index;
            SelectionLength = 0;
            EnsureCaretVisible(EffectiveFontSize());
            RaiseSelectionChangedIfNeeded(oldStart, oldLength);
            return;
        }

        m_caretIndex = index;
        CaretIndex = m_caretIndex;
        SelectionStart = std::min(m_selectionAnchor, m_caretIndex);
        SelectionLength = std::abs(m_caretIndex - m_selectionAnchor);
        EnsureCaretVisible(EffectiveFontSize());
        RaiseSelectionChangedIfNeeded(oldStart, oldLength);
    }

    void TextBox::RaiseSelectionChangedIfNeeded(int32_t oldStart, int32_t oldLength)
    {
        if (oldStart == SelectionStart && oldLength == SelectionLength)
            return;

        SelectionChangedEventArgs args;
        args.kind = EventKind::SelectionChanged;
        args.route = RoutingStrategy::Bubble;
        args.originalSource = this;
        args.oldIndex = oldStart;
        args.newIndex = SelectionStart;
        if (OnSelectionChanged)
            OnSelectionChanged(SelectionStart, SelectionLength);
        RaiseEvent(args);
    }

    bool TextBox::DeleteSelection()
    {
        if (!HasSelection())
            return false;
        const int32_t begin = SelectionBegin();
        const int32_t count = SelectionEnd() - begin;
        std::string next = Text;
        next.erase(static_cast<size_t>(begin), static_cast<size_t>(count));
        SetText(std::move(next));
        SetCaretIndex(begin, false);
        return true;
    }

    void TextBox::ReplaceSelection(std::string_view replacement)
    {
        const int32_t begin = HasSelection() ? SelectionBegin() : m_caretIndex;
        const int32_t end = HasSelection() ? SelectionEnd() : m_caretIndex;
        std::string next = Text;
        next.replace(static_cast<size_t>(begin), static_cast<size_t>(end - begin), replacement.data(), replacement.size());
        if (MaxLength >= 0 && static_cast<int32_t>(next.size()) > MaxLength)
            next.resize(static_cast<size_t>(MaxLength));
        const int32_t caret = std::min<int32_t>(static_cast<int32_t>(begin + replacement.size()), static_cast<int32_t>(next.size()));
        SetText(std::move(next));
        SetCaretIndex(caret, false);
    }

    void TextBox::EnsureCaretVisible(float fontSize)
    {
        if (!AcceptsReturn)
        {
            m_textScrollY = 0.0f;
            return;
        }

        const Rect body = BodyRect();
        const float viewportHeight = std::max(0.0f, body.height - Padding.Vertical());
        if (body.width <= 0.0f || viewportHeight <= 0.0f)
            return;

        const float wrapWidth = TextWrapping ? std::max(0.0f, body.width - Padding.Horizontal()) : 0.0f;
        const std::vector<TextLine> lines = BuildTextLines(Text, wrapWidth, fontSize);
        const float lineHeight = LineHeight(fontSize);
        size_t caretLineIndex = 0;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (m_caretIndex >= lines[i].start && m_caretIndex <= lines[i].end)
            {
                caretLineIndex = i;
                break;
            }
        }

        const float caretTop = lineHeight * static_cast<float>(caretLineIndex);
        const float caretBottom = caretTop + lineHeight;
        if (caretTop < m_textScrollY)
            m_textScrollY = caretTop;
        else if (caretBottom > m_textScrollY + viewportHeight)
            m_textScrollY = caretBottom - viewportHeight;

        const float contentHeight = std::max(lineHeight, lineHeight * static_cast<float>(lines.size()));
        m_textScrollY = std::clamp(m_textScrollY, 0.0f, std::max(0.0f, contentHeight - viewportHeight));
    }

    Rect TextBox::BodyRect() const
    {
        const float headerHeight = Header.empty() ? 0.0f : 20.0f;
        const float headerGap = Header.empty() ? 0.0f : 6.0f;
        return { Bounds.x, Bounds.y + headerHeight + headerGap, Bounds.width, std::max(0.0f, Bounds.height - headerHeight - headerGap) };
    }

    float TextBox::LineHeight(float fontSize) const
    {
        return std::ceil(fontSize + 4.0f);
    }

    std::vector<TextBox::TextLine> TextBox::BuildTextLines(std::string_view text, float wrapWidth, float fontSize) const
    {
        std::vector<TextLine> lines;
        const int32_t length = static_cast<int32_t>(text.size());
        int32_t lineStart = 0;
        int32_t currentStart = 0;
        std::string current;
        auto pushLine = [&](int32_t start, int32_t end)
        {
            TextLine line;
            line.start = std::clamp(start, 0, length);
            line.end = std::clamp(end, line.start, length);
            line.text.assign(text.substr(static_cast<size_t>(line.start), static_cast<size_t>(line.end - line.start)));
            lines.push_back(std::move(line));
        };

        for (int32_t i = 0; i < length; ++i)
        {
            const char c = text[static_cast<size_t>(i)];
            if (c == '\n')
            {
                pushLine(currentStart, i);
                current.clear();
                currentStart = i + 1;
                lineStart = currentStart;
                continue;
            }

            if (TextWrapping && wrapWidth > 1.0f && !current.empty())
            {
                std::string candidate = current;
                candidate.push_back(c);
                if (MeasureText(candidate, 0.0f, fontSize).x > wrapWidth)
                {
                    pushLine(lineStart, i);
                    current.clear();
                    lineStart = i;
                    currentStart = i;
                }
            }
            current.push_back(c);
        }

        pushLine(currentStart, length);
        if (lines.empty())
            pushLine(0, 0);
        return lines;
    }

    int32_t TextBox::HitTestTextIndex(Vec2 point, Rect bodyRect, float fontSize) const
    {
        if (Text.empty())
            return 0;
        const float wrapWidth = TextWrapping ? std::max(0.0f, bodyRect.width - Padding.Horizontal()) : 0.0f;
        std::vector<TextLine> lines = BuildTextLines(Text, wrapWidth, fontSize);
        const float lineHeight = LineHeight(fontSize);
        const float textTop = AcceptsReturn ? bodyRect.y + Padding.top - m_textScrollY : bodyRect.y + (bodyRect.height - lineHeight) * 0.5f;
        int32_t lineIndex = static_cast<int32_t>(std::floor((point.y - textTop) / std::max(1.0f, lineHeight)));
        lineIndex = std::clamp(lineIndex, 0, static_cast<int32_t>(lines.size()) - 1);
        const TextLine& line = lines[static_cast<size_t>(lineIndex)];
        const float relX = point.x - (bodyRect.x + Padding.left);
        if (relX <= 0.0f)
            return line.start;

        for (int32_t i = line.start; i < line.end; ++i)
        {
            const std::string before = Text.substr(static_cast<size_t>(line.start), static_cast<size_t>(i - line.start));
            const std::string withChar = Text.substr(static_cast<size_t>(line.start), static_cast<size_t>(i - line.start + 1));
            const float mid = (MeasureText(before, 0.0f, fontSize).x + MeasureText(withChar, 0.0f, fontSize).x) * 0.5f;
            if (relX < mid)
                return i;
        }
        return line.end;
    }

    void TextBox::RenderOverride(ImDrawList& drawList)
    {
        ClampEditingState();
        VisualState state = GetVisualState();
        const float fontSize = EffectiveFontSize();
        const float headerHeight = Header.empty() ? 0.0f : 20.0f;
        const float headerGap = Header.empty() ? 0.0f : 6.0f;
        if (!Header.empty())
            drawList.AddText(GetDefaultFont(), fontSize, { Bounds.x, Bounds.y }, Foreground.ToU32(Opacity), Header.c_str(), nullptr, Bounds.width);

        Rect bodyRect = BodyRect();
        if (HasStyledPartVisual(Style.PART_Root))
            DrawStyledPart(drawList, Bounds, Style.PART_Root, state);

        ControlPartStyle background = Style.PART_Background;
        if (!HasStyledPartVisual(background))
        {
            background.BackgroundColor = IsPressed ? BackgroundPressed : (IsHovered ? BackgroundHover : BackgroundNormal);
            background.BorderColor = IsFocused() ? ColorFromBytes(0, 120, 212, 255) : BorderBrush;
            background.BorderThickness = BorderThickness.left;
            background.CornerRadius = CornerRadius;
        }
        background = ApplyStateTransition(background);
        DrawStyledPart(drawList, bodyRect, background, state);
        DrawStyledPart(drawList, bodyRect, Style.PART_Border, state);

        const bool empty = Text.empty();
        const std::string& visibleText = empty ? PlaceholderText : Text;
        ControlPartStyle textPart = empty ? Style.PART_PlaceholderText : Style.PART_TextPresenter;
        Color textColor = ResolvePartForeground(textPart, empty ? Color { 0.55f, 0.58f, 0.64f, 1.0f } : Foreground);
        const float wrapWidth = TextWrapping ? std::max(0.0f, bodyRect.width - Padding.Horizontal()) : 0.0f;
        const float lineHeight = LineHeight(fontSize);
        std::vector<TextLine> lines = BuildTextLines(visibleText, wrapWidth, fontSize);
        const float contentHeight = std::max(lineHeight, lineHeight * static_cast<float>(lines.size()));
        if (AcceptsReturn)
        {
            const float viewportHeight = std::max(0.0f, bodyRect.height - Padding.Vertical());
            m_textScrollY = std::clamp(m_textScrollY, 0.0f, std::max(0.0f, contentHeight - viewportHeight));
        }
        else
        {
            m_textScrollY = 0.0f;
        }
        const float textY = AcceptsReturn ? bodyRect.y + Padding.top - m_textScrollY : bodyRect.y + std::floor((bodyRect.height - contentHeight) * 0.5f);
        const Vec2 pos(bodyRect.x + Padding.left, textY);
        drawList.PushClipRect(Vec2(bodyRect.Left(), bodyRect.Top()), Vec2(bodyRect.Right(), bodyRect.Bottom()), true);
        DrawStyledPart(drawList, bodyRect, Style.PART_TextPresenter, state);

        if (!empty && HasSelection())
        {
            const int32_t selectionBegin = SelectionBegin();
            const int32_t selectionEnd = SelectionEnd();
            const Color selectionColor = ColorFromBytes(0, 120, 212, 72);
            for (size_t i = 0; i < lines.size(); ++i)
            {
                const TextLine& line = lines[i];
                const int32_t begin = std::max(selectionBegin, line.start);
                const int32_t end = std::min(selectionEnd, line.end);
                if (end <= begin)
                    continue;
                const std::string before = Text.substr(static_cast<size_t>(line.start), static_cast<size_t>(begin - line.start));
                const std::string selected = Text.substr(static_cast<size_t>(begin), static_cast<size_t>(end - begin));
                const float x0 = pos.x + MeasureText(before, 0.0f, fontSize).x;
                const float x1 = x0 + std::max(2.0f, MeasureText(selected, 0.0f, fontSize).x);
                const float y = pos.y + lineHeight * static_cast<float>(i);
                drawList.AddRectFilled({ x0, y + 1.0f }, { x1, y + lineHeight - 1.0f }, selectionColor.ToU32(Opacity), 2.0f);
            }
        }

        for (size_t i = 0; i < lines.size(); ++i)
        {
            const TextLine& line = lines[i];
            if (line.text.empty())
                continue;
            drawList.AddText(GetDefaultFont(), fontSize, { pos.x, pos.y + lineHeight * static_cast<float>(i) }, textColor.ToU32(Opacity), line.text.c_str());
        }

        if (IsFocused() && !IsReadOnly)
        {
            ControlPartStyle caret = Style.PART_Caret;
            if (!HasStyledPartVisual(caret))
                caret.BackgroundColor = Foreground;
            std::vector<TextLine> textLines = BuildTextLines(Text, wrapWidth, fontSize);
            size_t caretLineIndex = 0;
            for (size_t i = 0; i < textLines.size(); ++i)
            {
                if (m_caretIndex >= textLines[i].start && m_caretIndex <= textLines[i].end)
                {
                    caretLineIndex = i;
                    break;
                }
            }
            const TextLine& caretLine = textLines[caretLineIndex];
            const int32_t caretColumn = std::clamp(m_caretIndex - caretLine.start, 0, static_cast<int32_t>(caretLine.text.size()));
            const std::string caretPrefix = caretLine.text.substr(0, static_cast<size_t>(caretColumn));
            const float caretX = std::floor(std::min(bodyRect.Right() - Padding.right - 1.0f, pos.x + MeasureText(caretPrefix, 0.0f, fontSize).x + 1.0f)) + 0.5f;
            const float caretTop = std::floor(pos.y + lineHeight * static_cast<float>(caretLineIndex) + 2.0f);
            const float caretBottom = std::min(bodyRect.Bottom() - Padding.bottom, caretTop + std::max(1.0f, lineHeight - 4.0f));
            const Color caretColor = caret.BackgroundColor.a > 0.0f ? caret.BackgroundColor : ResolvePartForeground(caret, Foreground);
            if (std::fmod(static_cast<float>(GetTimeSeconds()), 1.0f) < 0.55f)
                drawList.AddLine({ caretX, caretTop }, { caretX, caretBottom }, caretColor.ToU32(Opacity), 1.0f);
        }

        drawList.PopClipRect();
        if (IsFocused() && GetFocusVisualsEnabled())
        {
            if (HasStyledPartVisual(Style.PART_FocusVisual))
                DrawStyledPart(drawList, bodyRect, Style.PART_FocusVisual, VisualState::Focused);
            else
                drawList.AddRect({ bodyRect.Left() + 0.5f, bodyRect.Top() + 0.5f }, { bodyRect.Right() - 0.5f, bodyRect.Bottom() - 0.5f }, ColorFromBytes(0, 120, 212, 255), CornerRadius, ImDrawFlags_None, 1.5f);
        }
    }

    void TextBox::OnEvent(EventArgs& args)
    {
        if (args.kind == EventKind::GotFocus)
        {
            if (OnGotFocus)
                OnGotFocus();
        }
        else if (args.kind == EventKind::LostFocus)
        {
            m_isSelecting = false;
            IsPressed = false;
            if (OnLostFocus)
                OnLostFocus();
        }
        else if (args.kind == EventKind::PointerEntered)
            IsHovered = true;
        else if (args.kind == EventKind::PointerExited)
            IsHovered = false;
        else if (args.kind == EventKind::PointerPressed)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            const Rect body = BodyRect();
            if (!body.Contains(pointer.position))
                return;
            IsPressed = true;
            const int32_t hitIndex = HitTestTextIndex(pointer.position, body, EffectiveFontSize());
            SetCaretIndex(hitIndex, false);
            m_isSelecting = true;
            CapturePointer(pointer.pointerId);
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerMoved && m_isSelecting)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            SetCaretIndex(HitTestTextIndex(pointer.position, BodyRect(), EffectiveFontSize()), true);
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerReleased)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            IsPressed = false;
            m_isSelecting = false;
            ReleasePointer(pointer.pointerId);
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerWheelChanged && AcceptsReturn)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            const Rect body = BodyRect();
            const float fontSize = EffectiveFontSize();
            const float wrapWidth = TextWrapping ? std::max(0.0f, body.width - Padding.Horizontal()) : 0.0f;
            const std::vector<TextLine> lines = BuildTextLines(Text.empty() ? PlaceholderText : Text, wrapWidth, fontSize);
            const float lineHeight = LineHeight(fontSize);
            const float contentHeight = std::max(lineHeight, lineHeight * static_cast<float>(lines.size()));
            const float viewportHeight = std::max(0.0f, body.height - Padding.Vertical());
            const float maxScroll = std::max(0.0f, contentHeight - viewportHeight);
            if (maxScroll > 0.0f)
            {
                m_textScrollY = std::clamp(m_textScrollY - pointer.wheelDelta.y * 30.0f, 0.0f, maxScroll);
                args.handled = true;
            }
        }
        else if (!IsReadOnly && IsFocused() && args.kind == EventKind::TextInput)
        {
            auto& text = static_cast<TextInputEventArgs&>(args);
            const bool isNewLine = AcceptsReturn && (text.codepoint == 10 || text.codepoint == 13);
            if ((text.codepoint >= 32 && text.codepoint < 127) || isNewLine)
            {
                if (MaxLength >= 0 && !HasSelection() && static_cast<int32_t>(Text.size()) >= MaxLength)
                {
                    args.handled = true;
                    return;
                }
                const char ch = isNewLine ? '\n' : static_cast<char>(text.codepoint);
                ReplaceSelection(std::string_view(&ch, 1));
                args.handled = true;
            }
        }
        else if (!IsReadOnly && IsFocused() && args.kind == EventKind::KeyDown)
        {
            auto& key = static_cast<KeyEventArgs&>(args);
            const bool shift = (static_cast<uint32_t>(key.modifiers) & static_cast<uint32_t>(KeyModifiers::Shift)) != 0;
            const bool control = (static_cast<uint32_t>(key.modifiers) & static_cast<uint32_t>(KeyModifiers::Control)) != 0;
            if (control && key.key == Key::A)
            {
                const int32_t oldStart = SelectionStart;
                const int32_t oldLength = SelectionLength;
                m_selectionAnchor = 0;
                m_caretIndex = TextLength();
                CaretIndex = m_caretIndex;
                SelectionStart = 0;
                SelectionLength = TextLength();
                RaiseSelectionChangedIfNeeded(oldStart, oldLength);
                args.handled = true;
            }
            else if (key.key == Key::Backspace)
            {
                if (!DeleteSelection() && m_caretIndex > 0)
                {
                    std::string next = Text;
                    next.erase(static_cast<size_t>(m_caretIndex - 1), 1);
                    SetText(std::move(next));
                    SetCaretIndex(m_caretIndex - 1, false);
                }
                args.handled = true;
            }
            else if (key.key == Key::DeleteKey)
            {
                if (!DeleteSelection() && m_caretIndex < TextLength())
                {
                    std::string next = Text;
                    next.erase(static_cast<size_t>(m_caretIndex), 1);
                    SetText(std::move(next));
                    SetCaretIndex(m_caretIndex, false);
                }
                args.handled = true;
            }
            else if (key.key == Key::Enter && AcceptsReturn)
            {
                ReplaceSelection("\n");
                args.handled = true;
            }
            else if (key.key == Key::Enter)
            {
                if (OnSubmit)
                    OnSubmit();
                EventArgs submit;
                submit.kind = EventKind::NavigationSubmit;
                submit.route = RoutingStrategy::Bubble;
                submit.originalSource = this;
                RaiseEvent(submit);
                args.handled = true;
            }
            else if (key.key == Key::Escape)
            {
                if (OnCancel)
                    OnCancel();
                EventArgs cancel;
                cancel.kind = EventKind::NavigationCancel;
                cancel.route = RoutingStrategy::Bubble;
                cancel.originalSource = this;
                RaiseEvent(cancel);
                args.handled = true;
            }
            else if (key.key == Key::Left)
            {
                SetCaretIndex(m_caretIndex - 1, shift);
                args.handled = true;
            }
            else if (key.key == Key::Right)
            {
                SetCaretIndex(m_caretIndex + 1, shift);
                args.handled = true;
            }
            else if (key.key == Key::Home)
            {
                SetCaretIndex(0, shift);
                args.handled = true;
            }
            else if (key.key == Key::End)
            {
                SetCaretIndex(TextLength(), shift);
                args.handled = true;
            }
            else if ((key.key == Key::Up || key.key == Key::Down) && AcceptsReturn)
            {
                Rect body = BodyRect();
                const float fontSize = EffectiveFontSize();
                const float wrapWidth = TextWrapping ? std::max(0.0f, body.width - Padding.Horizontal()) : 0.0f;
                std::vector<TextLine> lines = BuildTextLines(Text, wrapWidth, fontSize);
                size_t lineIndex = 0;
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    if (m_caretIndex >= lines[i].start && m_caretIndex <= lines[i].end)
                    {
                        lineIndex = i;
                        break;
                    }
                }
                const TextLine& line = lines[lineIndex];
                const int32_t column = std::clamp(m_caretIndex - line.start, 0, static_cast<int32_t>(line.text.size()));
                const float caretX = body.x + Padding.left + MeasureText(line.text.substr(0, static_cast<size_t>(column)), 0.0f, fontSize).x;
                const float targetLine = key.key == Key::Up ? std::max<int32_t>(0, static_cast<int32_t>(lineIndex) - 1) : std::min<int32_t>(static_cast<int32_t>(lines.size()) - 1, static_cast<int32_t>(lineIndex) + 1);
                const float textTop = body.y + Padding.top;
                const float y = textTop + LineHeight(fontSize) * targetLine + LineHeight(fontSize) * 0.5f;
                SetCaretIndex(HitTestTextIndex({ caretX, y }, body, fontSize), shift);
                args.handled = true;
            }
        }
    }

    float TextBox::EffectiveFontSize() const
    {
        return FontSize > 0.0f ? FontSize : GetFontSize();
    }
}

// ---- End inlined TextBox control ----


namespace FyGUI
{

    class Image : public UIElement
    {
    public:
        ImageSource Source;
        Color Tint = { 1.0f, 1.0f, 1.0f, 1.0f };
        ImageStyle Style;

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            Vec2 size = Source.size;
            if (Width >= 0.0f)
                size.x = Width;
            if (Height >= 0.0f)
                size.y = Height;
            if (size.x <= 0.0f)
                size.x = std::min(availableSize.x, 64.0f);
            if (size.y <= 0.0f)
                size.y = std::min(availableSize.y, 64.0f);
            return size;
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            VisualState state = GetVisualState();
            if (HasStyledPartVisual(Style.PART_Root))
                DrawStyledPart(drawList, Bounds, Style.PART_Root, state, Tint);

            if (HasStyledPartVisual(Style.PART_Image))
            {
                ControlPartStyle imageStyle = Style.PART_Image;
                if (!imageStyle.Image)
                    imageStyle.Image = Source.texture;
                imageStyle.UseImage = imageStyle.UseImage || imageStyle.Image != 0;
                DrawStyledPart(drawList, Bounds, imageStyle, state, Tint);
            }
            else if (Source.texture)
            {
                drawList.AddImage(Source.texture,
                    Vec2(Bounds.Left(), Bounds.Top()),
                    Vec2(Bounds.Right(), Bounds.Bottom()),
                    Source.uv0,
                    Source.uv1,
                    Tint.ToU32(Opacity));
            }

            if (HasStyledPartVisual(Style.PART_Border))
                DrawStyledPart(drawList, Bounds, Style.PART_Border, state);
            if (HasStyledPartVisual(Style.PART_Overlay))
                DrawStyledPart(drawList, Bounds, Style.PART_Overlay, state, Tint);
        }
    };

}


// ---- Begin inlined Button control ----
// Button is inlined after
// Control, ButtonStyle and the shared retained-UI helpers are declared.
namespace FyGUI
{
    class Button : public Control
    {
    public:
        std::string Content;
        std::function<void()> OnClick;
        ButtonStyle Style;
        TextureId IconTexture = 0;
        FyGUI::HorizontalAlignment HorizontalContentAlignment = FyGUI::HorizontalAlignment::Center;
        FyGUI::VerticalAlignment VerticalContentAlignment = FyGUI::VerticalAlignment::Center;

        explicit Button(std::string text = {});

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override;
        void RenderOverride(ImDrawList& drawList) override;
        void OnEvent(EventArgs& args) override;
        bool OnAction(UIAction action) override;
    };
}

namespace FyGUI
{
    Button::Button(std::string text) : Content(std::move(text))
    {
        IsTabStop = true;
        Padding = Thickness(14.0f, 8.0f, 14.0f, 8.0f);
        BorderThickness = Thickness(1.0f);
        CornerRadius = 4.0f;
        BackgroundNormal = ColorFromBytes(255, 255, 255);
        BackgroundHover = ColorFromBytes(243, 242, 241);
        BackgroundPressed = ColorFromBytes(237, 235, 233);
        BorderBrush = ColorFromBytes(138, 136, 134);
        Foreground = ColorFromBytes(32, 31, 30);
        ProtectedCursor = Cursor::Hand;
        VerticalAlignment = FyGUI::VerticalAlignment::Top;
    }

    Vec2 Button::MeasureOverride(Vec2)
    {
        Vec2 textSize = MeasureText(Content);
        const float iconAdvance = IconTexture || HasStyledPartVisual(Style.PART_Icon) ? textSize.y + 8.0f : 0.0f;
        return { textSize.x + iconAdvance + Padding.Horizontal(), textSize.y + Padding.Vertical() };
    }

    void Button::RenderOverride(ImDrawList& drawList)
    {
        VisualState state = GetVisualState();
        const Rect visualBounds = AnimatedVisualBounds();
        if (HasStyledPartVisual(Style.PART_Root) || HasStyledPartVisual(Style.PART_Background) || HasStyledPartVisual(Style.PART_Border))
        {
            DrawStyledPart(drawList, visualBounds, Style.PART_Root, state);
            ControlPartStyle background = Style.PART_Background;
            if (!HasStyledPartVisual(background))
            {
                background.BackgroundColor = BackgroundNormal;
                background.CornerRadius = CornerRadius;
                background.Opacity = Opacity;
            }
            background = ApplyStateTransition(background);
            DrawStyledPart(drawList, visualBounds, background, state);

            ControlPartStyle border = Style.PART_Border;
            if (!HasStyledPartVisual(border))
            {
                border.BorderColor = BorderBrush;
                border.BorderThickness = BorderThickness.left;
                border.CornerRadius = CornerRadius;
                border.Opacity = Opacity;
            }
            border = ApplyStateTransition(border);
            DrawStyledPart(drawList, visualBounds, border, state);
        }
        else
        {
            Control::RenderOverride(drawList);
        }

        Vec2 textSize = MeasureText(Content);
        const bool hasIcon = IconTexture || HasStyledPartVisual(Style.PART_Icon);
        const float iconSize = hasIcon ? std::max(0.0f, Bounds.height - Padding.Vertical()) : 0.0f;
        const float iconGap = hasIcon && !Content.empty() ? 8.0f : 0.0f;
        const float groupWidth = iconSize + iconGap + textSize.x;
        const float contentX = Bounds.x + Padding.left;
        const float contentY = Bounds.y + Padding.top;
        const float contentWidth = std::max(0.0f, Bounds.width - Padding.Horizontal());
        const float contentHeight = std::max(0.0f, Bounds.height - Padding.Vertical());
        float groupX = contentX;
        if (HorizontalContentAlignment == FyGUI::HorizontalAlignment::Center || HorizontalContentAlignment == FyGUI::HorizontalAlignment::Stretch)
            groupX = contentX + (contentWidth - groupWidth) * 0.5f;
        else if (HorizontalContentAlignment == FyGUI::HorizontalAlignment::Right)
            groupX = contentX + contentWidth - groupWidth;

        if (hasIcon)
        {
            Rect iconRect = { groupX, Bounds.y + (Bounds.height - iconSize) * 0.5f, iconSize, iconSize };
            ControlPartStyle icon = Style.PART_Icon;
            if (!icon.Image)
                icon.Image = IconTexture;
            icon.UseImage = icon.UseImage || icon.Image != 0;
            DrawStyledPart(drawList, iconRect, icon, state);
        }

        Color textColor = ResolvePartForeground(Style.PART_Text, Foreground);
        Vec2 pos(groupX + iconSize + iconGap, contentY);
        if (VerticalContentAlignment == FyGUI::VerticalAlignment::Center || VerticalContentAlignment == FyGUI::VerticalAlignment::Stretch)
            pos.y = contentY + (contentHeight - textSize.y) * 0.5f;
        else if (VerticalContentAlignment == FyGUI::VerticalAlignment::Bottom)
            pos.y = contentY + contentHeight - textSize.y;
        drawList.AddText(pos, textColor.ToU32(Opacity), Content.c_str());

        if (IsFocused() && HasStyledPartVisual(Style.PART_FocusVisual))
            DrawStyledPart(drawList, visualBounds, Style.PART_FocusVisual, VisualState::Focused);
    }

    void Button::OnEvent(EventArgs& args)
    {
        if (args.kind == EventKind::PointerEntered)
            IsHovered = true;
        else if (args.kind == EventKind::PointerExited)
            IsHovered = false;
        else if (args.kind == EventKind::PointerPressed)
        {
            IsPressed = true;
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerReleased)
        {
            if (IsPressed && OnClick)
                OnClick();

            IsPressed = false;
            ClickEventArgs click;
            click.kind = EventKind::Click;
            click.route = RoutingStrategy::Bubble;
            click.originalSource = this;
            RaiseEvent(click);
            args.handled = true;
        }
    }

    bool Button::OnAction(UIAction action)
    {
        if (action != UIAction::Accept)
            return false;

        if (OnClick)
            OnClick();

        ClickEventArgs click;
        click.kind = EventKind::Click;
        click.route = RoutingStrategy::Bubble;
        click.originalSource = this;
        RaiseEvent(click);
        return true;
    }
}

// ---- End inlined Button control ----


namespace FyGUI
{

    class ToggleButton : public Button
    {
    public:
        CheckedState State = CheckedState::Unchecked;
        std::function<void(CheckedState)> OnCheckedChanged;

        VisualState GetVisualState() const override
        {
            if (!IsEnabled)
                return VisualState::Disabled;
            if (IsPressed)
                return VisualState::Pressed;
            if (State == CheckedState::Checked)
                return VisualState::Checked;
            if (IsHovered)
                return VisualState::PointerOver;
            if (IsFocused())
                return VisualState::Focused;
            return VisualState::Normal;
        }

        explicit ToggleButton(std::string text = {}) : Button(std::move(text)) {}

        void SetState(CheckedState state)
        {
            if (State == state)
                return;
            CheckedState old = State;
            State = state;
            if (OnCheckedChanged)
                OnCheckedChanged(State);

            CheckedChangedEventArgs args;
            args.kind = EventKind::CheckedChanged;
            args.route = RoutingStrategy::Bubble;
            args.originalSource = this;
            args.oldState = old;
            args.newState = State;
            RaiseEvent(args);
        }

    protected:
        void OnEvent(EventArgs& args) override
        {
            if (args.kind == EventKind::PointerReleased && IsPressed)
                SetState(State == CheckedState::Checked ? CheckedState::Unchecked : CheckedState::Checked);
            Button::OnEvent(args);
        }

        bool OnAction(UIAction action) override
        {
            if (action != UIAction::Accept)
                return Button::OnAction(action);
            SetState(State == CheckedState::Checked ? CheckedState::Unchecked : CheckedState::Checked);
            return true;
        }
    };

    class CheckBox : public ToggleButton
    {
    public:
        bool IsThreeState = false;
        CheckBoxStyle CheckStyle;

        explicit CheckBox(std::string text = {}) : ToggleButton(std::move(text))
        {
            Padding = Thickness(28.0f, 6.0f, 10.0f, 6.0f);
            BackgroundNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
            BackgroundHover = ColorFromBytes(243, 242, 241, 180);
            BackgroundPressed = ColorFromBytes(237, 235, 233, 220);
            BorderBrush = { 0.0f, 0.0f, 0.0f, 0.0f };
            Foreground = ColorFromBytes(32, 31, 30);
        }

    protected:
        void RenderOverride(ImDrawList& drawList) override
        {
            VisualState state = GetVisualState();
            if (HasStyledPartVisual(CheckStyle.PART_Root))
                DrawStyledPart(drawList, Bounds, CheckStyle.PART_Root, state);
            else
                Control::RenderOverride(drawList);

            const float box = std::min(18.0f, Bounds.height - 4.0f);
            const float x = Bounds.x + 4.0f;
            const float y = Bounds.y + (Bounds.height - box) * 0.5f;
            Rect boxRect = { x, y, box, box };
            if (HasStyledPartVisual(CheckStyle.PART_Box))
                DrawStyledPart(drawList, boxRect, CheckStyle.PART_Box, state);
            else
                drawList.AddRect(Vec2{ x, y }, Vec2{ x + box, y + box }, ColorFromBytes(160, 170, 190, 255), 3.0f, ImDrawFlags_None, 1.0f);

            if (State == CheckedState::Checked)
            {
                if (HasStyledPartVisual(CheckStyle.PART_CheckMark))
                    DrawStyledPart(drawList, boxRect, CheckStyle.PART_CheckMark, VisualState::Checked);
                else
                {
                    drawList.AddRectFilled(Vec2{ x, y }, Vec2{ x + box, y + box }, ColorFromBytes(0, 120, 212, 255), 3.0f);
                    drawList.AddLine(Vec2{ x + 4.0f, y + box * 0.55f }, Vec2{ x + box * 0.42f, y + box - 4.0f }, ColorFromBytes(240, 245, 255, 255), 2.0f);
                    drawList.AddLine(Vec2{ x + box * 0.42f, y + box - 4.0f }, Vec2{ x + box - 4.0f, y + 4.0f }, ColorFromBytes(240, 245, 255, 255), 2.0f);
                }
            }
            Vec2 textSize = MeasureText(Content);
            Color textColor = ResolvePartForeground(CheckStyle.PART_Text, Foreground);
            drawList.AddText(Vec2(Bounds.x + 28.0f, Bounds.y + (Bounds.height - textSize.y) * 0.5f), textColor.ToU32(Opacity), Content.c_str());

            if (IsFocused() && HasStyledPartVisual(CheckStyle.PART_FocusVisual))
                DrawStyledPart(drawList, Bounds, CheckStyle.PART_FocusVisual, VisualState::Focused);
        }
    };

    class RadioButton : public ToggleButton
    {
    public:
        std::string GroupName;

        RadioButton(std::string text = {}, std::string group = {}) : ToggleButton(std::move(text)), GroupName(std::move(group))
        {
            Padding = Thickness(28.0f, 6.0f, 10.0f, 6.0f);
            BackgroundNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
            BackgroundHover = ColorFromBytes(243, 242, 241, 180);
            BackgroundPressed = ColorFromBytes(237, 235, 233, 220);
            BorderBrush = { 0.0f, 0.0f, 0.0f, 0.0f };
            Foreground = ColorFromBytes(32, 31, 30);
        }

    protected:
        void OnEvent(EventArgs& args) override
        {
            if (args.kind == EventKind::PointerReleased && IsPressed)
            {
                UncheckGroupPeers();
                SetState(CheckedState::Checked);
            }
            Button::OnEvent(args);
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            Control::RenderOverride(drawList);
            const float radius = std::min(8.0f, (Bounds.height - 4.0f) * 0.5f);
            const Vec2 center(Bounds.x + 4.0f + radius, Bounds.y + Bounds.height * 0.5f);
            const Color accent = BorderBrush.a > 0.0f ? BorderBrush : ColorFromBytes(0, 120, 212, 255);
            const Color ring = State == CheckedState::Checked ? accent : ColorFromBytes(118, 118, 118, 255);
            drawList.AddCircle(center, radius, ring, 24, 1.35f);
            if (State == CheckedState::Checked)
                drawList.AddCircleFilled(center, radius * 0.50f, accent, 24);
            Vec2 textSize = MeasureText(Content);
            drawList.AddText(Vec2(Bounds.x + 28.0f, Bounds.y + (Bounds.height - textSize.y) * 0.5f), Foreground.ToU32(Opacity), Content.c_str());
        }

    private:
        void UncheckGroupPeers()
        {
            UIElement* parent = GetParent();
            if (!parent)
                return;

            auto* panel = dynamic_cast<Panel*>(parent);
            if (!panel)
                return;

            for (auto& child : panel->Children)
            {
                auto* radio = dynamic_cast<RadioButton*>(child.get());
                if (!radio || radio == this)
                    continue;
                if (radio->GroupName == GroupName)
                    radio->SetState(CheckedState::Unchecked);
            }
        }
    };

    class RadioButtons : public StackPanel
    {
    public:
        std::string Header;
        std::vector<std::string> Items;
        int32_t SelectedIndex = -1;
        std::string SelectedItem;
        int32_t MaxColumns = 1;
        float ItemSpacing = 12.0f;
        std::function<void(int32_t)> OnSelectionChanged;

        RadioButtons()
        {
            Orientation = FyGUI::Orientation::Vertical;
            Spacing = 8.0f;
        }

        void AddOption(std::string option)
        {
            Items.push_back(std::move(option));
            RefreshItems();
        }

        void SetSelectedIndex(int32_t index)
        {
            const int32_t clamped = Items.empty() ? -1 : std::clamp(index, 0, static_cast<int32_t>(Items.size()) - 1);
            if (SelectedIndex == clamped)
                return;

            const int32_t old = SelectedIndex;
            SelectedIndex = clamped;
            SelectedItem = SelectedIndex >= 0 ? Items[static_cast<size_t>(SelectedIndex)] : std::string{};
            UpdateRadioStates();

            if (OnSelectionChanged)
                OnSelectionChanged(SelectedIndex);

            SelectionChangedEventArgs args;
            args.kind = EventKind::SelectionChanged;
            args.route = RoutingStrategy::Bubble;
            args.originalSource = this;
            args.oldIndex = old;
            args.newIndex = SelectedIndex;
            RaiseEvent(args);
        }

        void RefreshItems()
        {
            Children.clear();
            m_optionButtons.clear();

            if (!Header.empty())
            {
                auto header = std::make_shared<TextBlock>(Header);
                header->Foreground = Foreground;
                header->TextWrapping = TextWrappingMode::Wrap;
                header->IsHitTestVisible = false;
                AddChild(header);
            }

            std::shared_ptr<Panel> host;
            if (MaxColumns > 1)
            {
                auto wrap = std::make_shared<WrapPanel>();
                wrap->Orientation = FyGUI::Orientation::Horizontal;
                wrap->Spacing = ItemSpacing;
                wrap->LineSpacing = 6.0f;
                wrap->Width = Width >= 0.0f ? Width : AutoSize;
                host = wrap;
            }
            else
            {
                auto stack = std::make_shared<StackPanel>();
                stack->Orientation = FyGUI::Orientation::Vertical;
                stack->Spacing = 6.0f;
                host = stack;
            }

            const std::string groupName = "RadioButtonsGroup_" + std::to_string(reinterpret_cast<std::uintptr_t>(this));
            for (size_t i = 0; i < Items.size(); ++i)
            {
                auto radio = std::make_shared<RadioButton>(Items[i], groupName);
                radio->Width = MaxColumns > 1 ? 110.0f : (Width >= 0.0f ? Width : 180.0f);
                radio->Height = 30.0f;
                radio->Foreground = Foreground;
                radio->OnCheckedChanged = [this, index = static_cast<int32_t>(i)](CheckedState state)
                {
                    if (state == CheckedState::Checked)
                        SetSelectedIndex(index);
                };
                m_optionButtons.push_back(radio);
                host->AddChild(radio);
            }

            AddChild(host);
            if (Items.empty())
                SelectedIndex = -1;
            else if (SelectedIndex >= 0)
                SelectedIndex = std::clamp(SelectedIndex, 0, static_cast<int32_t>(Items.size()) - 1);
            SelectedItem = SelectedIndex >= 0 ? Items[static_cast<size_t>(SelectedIndex)] : std::string{};
            UpdateRadioStates();
        }

    private:
        std::vector<std::weak_ptr<RadioButton>> m_optionButtons;

        void UpdateRadioStates()
        {
            for (size_t i = 0; i < m_optionButtons.size(); ++i)
            {
                if (auto radio = m_optionButtons[i].lock())
                    radio->State = static_cast<int32_t>(i) == SelectedIndex ? CheckedState::Checked : CheckedState::Unchecked;
            }
        }
    };

    class ToggleSwitch : public ToggleButton
    {
    public:
        std::string Header;
        std::string OnContent = "On";
        std::string OffContent = "Off";
        ToggleSwitchStyle ToggleStyle;

        ToggleSwitch()
        {
            Width = 220.0f;
            Height = AutoSize;
            Padding = Thickness(0.0f);
            BackgroundNormal = ColorFromBytes(0, 0, 0, 0);
            BackgroundHover = BackgroundNormal;
            BackgroundPressed = BackgroundNormal;
            BorderBrush = ColorFromBytes(0, 0, 0, 0);
            Foreground = ColorFromBytes(32, 31, 30);
            ProtectedCursor = Cursor::Hand;
            IsTabStop = true;
            UseSystemFocusVisuals = false;
        }

    protected:
        Vec2 MeasureOverride(Vec2) override
        {
            const float headerHeight = Header.empty() ? 0.0f : 22.0f;
            const std::string& label = State == CheckedState::Checked ? OnContent : OffContent;
            Vec2 text = MeasureText(label);
            return { Width >= 0.0f ? Width : 60.0f + text.x + 10.0f, headerHeight + 32.0f };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            DrawStyledPart(drawList, Bounds, ToggleStyle.PART_Root, GetVisualState());
            const float headerHeight = Header.empty() ? 0.0f : 22.0f;
            if (!Header.empty())
            {
                const Color headerColor = ToggleStyle.PART_HeaderText.ForegroundColor.a > 0.0f ? ToggleStyle.PART_HeaderText.ForegroundColor : Foreground;
                drawList.AddText(GetDefaultFont(), GetFontSize(), { Bounds.x, Bounds.y }, headerColor.ToU32(Opacity), Header.c_str(), nullptr, Bounds.width);
            }

            const bool on = State == CheckedState::Checked;
            Rect track = { Bounds.x, Bounds.y + headerHeight + 4.0f, 40.0f, 20.0f };
            ControlPartStyle trackStyle = ToggleStyle.PART_Track;
            if (!HasStyledPartVisual(trackStyle))
                trackStyle = SolidPart(on ? ColorFromBytes(0, 120, 212) : ColorFromBytes(118, 118, 118), Color{ 0, 0, 0, 0 }, 0.0f, 10.0f);
            else if (on && trackStyle.SelectedImage == 0 && trackStyle.CheckedImage == 0)
                trackStyle.BackgroundColor = ColorFromBytes(0, 120, 212);
            DrawStyledPart(drawList, track, trackStyle, on ? VisualState::Checked : GetVisualState());
            const float knobX = on ? track.Right() - 16.0f : track.x + 4.0f;
            Rect thumb = { knobX, track.y + 4.0f, 12.0f, 12.0f };
            if (HasStyledPartVisual(ToggleStyle.PART_Thumb))
                DrawStyledPart(drawList, thumb, ToggleStyle.PART_Thumb, on ? VisualState::Checked : GetVisualState());
            else
                drawList.AddCircleFilled({ knobX + 6.0f, track.y + 10.0f }, 6.0f, ColorFromBytes(255, 255, 255), 20);

            const std::string& label = on ? OnContent : OffContent;
            Vec2 labelSize = MeasureText(label);
            const Color textColor = ToggleStyle.PART_Text.ForegroundColor.a > 0.0f ? ToggleStyle.PART_Text.ForegroundColor : Foreground;
            drawList.AddText({ track.Right() + 10.0f, track.y + (track.height - labelSize.y) * 0.5f }, textColor.ToU32(Opacity), label.c_str());
            if (IsFocused() && GetFocusVisualsEnabled())
            {
                Rect focus = { track.Left() - 2.0f, track.Top() - 2.0f, track.width + 4.0f, track.height + 4.0f };
                if (HasStyledPartVisual(ToggleStyle.PART_FocusVisual))
                    DrawStyledPart(drawList, focus, ToggleStyle.PART_FocusVisual, VisualState::Focused);
                else
                    drawList.AddRect({ focus.Left(), focus.Top() }, { focus.Right(), focus.Bottom() }, ColorFromBytes(0, 120, 212, 210), 12.0f, ImDrawFlags_None, 1.2f);
            }
        }
    };

    class InfoBar : public Border
    {
    public:
        std::string Title;
        std::string Message;
        std::string Severity = "Informational";
        bool IsOpen = true;
        bool IsClosable = true;
        InfoBarStyle Style;
        std::function<void()> OnCloseButtonClick;

        InfoBar()
        {
            Width = 640.0f;
            Padding = Thickness(14.0f, 10.0f, 14.0f, 10.0f);
            CornerRadius = 6.0f;
            BorderThickness = Thickness(1.0f);
            Background = ColorFromBytes(243, 249, 253);
            BorderBrush = ColorFromBytes(199, 224, 244);
            Foreground = ColorFromBytes(32, 31, 30);
            IsTabStop = false;
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            if (!IsOpen)
                return {};
            const float textWidth = Width >= 0.0f ? Width - Padding.Horizontal() - 46.0f : std::min(availableSize.x, 640.0f) - Padding.Horizontal() - 46.0f;
            Vec2 title = Title.empty() ? Vec2{} : MeasureText(Title, textWidth);
            Vec2 message = Message.empty() ? Vec2{} : MeasureText(Message, textWidth);
            const float contentHeight = std::max(22.0f, title.y + (Title.empty() || Message.empty() ? 0.0f : 2.0f) + message.y);
            return { Width >= 0.0f ? Width : std::min(availableSize.x, 640.0f), contentHeight + Padding.Vertical() };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            if (!IsOpen)
                return;
            const Color accent = SeverityColor();
            ControlPartStyle background = Style.PART_Background;
            if (!HasStyledPartVisual(background))
                background = SolidPart(SeverityBackground(), SeverityBorder(), BorderThickness.left, CornerRadius);
            DrawStyledPart(drawList, Bounds, Style.PART_Root, GetVisualState());
            DrawStyledPart(drawList, Bounds, background, GetVisualState());
            Rect iconRect = { Bounds.x + Padding.left, Bounds.y + Padding.top + 3.0f, 14.0f, 14.0f };
            if (HasStyledPartVisual(Style.PART_Icon))
                DrawStyledPart(drawList, iconRect, Style.PART_Icon, GetVisualState());
            else
                drawList.AddCircleFilled({ iconRect.x + 7.0f, iconRect.y + 7.0f }, 7.0f, accent, 20);
            float x = Bounds.x + Padding.left + 28.0f;
            float y = Bounds.y + Padding.top;
            const float textWidth = std::max(0.0f, Bounds.width - Padding.Horizontal() - 46.0f);
            const Color titleColor = Style.PART_TitleText.ForegroundColor.a > 0.0f ? Style.PART_TitleText.ForegroundColor : Foreground;
            const Color messageColor = Style.PART_MessageText.ForegroundColor.a > 0.0f ? Style.PART_MessageText.ForegroundColor : Foreground;
            if (!Title.empty())
            {
                drawList.AddText(GetDefaultFont(), GetFontSize(), { x, y }, titleColor.ToU32(Opacity), Title.c_str(), nullptr, textWidth);
                y += GetFontSize() + 2.0f;
            }
            if (!Message.empty())
                drawList.AddText(GetDefaultFont(), GetFontSize(), { x, y }, messageColor.ToU32(Opacity), Message.c_str(), nullptr, textWidth);
            if (IsClosable)
            {
                Rect close = { Bounds.Right() - 30.0f, Bounds.y + 8.0f, 22.0f, 22.0f };
                if (HasStyledPartVisual(Style.PART_CloseButton))
                    DrawStyledPart(drawList, close, Style.PART_CloseButton, GetVisualState());
                const Color closeColor = Style.PART_CloseGlyph.ForegroundColor.a > 0.0f ? Style.PART_CloseGlyph.ForegroundColor : ColorFromBytes(96, 94, 92);
                drawList.AddLine({ close.x + 7.0f, close.y + 7.0f }, { close.Right() - 7.0f, close.Bottom() - 7.0f }, closeColor, 1.0f);
                drawList.AddLine({ close.Right() - 7.0f, close.y + 7.0f }, { close.x + 7.0f, close.Bottom() - 7.0f }, closeColor, 1.0f);
            }
            DrawStyledPart(drawList, Bounds, Style.PART_Border, GetVisualState());
        }

        void OnEvent(EventArgs& args) override
        {
            if (!IsClosable || args.kind != EventKind::PointerReleased)
                return;
            auto& pointer = static_cast<PointerEventArgs&>(args);
            Rect close = { Bounds.Right() - 30.0f, Bounds.y + 8.0f, 22.0f, 22.0f };
            if (close.Contains(pointer.position))
            {
                IsOpen = false;
                if (OnCloseButtonClick)
                    OnCloseButtonClick();
                args.handled = true;
            }
        }

    private:
        Color SeverityColor() const
        {
            if (EqualsIgnoreCaseAscii(Severity, "Success")) return ColorFromBytes(16, 124, 16);
            if (EqualsIgnoreCaseAscii(Severity, "Warning")) return ColorFromBytes(157, 93, 0);
            if (EqualsIgnoreCaseAscii(Severity, "Error")) return ColorFromBytes(196, 43, 28);
            return ColorFromBytes(0, 120, 212);
        }

        Color SeverityBackground() const
        {
            if (EqualsIgnoreCaseAscii(Severity, "Success")) return ColorFromBytes(243, 252, 243);
            if (EqualsIgnoreCaseAscii(Severity, "Warning")) return ColorFromBytes(255, 250, 240);
            if (EqualsIgnoreCaseAscii(Severity, "Error")) return ColorFromBytes(253, 244, 242);
            return ColorFromBytes(243, 249, 253);
        }

        Color SeverityBorder() const
        {
            if (EqualsIgnoreCaseAscii(Severity, "Success")) return ColorFromBytes(196, 232, 196);
            if (EqualsIgnoreCaseAscii(Severity, "Warning")) return ColorFromBytes(250, 223, 179);
            if (EqualsIgnoreCaseAscii(Severity, "Error")) return ColorFromBytes(242, 203, 198);
            return ColorFromBytes(199, 224, 244);
        }
    };

    class TeachingTip : public Border
    {
    public:
        std::string Title;
        std::string Subtitle;
        bool IsOpen = true;

        TeachingTip()
        {
            Width = 320.0f;
            Padding = Thickness(16.0f);
            CornerRadius = 8.0f;
            BorderThickness = Thickness(1.0f);
            Background = ColorFromBytes(255, 255, 255);
            BorderBrush = ColorFromBytes(218, 218, 218);
            Foreground = ColorFromBytes(32, 31, 30);
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            if (!IsOpen)
                return {};
            const float width = Width >= 0.0f ? Width : std::min(availableSize.x, 320.0f);
            const float textWidth = std::max(0.0f, width - Padding.Horizontal());
            Vec2 title = Title.empty() ? Vec2{} : MeasureText(Title, textWidth);
            Vec2 subtitle = Subtitle.empty() ? Vec2{} : MeasureText(Subtitle, textWidth);
            return { width, title.y + subtitle.y + Padding.Vertical() + 8.0f };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            if (!IsOpen)
                return;
            DrawBackgroundAndBorder(drawList);
            float y = Bounds.y + Padding.top;
            if (!Title.empty())
            {
                drawList.AddText(GetDefaultFont(), GetFontSize(), { Bounds.x + Padding.left, y }, Foreground.ToU32(Opacity), Title.c_str(), nullptr, Bounds.width - Padding.Horizontal());
                y += GetFontSize() + 6.0f;
            }
            if (!Subtitle.empty())
                drawList.AddText(GetDefaultFont(), GetFontSize(), { Bounds.x + Padding.left, y }, ColorFromBytes(96, 94, 92).ToU32(Opacity), Subtitle.c_str(), nullptr, Bounds.width - Padding.Horizontal());
        }
    };

    class NavigationViewItem : public Button
    {
    public:
        std::string Icon;

        NavigationViewItem()
        {
            Width = 214.0f;
            Height = 32.0f;
            HorizontalContentAlignment = FyGUI::HorizontalAlignment::Left;
            BackgroundNormal = ColorFromBytes(0, 0, 0, 0);
            BackgroundHover = ColorFromBytes(242, 237, 235);
            BackgroundPressed = ColorFromBytes(232, 226, 224);
            BorderBrush = ColorFromBytes(0, 0, 0, 0);
        }
    };

}


// ---- Begin inlined ProgressBar control ----
namespace FyGUI
{
    class ProgressBar : public RangeBase
    {
    public:
        bool IsIndeterminate = false;
        bool ShowError = false;
        bool ShowPaused = false;
        Color Fill = { 0.20f, 0.55f, 0.90f, 1.0f };
        ProgressBarStyle Style;

        ProgressBar();
        void SetValue(double value) override;

    protected:
        void Update(float deltaTime) override;
        Vec2 MeasureOverride(Vec2 availableSize) override;
        void RenderOverride(ImDrawList& drawList) override;

    private:
        bool m_hasAnimatedValue = false;
    };
}

namespace FyGUI
{
    ProgressBar::ProgressBar()
    {
        Width = 130.0f;
        Height = 4.0f;
        ValueTransitionSpeed = 10.0f;
        BorderThickness = Thickness(0.0f);
        CornerRadius = 2.0f;
        BackgroundNormal = ColorFromBytes(230, 230, 230);
        BackgroundHover = BackgroundNormal;
        BackgroundPressed = BackgroundNormal;
        BorderBrush = ColorFromBytes(0, 0, 0, 0);
        Fill = ColorFromBytes(0, 120, 212);
    }

    void ProgressBar::SetValue(double value)
    {
        double old = Value;
        Value = std::clamp(value, Minimum, Maximum);
        if (old != Value)
        {
            if (!AnimateValueChanges || !m_hasAnimatedValue)
                AnimatedValue = Value;
            if (OnValueChanged)
                OnValueChanged(Value);
            ValueChangedEventArgs args;
            args.kind = EventKind::ValueChanged;
            args.route = RoutingStrategy::Bubble;
            args.originalSource = this;
            args.oldValue = old;
            args.newValue = Value;
            RaiseEvent(args);
        }
    }

    void ProgressBar::Update(float deltaTime)
    {
        Control::Update(deltaTime);
        if (!m_hasAnimatedValue)
        {
            AnimatedValue = Value;
            m_hasAnimatedValue = true;
            return;
        }

        const float step = std::clamp(deltaTime * ValueTransitionSpeed, 0.0f, 1.0f);
        AnimatedValue += (Value - AnimatedValue) * static_cast<double>(step);
        if (std::abs(AnimatedValue - Value) < 0.0001)
            AnimatedValue = Value;
    }

    Vec2 ProgressBar::MeasureOverride(Vec2)
    {
        return { Width >= 0.0f ? Width : 160.0f, Height >= 0.0f ? Height : 18.0f };
    }

    void ProgressBar::RenderOverride(ImDrawList& drawList)
    {
        VisualState state = GetVisualState();
        if (HasStyledPartVisual(Style.PART_Background) || HasStyledPartVisual(Style.PART_Track) || HasStyledPartVisual(Style.PART_Root))
        {
            DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
            ControlPartStyle background = Style.PART_Background;
            if (!HasStyledPartVisual(background))
            {
                background.BackgroundColor = BackgroundNormal;
                background.BorderColor = BorderBrush;
                background.BorderThickness = BorderThickness.left;
                background.CornerRadius = CornerRadius;
            }
            background = ApplyStateTransition(background);
            DrawStyledPart(drawList, Bounds, background, state);
            DrawStyledPart(drawList, Bounds, Style.PART_Track, state);
        }
        else
        {
            Control::RenderOverride(drawList);
        }

        Rect fillRect = Bounds;
        Color fillColor = ShowError ? ColorFromBytes(196, 43, 28) : (ShowPaused ? ColorFromBytes(255, 185, 0) : Fill);
        if (IsIndeterminate)
        {
            const float segmentWidth = std::max(24.0f, Bounds.width * 0.34f);
            const float t = std::fmod(static_cast<float>(GetTimeSeconds()) * 0.65f, 1.0f);
            fillRect.x = Bounds.x + (Bounds.width + segmentWidth) * t - segmentWidth;
            fillRect.width = segmentWidth;
            fillRect.x = std::max(Bounds.x, fillRect.x);
            fillRect.width = std::min(fillRect.width, Bounds.Right() - fillRect.x);
        }
        else
        {
            const double range = std::max(0.0001, Maximum - Minimum);
            const float t = static_cast<float>((AnimatedValue - Minimum) / range);
            fillRect.width *= std::clamp(t, 0.0f, 1.0f);
        }
        if (HasStyledPartVisual(Style.PART_Fill))
            DrawStyledPart(drawList, fillRect, ApplyStateTransition(Style.PART_Fill), state);
        else
            drawList.AddRectFilled(Vec2(fillRect.Left(), fillRect.Top()), Vec2(fillRect.Right(), fillRect.Bottom()), fillColor.ToU32(Opacity), CornerRadius);

        if (HasStyledPartVisual(Style.PART_Overlay))
            DrawStyledPart(drawList, Bounds, ApplyStateTransition(Style.PART_Overlay), state);
    }
}

// ---- End inlined ProgressBar control ----


namespace FyGUI
{

}


// ---- Begin inlined Slider control ----
namespace FyGUI
{
    class Slider : public RangeBase
    {
    public:
        FyGUI::Orientation Orientation = FyGUI::Orientation::Horizontal;
        float TrackHeight = 4.0f;
        float ThumbRadius = 8.0f;
        bool IsDragging = false;
        bool IsFocusEngaged = false;
        bool IsDirectionReversed = false;
        double StepFrequency = 0.0;
        double SmallChange = 0.0;
        double TickFrequency = 0.0;
        std::string TickPlacement;
        std::string Header;
        std::function<void()> OnDragStarted;
        std::function<void()> OnDragCompleted;
        SliderStyle Style;

        Slider();
        void SetValue(double value) override;

    protected:
        void Update(float deltaTime) override;
        Vec2 MeasureOverride(Vec2 availableSize) override;
        void RenderOverride(ImDrawList& drawList) override;
        void OnEvent(EventArgs& args) override;
        bool OnAction(UIAction action) override;

    private:
        void DrawTicks(ImDrawList& drawList, Rect trackRect, bool vertical);
        void UpdateFromPointer(Vec2 position);

        bool m_hasAnimatedValue = false;
    };
}

namespace FyGUI
{
    Slider::Slider()
    {
        Width = 200.0f;
        Height = AutoSize;
        TrackHeight = 2.0f;
        ThumbRadius = 10.0f;
        BackgroundNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
        BackgroundHover = BackgroundNormal;
        BackgroundPressed = BackgroundNormal;
        BorderBrush = BackgroundNormal;
        IsTabStop = true;
    }

    void Slider::SetValue(double value)
    {
        double old = Value;
        double next = std::clamp(value, Minimum, Maximum);
        if (StepFrequency > 0.0)
            next = Minimum + std::round((next - Minimum) / StepFrequency) * StepFrequency;
        Value = std::clamp(next, Minimum, Maximum);
        if (old == Value)
            return;
        if (IsDragging || !AnimateValueChanges || !m_hasAnimatedValue)
            AnimatedValue = Value;
        if (OnValueChanged)
            OnValueChanged(Value);
        ValueChangedEventArgs args;
        args.kind = EventKind::ValueChanged;
        args.route = RoutingStrategy::Bubble;
        args.originalSource = this;
        args.oldValue = old;
        args.newValue = Value;
        RaiseEvent(args);
    }

    void Slider::Update(float deltaTime)
    {
        Control::Update(deltaTime);
        if (!m_hasAnimatedValue)
        {
            AnimatedValue = Value;
            m_hasAnimatedValue = true;
            return;
        }

        if (IsDragging)
        {
            AnimatedValue = Value;
            return;
        }

        const float step = std::clamp(deltaTime * ValueTransitionSpeed, 0.0f, 1.0f);
        AnimatedValue += (Value - AnimatedValue) * static_cast<double>(step);
        if (std::abs(AnimatedValue - Value) < 0.0001)
            AnimatedValue = Value;
    }

    Vec2 Slider::MeasureOverride(Vec2)
    {
        const bool vertical = Orientation == FyGUI::Orientation::Vertical;
        const float headerHeight = Header.empty() ? 0.0f : 22.0f;
        return {
            Width >= 0.0f ? Width : (vertical ? 44.0f : 200.0f),
            Height >= 0.0f ? Height : (vertical ? 180.0f : 32.0f + headerHeight)
        };
    }

    void Slider::RenderOverride(ImDrawList& drawList)
    {
        VisualState state = GetVisualState();
        Rect sliderBounds = Bounds;
        if (!Header.empty())
        {
            drawList.AddText(GetDefaultFont(), GetFontSize(), { Bounds.x, Bounds.y }, Foreground.ToU32(Opacity), Header.c_str(), nullptr, Bounds.width);
            sliderBounds.y += 22.0f;
            sliderBounds.height = std::max(0.0f, sliderBounds.height - 22.0f);
        }
        const double range = std::max(0.0001, Maximum - Minimum);
        const float t = static_cast<float>((AnimatedValue - Minimum) / range);
        const float clampedT = std::clamp(t, 0.0f, 1.0f);
        Rect trackRect {};
        Rect fillRect {};
        Rect thumbRect {};
        const bool vertical = Orientation == FyGUI::Orientation::Vertical;
        float thumbCenterX = Bounds.x + Bounds.width * 0.5f;
        float thumbCenterY = Bounds.y + Bounds.height * 0.5f;

        if (vertical)
        {
            const float cx = sliderBounds.x + sliderBounds.width * 0.5f;
            const float top = sliderBounds.y + ThumbRadius;
            const float bottom = std::max(top, sliderBounds.Bottom() - ThumbRadius);
            const float usable = std::max(0.0f, bottom - top);
            const float visualT = IsDirectionReversed ? clampedT : (1.0f - clampedT);
            thumbCenterY = top + usable * visualT;
            thumbCenterX = cx;
            trackRect = { cx - TrackHeight * 0.5f, top, TrackHeight, usable };
            fillRect = IsDirectionReversed ?
                Rect{ cx - TrackHeight * 0.5f, top, TrackHeight, std::max(0.0f, thumbCenterY - top) } :
                Rect{ cx - TrackHeight * 0.5f, thumbCenterY, TrackHeight, std::max(0.0f, bottom - thumbCenterY) };
            thumbRect = { cx - ThumbRadius, thumbCenterY - ThumbRadius, ThumbRadius * 2.0f, ThumbRadius * 2.0f };
        }
        else
        {
            const float cy = sliderBounds.y + sliderBounds.height * 0.5f;
            const float left = sliderBounds.x + ThumbRadius;
            const float right = std::max(left, sliderBounds.Right() - ThumbRadius);
            const float usable = std::max(0.0f, right - left);
            const float visualT = IsDirectionReversed ? (1.0f - clampedT) : clampedT;
            thumbCenterX = left + usable * visualT;
            thumbCenterY = cy;
            trackRect = { left, cy - TrackHeight * 0.5f, usable, TrackHeight };
            fillRect = IsDirectionReversed ?
                Rect{ thumbCenterX, cy - TrackHeight * 0.5f, std::max(0.0f, right - thumbCenterX), TrackHeight } :
                Rect{ left, cy - TrackHeight * 0.5f, std::max(0.0f, thumbCenterX - left), TrackHeight };
            thumbRect = { thumbCenterX - ThumbRadius, cy - ThumbRadius, ThumbRadius * 2.0f, ThumbRadius * 2.0f };
        }

        if (HasStyledPartVisual(Style.PART_Root))
            DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
        if (HasStyledPartVisual(Style.PART_Track))
            DrawStyledPart(drawList, trackRect, ApplyStateTransition(Style.PART_Track), state);
        else
            drawList.AddRectFilled(Vec2(trackRect.Left(), trackRect.Top()), Vec2(trackRect.Right(), trackRect.Bottom()), ColorFromBytes(230, 230, 230, 255), TrackHeight * 0.5f);
        if (HasStyledPartVisual(Style.PART_FillTrack))
            DrawStyledPart(drawList, fillRect, ApplyStateTransition(Style.PART_FillTrack), state);
        else
            drawList.AddRectFilled(Vec2(fillRect.Left(), fillRect.Top()), Vec2(fillRect.Right(), fillRect.Bottom()), ColorFromBytes(0, 120, 212, 255), TrackHeight * 0.5f);
        if (TickFrequency > 0.0)
            DrawTicks(drawList, trackRect, vertical);
        if (HasStyledPartVisual(Style.PART_Thumb))
            DrawStyledPart(drawList, thumbRect, ApplyStateTransition(Style.PART_Thumb), IsDragging ? VisualState::Pressed : state);
        else
        {
            drawList.AddCircleFilled(Vec2{ thumbCenterX, thumbCenterY }, ThumbRadius, ColorFromBytes(255, 255, 255, 255), 24);
            drawList.AddCircle(Vec2{ thumbCenterX, thumbCenterY }, ThumbRadius, IsDragging ? ColorFromBytes(0, 95, 184, 255) : ColorFromBytes(138, 136, 134, 255), 24, 1.0f);
        }
        if (HasStyledPartVisual(Style.PART_ThumbGrip))
            DrawStyledPart(drawList, thumbRect, Style.PART_ThumbGrip, state);
    }

    void Slider::OnEvent(EventArgs& args)
    {
        if (args.kind == EventKind::PointerPressed)
        {
            IsDragging = true;
            auto& pointer = static_cast<PointerEventArgs&>(args);
            CapturePointer(pointer.pointerId);
            if (OnDragStarted)
                OnDragStarted();
            UpdateFromPointer(pointer.position);
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerMoved && IsDragging)
        {
            UpdateFromPointer(static_cast<PointerEventArgs&>(args).position);
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerReleased)
        {
            auto& pointer = static_cast<PointerEventArgs&>(args);
            if (IsDragging && OnDragCompleted)
                OnDragCompleted();
            IsDragging = false;
            ReleasePointer(pointer.pointerId);
            args.handled = true;
        }
    }

    bool Slider::OnAction(UIAction action)
    {
        double step = SmallChange > 0.0 ? SmallChange : (StepFrequency > 0.0 ? StepFrequency : (Maximum - Minimum) / 20.0);
        const bool vertical = Orientation == FyGUI::Orientation::Vertical;
        if ((!vertical && action == UIAction::NavigateLeft) || (vertical && action == UIAction::NavigateDown))
        {
            SetValue(Value - step);
            return true;
        }
        if ((!vertical && action == UIAction::NavigateRight) || (vertical && action == UIAction::NavigateUp))
        {
            SetValue(Value + step);
            return true;
        }
        return false;
    }

    void Slider::DrawTicks(ImDrawList& drawList, Rect trackRect, bool vertical)
    {
        const double range = Maximum - Minimum;
        if (range <= 0.0 || TickFrequency <= 0.0)
            return;

        const int tickCount = std::min(256, static_cast<int>(std::floor(range / TickFrequency)) + 1);
        const Color tickColor = ColorFromBytes(138, 136, 134, 190);
        for (int i = 0; i < tickCount; ++i)
        {
            const double value = std::min(Maximum, Minimum + TickFrequency * static_cast<double>(i));
            const float t = static_cast<float>((value - Minimum) / range);
            if (vertical)
            {
                const float y = trackRect.Bottom() - trackRect.height * t;
                drawList.AddLine({ trackRect.Right() + 5.0f, y }, { trackRect.Right() + 11.0f, y }, tickColor, 1.0f);
            }
            else
            {
                const float x = trackRect.x + trackRect.width * t;
                drawList.AddLine({ x, trackRect.Bottom() + 5.0f }, { x, trackRect.Bottom() + 11.0f }, tickColor, 1.0f);
            }
        }
    }

    void Slider::UpdateFromPointer(Vec2 position)
    {
        Rect sliderBounds = Bounds;
        if (!Header.empty())
        {
            sliderBounds.y += 22.0f;
            sliderBounds.height = std::max(0.0f, sliderBounds.height - 22.0f);
        }
        float t = 0.0f;
        if (Orientation == FyGUI::Orientation::Vertical)
        {
            const float top = sliderBounds.y + ThumbRadius;
            const float bottom = sliderBounds.Bottom() - ThumbRadius;
            t = bottom > top ? std::clamp((bottom - position.y) / (bottom - top), 0.0f, 1.0f) : 0.0f;
        }
        else
        {
            const float left = sliderBounds.x + ThumbRadius;
            const float right = sliderBounds.Right() - ThumbRadius;
            t = right > left ? std::clamp((position.x - left) / (right - left), 0.0f, 1.0f) : 0.0f;
        }
        if (IsDirectionReversed)
            t = 1.0f - t;
        SetValue(Minimum + (Maximum - Minimum) * t);
    }
}

// ---- End inlined Slider control ----


namespace FyGUI
{

}


// ---- Begin inlined ListBox control ----
// Inlined after the core control and style types are declared.
namespace FyGUI
{
    struct ListBoxItemData
    {
        std::string Text;
        std::string BadgeText;
        ImageSource Icon;
        Color Foreground = { 1.0f, 1.0f, 1.0f, 1.0f };
        bool HasForeground = false;
        bool IsEnabled = true;
        void* UserData = nullptr;
    };

    class ListBox : public SelectorBase
    {
    public:
        std::vector<std::string> Items;
        std::vector<ListBoxItemData> ItemData;
        float ItemHeight = 28.0f;
        int32_t VisibleItems = 6;
        float ScrollOffsetY = 0.0f;
        bool IsScrollBarDragging = false;
        std::string ItemTemplateName = "Text";
        ListBoxStyle Style;

        ListBox();

        void SetItemData(std::vector<ListBoxItemData> items);
        void SetSelectedIndex(int32_t index) override;
        std::string GetItemText(int32_t index) const;
        std::string GetSelectedText() const;
        size_t GetItemCount() const;
        void ScrollBy(float delta);

    protected:
        size_t ItemCount() const;
        const ListBoxItemData* ItemDataAt(int32_t index) const;
        std::string ItemTextAt(int32_t index) const;
        bool UsesItemIconTemplate() const;
        bool UsesItemBadgeTemplate() const;

        Vec2 MeasureOverride(Vec2 availableSize) override;
        void RenderOverride(ImDrawList& drawList) override;
        void OnEvent(EventArgs& args) override;
        bool OnAction(UIAction action) override;
        int HitVisibleItemIndex(Vec2 position) const;

    private:
        bool NeedsScrollBar() const;
        Rect ScrollBarRect() const;
        Rect ScrollThumbRect() const;
        bool TryScrollBarPointer(Vec2 position);
        void UpdateScrollFromPointer(float y);
        void EnsureSelectedVisible();
        void DrawListScrollBar(ImDrawList& drawList, VisualState state);

        float m_scrollThumbGrabOffsetY = 0.0f;
        mutable std::vector<float> m_itemHitX;
        mutable std::vector<float> m_itemHitY;
        mutable std::vector<float> m_itemHitWidth;
        mutable std::vector<float> m_itemHitHeight;
        mutable std::vector<int> m_itemHitIndices;
    };
}

namespace FyGUI
{
    ListBox::ListBox()
    {
        Width = 220.0f;
        BorderThickness = Thickness(1.0f);
        CornerRadius = 3.0f;
        BackgroundNormal = ColorFromBytes(255, 255, 255);
        BackgroundHover = BackgroundNormal;
        BackgroundPressed = BackgroundNormal;
        BorderBrush = ColorFromBytes(138, 136, 134);
        Foreground = ColorFromBytes(32, 31, 30);
        IsTabStop = true;
    }

    void ListBox::SetItemData(std::vector<ListBoxItemData> items)
    {
        ItemData = std::move(items);
        Items.clear();
        Items.reserve(ItemData.size());
        for (const auto& item : ItemData)
            Items.push_back(item.Text);
        if (SelectedIndex >= static_cast<int32_t>(ItemCount()))
            SelectedIndex = ItemCount() == 0 ? -1 : static_cast<int32_t>(ItemCount()) - 1;
        EnsureSelectedVisible();
    }

    void ListBox::SetSelectedIndex(int32_t index)
    {
        const size_t count = ItemCount();
        int32_t clamped = count == 0 ? -1 : std::clamp(index, 0, static_cast<int32_t>(count) - 1);
        if (SelectedIndex == clamped)
            return;
        int32_t old = SelectedIndex;
        SelectedIndex = clamped;
        EnsureSelectedVisible();
        if (OnSelectionChanged)
            OnSelectionChanged(SelectedIndex);
        SelectionChangedEventArgs args;
        args.kind = EventKind::SelectionChanged;
        args.route = RoutingStrategy::Bubble;
        args.originalSource = this;
        args.oldIndex = old;
        args.newIndex = SelectedIndex;
        RaiseEvent(args);
    }

    std::string ListBox::GetItemText(int32_t index) const
    {
        return ItemTextAt(index);
    }

    std::string ListBox::GetSelectedText() const
    {
        return GetItemText(SelectedIndex);
    }

    size_t ListBox::GetItemCount() const
    {
        return ItemCount();
    }

    void ListBox::ScrollBy(float delta)
    {
        const float contentHeight = ItemHeight * static_cast<float>(ItemCount());
        const float viewportHeight = ItemHeight * static_cast<float>(std::max(1, VisibleItems));
        ScrollOffsetY = std::clamp(ScrollOffsetY + delta, 0.0f, std::max(0.0f, contentHeight - viewportHeight));
    }

    size_t ListBox::ItemCount() const
    {
        return !ItemData.empty() ? ItemData.size() : Items.size();
    }

    const ListBoxItemData* ListBox::ItemDataAt(int32_t index) const
    {
        return index >= 0 && index < static_cast<int32_t>(ItemData.size()) ? &ItemData[static_cast<size_t>(index)] : nullptr;
    }

    std::string ListBox::ItemTextAt(int32_t index) const
    {
        if (const ListBoxItemData* data = ItemDataAt(index))
            return data->Text;
        return index >= 0 && index < static_cast<int32_t>(Items.size()) ? Items[static_cast<size_t>(index)] : std::string{};
    }

    bool ListBox::UsesItemIconTemplate() const
    {
        return EqualsIgnoreCaseAscii(ItemTemplateName, "IconText") ||
            EqualsIgnoreCaseAscii(ItemTemplateName, "IconBadge") ||
            EqualsIgnoreCaseAscii(ItemTemplateName, "GameItem");
    }

    bool ListBox::UsesItemBadgeTemplate() const
    {
        return EqualsIgnoreCaseAscii(ItemTemplateName, "Badge") ||
            EqualsIgnoreCaseAscii(ItemTemplateName, "IconBadge") ||
            EqualsIgnoreCaseAscii(ItemTemplateName, "GameItem");
    }

    Vec2 ListBox::MeasureOverride(Vec2)
    {
        int count = VisibleItems > 0 ? std::min<int>(VisibleItems, static_cast<int>(ItemCount())) : static_cast<int>(ItemCount());
        return { Width >= 0.0f ? Width : 220.0f, Padding.Vertical() + ItemHeight * static_cast<float>(std::max(1, count)) };
    }

    void ListBox::RenderOverride(ImDrawList& drawList)
    {
        VisualState state = GetVisualState();
        if (HasStyledPartVisual(Style.PART_Root) || HasStyledPartVisual(Style.PART_Background) || HasStyledPartVisual(Style.PART_Border))
        {
            DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
            ControlPartStyle background = Style.PART_Background;
            if (!HasStyledPartVisual(background))
            {
                background.BackgroundColor = BackgroundNormal;
                background.BorderColor = BorderBrush;
                background.BorderThickness = BorderThickness.left;
                background.CornerRadius = CornerRadius;
            }
            DrawStyledPart(drawList, Bounds, background, state);
            DrawStyledPart(drawList, Bounds, Style.PART_Border, state);
        }
        else
        {
            Control::RenderOverride(drawList);
        }

        drawList.PushClipRect(Vec2(Bounds.Left(), Bounds.Top()), Vec2(Bounds.Right(), Bounds.Bottom()), true);
        const int firstIndex = static_cast<int>(ScrollOffsetY / std::max(1.0f, ItemHeight));
        const float firstOffset = std::fmod(ScrollOffsetY, std::max(1.0f, ItemHeight));
        const int count = std::min<int>(static_cast<int>(ItemCount()) - firstIndex, std::max(1, VisibleItems) + 1);
        for (int local = 0; local < count; ++local)
        {
            const int i = firstIndex + local;
            float y = Bounds.y + Padding.top + ItemHeight * static_cast<float>(local) - firstOffset;
            Rect itemRect = { Bounds.x + Padding.left, y, Bounds.width - Padding.Horizontal(), ItemHeight };
            if (itemRect.Bottom() < Bounds.Top() || itemRect.Top() > Bounds.Bottom())
                continue;
            if (i == SelectedIndex)
            {
                if (HasStyledPartVisual(Style.PART_SelectionVisual) || HasStyledPartVisual(Style.ItemStyle.PART_Selection))
                    DrawStyledPart(drawList, itemRect, HasStyledPartVisual(Style.ItemStyle.PART_Selection) ? Style.ItemStyle.PART_Selection : Style.PART_SelectionVisual, VisualState::Selected);
                else
                    drawList.AddRectFilled(Vec2(itemRect.Left(), itemRect.Top()), Vec2(itemRect.Right(), itemRect.Bottom()), ColorFromBytes(204, 228, 247, 255), 2.0f);
            }
            DrawStyledPart(drawList, itemRect, Style.PART_ItemContainer, i == SelectedIndex ? VisualState::Selected : VisualState::Normal);
            const ListBoxItemData* data = ItemDataAt(i);
            float textX = itemRect.x + 7.0f;
            if (data && data->Icon.texture && UsesItemIconTemplate())
            {
                const float iconSize = std::min(20.0f, std::max(0.0f, ItemHeight - 8.0f));
                Rect iconRect = { itemRect.x + 7.0f, itemRect.y + (ItemHeight - iconSize) * 0.5f, iconSize, iconSize };
                ControlPartStyle icon = Style.ItemStyle.PART_Icon;
                if (!icon.Image)
                    icon.Image = data->Icon.texture;
                icon.UseImage = true;
                DrawStyledPart(drawList, iconRect, icon, i == SelectedIndex ? VisualState::Selected : VisualState::Normal);
                textX = iconRect.Right() + 8.0f;
            }

            float textRight = itemRect.Right() - 7.0f;
            if (data && !data->BadgeText.empty() && UsesItemBadgeTemplate())
            {
                Vec2 badgeTextSize = MeasureText(data->BadgeText);
                const float badgeWidth = std::max(28.0f, badgeTextSize.x + 14.0f);
                Rect badgeRect = { itemRect.Right() - badgeWidth - 7.0f, itemRect.y + 5.0f, badgeWidth, std::max(0.0f, ItemHeight - 10.0f) };
                ControlPartStyle badge = Style.ItemStyle.PART_Badge;
                if (!HasStyledPartVisual(badge))
                {
                    badge.BackgroundColor = { 0.22f, 0.32f, 0.44f, 0.86f };
                    badge.BorderColor = { 0.48f, 0.72f, 0.92f, 0.74f };
                    badge.BorderThickness = 1.0f;
                    badge.CornerRadius = badgeRect.height * 0.45f;
                }
                DrawStyledPart(drawList, badgeRect, badge, i == SelectedIndex ? VisualState::Selected : VisualState::Normal);
                Color badgeText = ResolvePartForeground(Style.ItemStyle.PART_Badge, Color{ 0.92f, 0.97f, 1.0f, 1.0f });
                drawList.AddText(Vec2(badgeRect.x + (badgeRect.width - badgeTextSize.x) * 0.5f, badgeRect.y + (badgeRect.height - badgeTextSize.y) * 0.5f), badgeText.ToU32(Opacity), data->BadgeText.c_str());
                textRight = badgeRect.x - 7.0f;
            }

            Color textColor = data && data->HasForeground ? data->Foreground : ResolvePartForeground(Style.ItemStyle.PART_Text, Foreground);
            const std::string text = ItemTextAt(i);
            drawList.AddText(GetDefaultFont(), GetFontSize(), Vec2(textX, itemRect.y + (ItemHeight - GetFontSize()) * 0.5f), textColor.ToU32((data && !data->IsEnabled) ? Opacity * 0.45f : Opacity), text.c_str(), nullptr, std::max(0.0f, textRight - textX));
        }
        drawList.PopClipRect();

        DrawListScrollBar(drawList, state);
    }

    void ListBox::OnEvent(EventArgs& args)
    {
        if (args.kind == EventKind::PointerPressed)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            if (TryScrollBarPointer(pointer.position))
            {
                IsScrollBarDragging = true;
                CapturePointer(pointer.pointerId);
                args.handled = true;
                return;
            }

            int index = HitVisibleItemIndex(pointer.position);
            if (index >= 0 && index < static_cast<int>(ItemCount()))
                SetSelectedIndex(index);
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerMoved && IsScrollBarDragging)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            UpdateScrollFromPointer(pointer.position.y);
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerReleased)
        {
            IsScrollBarDragging = false;
            m_scrollThumbGrabOffsetY = 0.0f;
        }
        else if (args.kind == EventKind::PointerWheelChanged)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            ScrollBy(-pointer.wheelDelta.y * ItemHeight * 2.0f);
            args.handled = true;
        }
    }

    bool ListBox::OnAction(UIAction action)
    {
        if (action == UIAction::NavigateUp)
        {
            SetSelectedIndex(SelectedIndex <= 0 ? 0 : SelectedIndex - 1);
            return true;
        }
        if (action == UIAction::NavigateDown)
        {
            SetSelectedIndex(SelectedIndex + 1);
            return true;
        }
        return false;
    }

    int ListBox::HitVisibleItemIndex(Vec2 position) const
    {
        const int firstIndex = static_cast<int>(ScrollOffsetY / std::max(1.0f, ItemHeight));
        const float firstOffset = std::fmod(ScrollOffsetY, std::max(1.0f, ItemHeight));
        const int count = std::min<int>(static_cast<int>(ItemCount()) - firstIndex, std::max(1, VisibleItems) + 1);
        if (count <= 0)
            return -1;

        m_itemHitX.resize(static_cast<size_t>(count));
        m_itemHitY.resize(static_cast<size_t>(count));
        m_itemHitWidth.resize(static_cast<size_t>(count));
        m_itemHitHeight.resize(static_cast<size_t>(count));
        m_itemHitIndices.resize(static_cast<size_t>(count));
        for (int local = 0; local < count; ++local)
        {
            const int itemIndex = firstIndex + local;
            const float y = Bounds.y + Padding.top + ItemHeight * static_cast<float>(local) - firstOffset;
            m_itemHitX[static_cast<size_t>(local)] = Bounds.x + Padding.left;
            m_itemHitY[static_cast<size_t>(local)] = y;
            m_itemHitWidth[static_cast<size_t>(local)] = std::max(0.0f, Bounds.width - Padding.Horizontal());
            m_itemHitHeight[static_cast<size_t>(local)] = ItemHeight;
            m_itemHitIndices[static_cast<size_t>(local)] = itemIndex;
        }

        for (int local = 0; local < count; ++local)
        {
            Rect itemRect {
                m_itemHitX[static_cast<size_t>(local)],
                m_itemHitY[static_cast<size_t>(local)],
                m_itemHitWidth[static_cast<size_t>(local)],
                m_itemHitHeight[static_cast<size_t>(local)]
            };
            if (itemRect.Contains(position))
                return m_itemHitIndices[static_cast<size_t>(local)];
        }
        return -1;
    }

    bool ListBox::NeedsScrollBar() const
    {
        return static_cast<int>(ItemCount()) > std::max(1, VisibleItems);
    }

    Rect ListBox::ScrollBarRect() const
    {
        return { Bounds.Right() - 10.0f, Bounds.Top() + 2.0f, 8.0f, std::max(0.0f, Bounds.height - 4.0f) };
    }

    Rect ListBox::ScrollThumbRect() const
    {
        Rect bar = ScrollBarRect();
        const float contentHeight = ItemHeight * static_cast<float>(ItemCount());
        const float viewportHeight = ItemHeight * static_cast<float>(std::max(1, VisibleItems));
        const float thumbH = std::min(bar.height, std::max(0.0f, std::max(18.0f, (viewportHeight / std::max(1.0f, contentHeight)) * bar.height)));
        const float maxOffset = std::max(1.0f, contentHeight - viewportHeight);
        const float y = bar.y + (bar.height - thumbH) * std::clamp(ScrollOffsetY / maxOffset, 0.0f, 1.0f);
        return { bar.x, y, bar.width, thumbH };
    }

    bool ListBox::TryScrollBarPointer(Vec2 position)
    {
        if (!NeedsScrollBar())
            return false;
        Rect bar = ScrollBarRect();
        if (!bar.Contains(position))
            return false;
        Rect thumb = ScrollThumbRect();
        if (thumb.Contains(position))
        {
            m_scrollThumbGrabOffsetY = position.y - thumb.y;
            UpdateScrollFromPointer(position.y);
            return true;
        }

        const float viewportHeight = ItemHeight * static_cast<float>(std::max(1, VisibleItems));
        ScrollBy(position.y < thumb.y ? -viewportHeight : viewportHeight);
        thumb = ScrollThumbRect();
        m_scrollThumbGrabOffsetY = std::clamp(position.y - thumb.y, 0.0f, thumb.height);
        UpdateScrollFromPointer(position.y);
        return true;
    }

    void ListBox::UpdateScrollFromPointer(float y)
    {
        Rect bar = ScrollBarRect();
        Rect thumb = ScrollThumbRect();
        const float contentHeight = ItemHeight * static_cast<float>(ItemCount());
        const float viewportHeight = ItemHeight * static_cast<float>(std::max(1, VisibleItems));
        const float maxOffset = std::max(0.0f, contentHeight - viewportHeight);
        const float track = std::max(1.0f, bar.height - thumb.height);
        const float t = std::clamp((y - bar.y - m_scrollThumbGrabOffsetY) / track, 0.0f, 1.0f);
        ScrollOffsetY = maxOffset * t;
    }

    void ListBox::EnsureSelectedVisible()
    {
        if (SelectedIndex < 0 || ItemCount() == 0)
            return;

        const float contentHeight = ItemHeight * static_cast<float>(ItemCount());
        const float viewportHeight = ItemHeight * static_cast<float>(std::max(1, VisibleItems));
        const float maxOffset = std::max(0.0f, contentHeight - viewportHeight);
        const float itemTop = ItemHeight * static_cast<float>(SelectedIndex);
        const float itemBottom = itemTop + ItemHeight;
        if (itemTop < ScrollOffsetY)
            ScrollOffsetY = itemTop;
        else if (itemBottom > ScrollOffsetY + viewportHeight)
            ScrollOffsetY = itemBottom - viewportHeight;
        ScrollOffsetY = std::clamp(ScrollOffsetY, 0.0f, maxOffset);
    }

    void ListBox::DrawListScrollBar(ImDrawList& drawList, VisualState state)
    {
        if (!NeedsScrollBar())
            return;
        Rect bar = ScrollBarRect();
        ControlPartStyle track = Style.PART_ScrollViewer;
        if (!HasStyledPartVisual(track))
            track.BackgroundColor = ColorFromBytes(243, 243, 243, 255);
        DrawStyledPart(drawList, bar, track, state);

        ControlPartStyle thumb = Style.PART_HoverVisual;
        if (!HasStyledPartVisual(thumb))
        {
            thumb.BackgroundColor = ColorFromBytes(138, 136, 134, 210);
            thumb.CornerRadius = 4.0f;
        }
        DrawStyledPart(drawList, ScrollThumbRect(), thumb, IsScrollBarDragging ? VisualState::Pressed : state);
    }
}

// ---- End inlined ListBox control ----


// ---- Begin inlined ComboBox control ----
// Inlined after ListBox and ComboBoxStyle are declared.
namespace FyGUI
{
    class ComboBox : public ListBox
    {
    public:
        std::string Header;
        std::string Text;
        std::string PlaceholderText = "Select an item";
        bool IsOpen = false;
        bool IsEditable = false;
        float MaxDropDownHeight = 220.0f;
        ComboBoxStyle ComboStyle;
        std::function<void()> OnDropDownOpened;
        std::function<void()> OnDropDownClosed;

        ComboBox();
        void SetIsOpen(bool open);

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override;
        UIElement* HitTest(Vec2 point) override;
        UIElement* HitTestOverlay(Vec2 point) override;
        void OnEvent(EventArgs& args) override;
        bool OnAction(UIAction action) override;
        void RenderOverride(ImDrawList& drawList) override;
        void RenderOverlay(ImDrawList& drawList) override;

    private:
        Rect SelectionRect() const;
        Rect DropDownRect() const;
        int VisibleDropDownItemCount() const;
        int HitDropDownIndex(Vec2 position) const;
        void ScrollDropDownBy(float delta);
        bool NeedsDropDownScrollBar() const;
        Rect DropDownScrollBarRect() const;
        Rect DropDownScrollThumbRect() const;
        bool TryDropDownScrollBarPointer(Vec2 position);
        void UpdateDropDownScrollFromPointer(float y);
        void EnsureDropDownSelectedVisible();
        void DrawDropDownScrollBar(ImDrawList& drawList);

        bool m_isDropDownScrollBarDragging = false;
        float m_dropDownScrollThumbGrabOffsetY = 0.0f;
        mutable std::vector<float> m_comboHitX;
        mutable std::vector<float> m_comboHitY;
        mutable std::vector<float> m_comboHitWidth;
        mutable std::vector<float> m_comboHitHeight;
        mutable std::vector<int> m_comboHitIndices;
    };
}

namespace FyGUI
{
    ComboBox::ComboBox()
    {
        VisibleItems = 1;
        Height = AutoSize;
        Width = 200.0f;
        ItemHeight = 32.0f;
        BackgroundNormal = ColorFromBytes(255, 255, 255, 255);
        BackgroundHover = ColorFromBytes(255, 255, 255, 255);
        BackgroundPressed = ColorFromBytes(250, 250, 250, 255);
        Foreground = ColorFromBytes(32, 31, 30, 255);
        BorderBrush = ColorFromBytes(138, 136, 134, 255);
        BorderThickness = Thickness(1.0f);
        CornerRadius = 4.0f;
        VerticalAlignment = FyGUI::VerticalAlignment::Top;
        IsTabStop = true;
        UseSystemFocusVisuals = false;
    }

    Vec2 ComboBox::MeasureOverride(Vec2)
    {
        const float headerHeight = Header.empty() ? 0.0f : 20.0f;
        const float headerGap = Header.empty() ? 0.0f : 6.0f;
        return { Width >= 0.0f ? Width : 200.0f, headerHeight + headerGap + ItemHeight };
    }

    void ComboBox::SetIsOpen(bool open)
    {
        if (IsOpen == open)
            return;
        IsOpen = open;
        m_isDropDownScrollBarDragging = false;
        m_dropDownScrollThumbGrabOffsetY = 0.0f;
        if (IsOpen)
        {
            EnsureDropDownSelectedVisible();
            if (OnDropDownOpened)
                OnDropDownOpened();
        }
        else if (OnDropDownClosed)
        {
            OnDropDownClosed();
        }
    }

    UIElement* ComboBox::HitTest(Vec2 point)
    {
        if (Bounds.Contains(point))
            return this;
        if (IsOpen && IsEnabled && IsHitTestVisible && Visibility == FyGUI::Visibility::Visible && DropDownRect().Contains(point))
            return this;
        return nullptr;
    }

    UIElement* ComboBox::HitTestOverlay(Vec2 point)
    {
        if (IsOpen && IsEnabled && IsHitTestVisible && Visibility == FyGUI::Visibility::Visible && DropDownRect().Contains(point))
            return this;
        return nullptr;
    }

    void ComboBox::OnEvent(EventArgs& args)
    {
        if (args.kind == EventKind::PointerEntered)
        {
            IsHovered = true;
            return;
        }
        if (args.kind == EventKind::LostFocus)
        {
            SetIsOpen(false);
            return;
        }
        if (args.kind == EventKind::PointerExited)
        {
            IsHovered = false;
            return;
        }
        if (args.kind == EventKind::PointerPressed)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            if (IsOpen && DropDownRect().Contains(pointer.position))
            {
                if (TryDropDownScrollBarPointer(pointer.position))
                {
                    m_isDropDownScrollBarDragging = true;
                    CapturePointer(pointer.pointerId);
                    args.handled = true;
                    return;
                }

                const int index = HitDropDownIndex(pointer.position);
                if (index >= 0 && index < static_cast<int>(ItemCount()))
                {
                    SetSelectedIndex(index);
                    if (IsEditable)
                        Text = ItemTextAt(index);
                }
                SetIsOpen(false);
                args.handled = true;
                return;
            }

            if (SelectionRect().Contains(pointer.position) || Bounds.Contains(pointer.position))
            {
                SetIsOpen(!IsOpen);
                args.handled = true;
                return;
            }

            SetIsOpen(false);
            args.handled = true;
            return;
        }
        if (args.kind == EventKind::PointerMoved && m_isDropDownScrollBarDragging)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            UpdateDropDownScrollFromPointer(pointer.position.y);
            args.handled = true;
            return;
        }
        if (args.kind == EventKind::PointerReleased)
        {
            m_isDropDownScrollBarDragging = false;
            m_dropDownScrollThumbGrabOffsetY = 0.0f;
        }
        if (IsEditable && IsFocused() && args.kind == EventKind::TextInput)
        {
            auto& text = static_cast<TextInputEventArgs&>(args);
            if (text.codepoint >= 32 && text.codepoint < 127)
            {
                Text.push_back(static_cast<char>(text.codepoint));
                SetIsOpen(true);
                args.handled = true;
                return;
            }
        }
        if (IsEditable && IsFocused() && args.kind == EventKind::KeyDown)
        {
            auto& key = static_cast<KeyEventArgs&>(args);
            if ((key.key == Key::Backspace || key.key == Key::DeleteKey) && !Text.empty())
            {
                Text.pop_back();
                SetIsOpen(true);
                args.handled = true;
                return;
            }
        }
        if (args.kind == EventKind::PointerWheelChanged && IsOpen)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            ScrollDropDownBy(-pointer.wheelDelta.y * ItemHeight);
            args.handled = true;
            return;
        }
        ListBox::OnEvent(args);
    }

    bool ComboBox::OnAction(UIAction action)
    {
        if (action == UIAction::Accept)
        {
            SetIsOpen(!IsOpen);
            return true;
        }
        if (action == UIAction::Cancel && IsOpen)
        {
            SetIsOpen(false);
            return true;
        }
        if (action == UIAction::NavigateUp || action == UIAction::NavigateDown)
        {
            ListBox::OnAction(action);
            if (IsEditable)
                Text = ItemTextAt(SelectedIndex);
            if (!IsOpen)
                SetIsOpen(true);
            EnsureDropDownSelectedVisible();
            return true;
        }
        return ListBox::OnAction(action);
    }

    void ComboBox::RenderOverride(ImDrawList& drawList)
    {
        VisualState state = GetVisualState();
        const Rect selectionRect = SelectionRect();

        if (!Header.empty())
        {
            Color headerColor = ResolvePartForeground(ComboStyle.PART_HeaderText, Foreground);
            drawList.AddText(GetDefaultFont(), GetFontSize(), { Bounds.x, Bounds.y }, headerColor.ToU32(Opacity), Header.c_str(), nullptr, Bounds.width);
        }

        if (HasStyledPartVisual(ComboStyle.PART_Root))
            DrawStyledPart(drawList, Bounds, ComboStyle.PART_Root, state);

        ControlPartStyle selection = ComboStyle.PART_SelectionBox;
        if (!HasStyledPartVisual(selection))
        {
            selection.BackgroundColor = BackgroundNormal;
            selection.BorderColor = IsFocused() ? ColorFromBytes(0, 120, 212, 255) : BorderBrush;
            selection.BorderThickness = BorderThickness.left;
            selection.CornerRadius = CornerRadius;
            selection.ForegroundColor = Foreground;
        }
        DrawStyledPart(drawList, selectionRect, selection, IsOpen ? VisualState::Pressed : state);

        const std::string selectedValue = SelectedIndex >= 0 ? ItemTextAt(SelectedIndex) : std::string{};
        const std::string selectedText = IsEditable && !Text.empty() ? Text : (!selectedValue.empty() ? selectedValue : PlaceholderText);
        const bool hasValue = (IsEditable && !Text.empty()) || !selectedValue.empty();
        const Color textColor = hasValue ? ResolvePartForeground(ComboStyle.PART_Text, Foreground) : ColorFromBytes(96, 94, 92, 255);
        const float buttonWidth = 28.0f;
        Rect textRect = { selectionRect.x + 10.0f, selectionRect.y, std::max(0.0f, selectionRect.width - buttonWidth - 16.0f), selectionRect.height };
        Vec2 textSize = MeasureText(selectedText);
        drawList.PushClipRect({ textRect.Left(), textRect.Top() }, { textRect.Right(), textRect.Bottom() }, true);
        drawList.AddText(GetDefaultFont(), GetFontSize(), { textRect.x, std::floor(textRect.y + (textRect.height - textSize.y) * 0.5f) }, textColor.ToU32(Opacity), selectedText.c_str());
        drawList.PopClipRect();
        if (IsEditable && IsFocused() && std::fmod(static_cast<float>(GetTimeSeconds()), 1.0f) < 0.55f)
        {
            const float caretX = std::min(textRect.Right() - 2.0f, textRect.x + MeasureText(selectedText).x + 2.0f);
            drawList.AddLine({ caretX, selectionRect.y + 7.0f }, { caretX, selectionRect.Bottom() - 7.0f }, Foreground, 1.0f);
        }

        Rect buttonRect = { selectionRect.Right() - buttonWidth, selectionRect.Top(), buttonWidth, selectionRect.height };
        ControlPartStyle button = ComboStyle.PART_DropDownButton;
        if (!HasStyledPartVisual(button))
        {
            button.BackgroundColor = ColorFromBytes(0, 0, 0, 0);
            button.CornerRadius = CornerRadius;
        }
        DrawStyledPart(drawList, buttonRect, button, state);

        ControlPartStyle glyph = ComboStyle.PART_DropDownGlyph;
        const Color glyphColor = ResolvePartForeground(glyph, ColorFromBytes(50, 49, 48, 255));
        const Vec2 center = { buttonRect.x + buttonRect.width * 0.5f, buttonRect.y + buttonRect.height * 0.5f };
        if (IsOpen)
        {
            drawList.AddLine({ center.x - 4.5f, center.y + 2.0f }, { center.x, center.y - 2.5f }, glyphColor, 1.4f);
            drawList.AddLine({ center.x, center.y - 2.5f }, { center.x + 4.5f, center.y + 2.0f }, glyphColor, 1.4f);
        }
        else
        {
            drawList.AddLine({ center.x - 4.5f, center.y - 2.0f }, { center.x, center.y + 2.5f }, glyphColor, 1.4f);
            drawList.AddLine({ center.x, center.y + 2.5f }, { center.x + 4.5f, center.y - 2.0f }, glyphColor, 1.4f);
        }

        if (IsFocused() && GetFocusVisualsEnabled())
            drawList.AddRect({ selectionRect.Left() + 0.5f, selectionRect.Top() + 0.5f }, { selectionRect.Right() - 0.5f, selectionRect.Bottom() - 0.5f }, ColorFromBytes(0, 120, 212, 255), CornerRadius, ImDrawFlags_None, 1.5f);
    }

    void ComboBox::RenderOverlay(ImDrawList& drawList)
    {
        if (!IsOpen)
            return;

        VisualState state = GetVisualState();
        const Rect popupRect = DropDownRect();
        ControlPartStyle popup = ComboStyle.PART_Popup;
        if (!HasStyledPartVisual(popup))
        {
            popup.BackgroundColor = ColorFromBytes(255, 255, 255, 255);
            popup.BorderColor = ColorFromBytes(138, 136, 134, 255);
            popup.BorderThickness = 1.0f;
            popup.CornerRadius = CornerRadius;
        }
        DrawStyledPart(drawList, popupRect, popup, state);

        drawList.PushClipRect({ popupRect.Left(), popupRect.Top() }, { popupRect.Right(), popupRect.Bottom() }, true);
        const int firstIndex = static_cast<int>(ScrollOffsetY / std::max(1.0f, ItemHeight));
        const float firstOffset = std::fmod(ScrollOffsetY, std::max(1.0f, ItemHeight));
        const int count = std::min<int>(VisibleDropDownItemCount() + 1, static_cast<int>(ItemCount()) - firstIndex);
        for (int local = 0; local < count; ++local)
        {
            const int index = firstIndex + local;
            Rect itemRect = { popupRect.x + 1.0f, popupRect.y + ItemHeight * static_cast<float>(local) - firstOffset, std::max(0.0f, popupRect.width - 2.0f), ItemHeight };
            if (itemRect.Bottom() < popupRect.Top() || itemRect.Top() > popupRect.Bottom())
                continue;

            const bool selected = index == SelectedIndex;
            const float scrollReserve = NeedsDropDownScrollBar() ? 10.0f : 0.0f;
            itemRect.width = std::max(0.0f, itemRect.width - scrollReserve);
            if (selected)
            {
                ControlPartStyle selectedStyle = ComboStyle.ItemStyle.PART_Selection;
                if (!HasStyledPartVisual(selectedStyle))
                    selectedStyle = SolidPart(ColorFromBytes(204, 228, 247, 255), ColorFromBytes(204, 228, 247, 0), 0.0f, 2.0f);
                DrawStyledPart(drawList, itemRect, selectedStyle, VisualState::Selected);
            }

            const ListBoxItemData* data = ItemDataAt(index);
            const Color itemColor = data && data->HasForeground ? data->Foreground : ResolvePartForeground(ComboStyle.ItemStyle.PART_Text, Foreground);
            const std::string itemText = ItemTextAt(index);
            Vec2 itemTextSize = MeasureText(itemText);
            Rect itemTextRect = { itemRect.x + 9.0f, itemRect.y, std::max(0.0f, itemRect.width - 18.0f), itemRect.height };
            drawList.PushClipRect({ itemTextRect.Left(), itemTextRect.Top() }, { itemTextRect.Right(), itemTextRect.Bottom() }, true);
            drawList.AddText(GetDefaultFont(), GetFontSize(), { itemTextRect.x, std::floor(itemTextRect.y + (itemTextRect.height - itemTextSize.y) * 0.5f) }, itemColor.ToU32((data && !data->IsEnabled) ? Opacity * 0.45f : Opacity), itemText.c_str());
            drawList.PopClipRect();
        }
        drawList.PopClipRect();

        DrawDropDownScrollBar(drawList);
    }

    Rect ComboBox::SelectionRect() const
    {
        const float headerHeight = Header.empty() ? 0.0f : 20.0f;
        const float headerGap = Header.empty() ? 0.0f : 6.0f;
        return { Bounds.x, Bounds.y + headerHeight + headerGap, Bounds.width, ItemHeight };
    }

    Rect ComboBox::DropDownRect() const
    {
        Rect selection = SelectionRect();
        const float height = ItemHeight * static_cast<float>(VisibleDropDownItemCount());
        return { selection.x, selection.Bottom() + 2.0f, selection.width, height };
    }

    int ComboBox::VisibleDropDownItemCount() const
    {
        const int fromHeight = MaxDropDownHeight > 0.0f ? std::max(1, static_cast<int>(MaxDropDownHeight / std::max(1.0f, ItemHeight))) : 6;
        const int requested = VisibleItems > 1 ? VisibleItems : 6;
        return std::max(1, std::min({ requested, fromHeight, std::max(1, static_cast<int>(ItemCount())) }));
    }

    int ComboBox::HitDropDownIndex(Vec2 position) const
    {
        Rect popup = DropDownRect();
        const int firstIndex = static_cast<int>(ScrollOffsetY / std::max(1.0f, ItemHeight));
        const float firstOffset = std::fmod(ScrollOffsetY, std::max(1.0f, ItemHeight));
        const int count = std::min<int>(VisibleDropDownItemCount() + 1, static_cast<int>(ItemCount()) - firstIndex);
        if (count <= 0)
            return -1;

        m_comboHitX.resize(static_cast<size_t>(count));
        m_comboHitY.resize(static_cast<size_t>(count));
        m_comboHitWidth.resize(static_cast<size_t>(count));
        m_comboHitHeight.resize(static_cast<size_t>(count));
        m_comboHitIndices.resize(static_cast<size_t>(count));
        for (int local = 0; local < count; ++local)
        {
            const int itemIndex = firstIndex + local;
            const float scrollReserve = NeedsDropDownScrollBar() ? 10.0f : 0.0f;
            m_comboHitX[static_cast<size_t>(local)] = popup.x + 1.0f;
            m_comboHitY[static_cast<size_t>(local)] = popup.y + ItemHeight * static_cast<float>(local) - firstOffset;
            m_comboHitWidth[static_cast<size_t>(local)] = std::max(0.0f, popup.width - 2.0f - scrollReserve);
            m_comboHitHeight[static_cast<size_t>(local)] = ItemHeight;
            m_comboHitIndices[static_cast<size_t>(local)] = itemIndex;
        }

        for (int local = 0; local < count; ++local)
        {
            Rect itemRect {
                m_comboHitX[static_cast<size_t>(local)],
                m_comboHitY[static_cast<size_t>(local)],
                m_comboHitWidth[static_cast<size_t>(local)],
                m_comboHitHeight[static_cast<size_t>(local)]
            };
            if (itemRect.Contains(position))
                return m_comboHitIndices[static_cast<size_t>(local)];
        }
        return -1;
    }

    void ComboBox::ScrollDropDownBy(float delta)
    {
        const float contentHeight = ItemHeight * static_cast<float>(ItemCount());
        const float viewportHeight = ItemHeight * static_cast<float>(VisibleDropDownItemCount());
        ScrollOffsetY = std::clamp(ScrollOffsetY + delta, 0.0f, std::max(0.0f, contentHeight - viewportHeight));
    }

    bool ComboBox::NeedsDropDownScrollBar() const
    {
        return static_cast<int>(ItemCount()) > VisibleDropDownItemCount();
    }

    Rect ComboBox::DropDownScrollBarRect() const
    {
        Rect popup = DropDownRect();
        return { popup.Right() - 9.0f, popup.Top() + 3.0f, 6.0f, std::max(0.0f, popup.height - 6.0f) };
    }

    Rect ComboBox::DropDownScrollThumbRect() const
    {
        Rect bar = DropDownScrollBarRect();
        const float contentHeight = ItemHeight * static_cast<float>(ItemCount());
        const float viewportHeight = ItemHeight * static_cast<float>(VisibleDropDownItemCount());
        const float thumbH = std::min(bar.height, std::max(18.0f, (viewportHeight / std::max(1.0f, contentHeight)) * bar.height));
        const float maxOffset = std::max(1.0f, contentHeight - viewportHeight);
        const float y = bar.y + (bar.height - thumbH) * std::clamp(ScrollOffsetY / maxOffset, 0.0f, 1.0f);
        return { bar.x, y, bar.width, thumbH };
    }

    bool ComboBox::TryDropDownScrollBarPointer(Vec2 position)
    {
        if (!NeedsDropDownScrollBar())
            return false;
        Rect bar = DropDownScrollBarRect();
        if (!bar.Contains(position))
            return false;
        Rect thumb = DropDownScrollThumbRect();
        if (thumb.Contains(position))
        {
            m_dropDownScrollThumbGrabOffsetY = position.y - thumb.y;
            UpdateDropDownScrollFromPointer(position.y);
            return true;
        }

        const float viewportHeight = ItemHeight * static_cast<float>(VisibleDropDownItemCount());
        ScrollDropDownBy(position.y < thumb.y ? -viewportHeight : viewportHeight);
        thumb = DropDownScrollThumbRect();
        m_dropDownScrollThumbGrabOffsetY = std::clamp(position.y - thumb.y, 0.0f, thumb.height);
        UpdateDropDownScrollFromPointer(position.y);
        return true;
    }

    void ComboBox::UpdateDropDownScrollFromPointer(float y)
    {
        Rect bar = DropDownScrollBarRect();
        Rect thumb = DropDownScrollThumbRect();
        const float contentHeight = ItemHeight * static_cast<float>(ItemCount());
        const float viewportHeight = ItemHeight * static_cast<float>(VisibleDropDownItemCount());
        const float maxOffset = std::max(0.0f, contentHeight - viewportHeight);
        const float track = std::max(1.0f, bar.height - thumb.height);
        const float t = std::clamp((y - bar.y - m_dropDownScrollThumbGrabOffsetY) / track, 0.0f, 1.0f);
        ScrollOffsetY = maxOffset * t;
    }

    void ComboBox::EnsureDropDownSelectedVisible()
    {
        if (SelectedIndex < 0 || ItemCount() == 0)
            return;
        const float contentHeight = ItemHeight * static_cast<float>(ItemCount());
        const float viewportHeight = ItemHeight * static_cast<float>(VisibleDropDownItemCount());
        const float maxOffset = std::max(0.0f, contentHeight - viewportHeight);
        const float itemTop = ItemHeight * static_cast<float>(SelectedIndex);
        const float itemBottom = itemTop + ItemHeight;
        if (itemTop < ScrollOffsetY)
            ScrollOffsetY = itemTop;
        else if (itemBottom > ScrollOffsetY + viewportHeight)
            ScrollOffsetY = itemBottom - viewportHeight;
        ScrollOffsetY = std::clamp(ScrollOffsetY, 0.0f, maxOffset);
    }

    void ComboBox::DrawDropDownScrollBar(ImDrawList& drawList)
    {
        if (!NeedsDropDownScrollBar())
            return;

        Rect bar = DropDownScrollBarRect();
        drawList.AddRectFilled({ bar.Left(), bar.Top() }, { bar.Right(), bar.Bottom() }, ColorFromBytes(0, 0, 0, 45), 2.0f);

        Rect thumb = DropDownScrollThumbRect();
        drawList.AddRectFilled({ thumb.Left(), thumb.Top() }, { thumb.Right(), thumb.Bottom() }, ColorFromBytes(96, 94, 92, 160), 2.0f);
    }
}

// ---- End inlined ComboBox control ----


namespace FyGUI
{

    struct TreeViewItem
    {
        std::string Text;
        bool IsExpanded = false;
        std::vector<TreeViewItem> Children;
    };

    class TreeView : public Control
    {
    public:
        std::vector<TreeViewItem> Items;
        int32_t SelectedIndex = -1;
        float ItemHeight = 26.0f;
        std::function<void(int32_t)> OnSelectionChanged;
        TreeViewStyle Style;

        TreeView()
        {
            Width = 260.0f;
            Height = 220.0f;
            BorderThickness = Thickness(1.0f);
            CornerRadius = 3.0f;
            IsTabStop = true;
            VerticalAlignment = FyGUI::VerticalAlignment::Top;
        }

    protected:
        Vec2 MeasureOverride(Vec2) override
        {
            return { Width >= 0.0f ? Width : 260.0f, Height >= 0.0f ? Height : 220.0f };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            VisualState state = GetVisualState();
            if (HasStyledPartVisual(Style.PART_Root))
                DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
            ControlPartStyle background = Style.PART_Background;
            if (!HasStyledPartVisual(background))
            {
                background.BackgroundColor = BackgroundNormal;
                background.BorderColor = BorderBrush;
                background.BorderThickness = BorderThickness.left;
                background.CornerRadius = CornerRadius;
            }
            DrawStyledPart(drawList, Bounds, background, state);
            DrawStyledPart(drawList, Bounds, Style.PART_Border, state);
            DrawStyledPart(drawList, Bounds, Style.PART_ItemsPresenter, state);
            drawList.PushClipRect(Vec2(Bounds.Left(), Bounds.Top()), Vec2(Bounds.Right(), Bounds.Bottom()), true);
            int row = 0;
            int flat = 0;
            for (auto& item : Items)
                RenderItem(drawList, item, 0, row, flat);
            drawList.PopClipRect();
        }

        void OnEvent(EventArgs& args) override
        {
            if (args.kind == EventKind::PointerPressed)
            {
                const auto& pointer = static_cast<PointerEventArgs&>(args);
                int index = static_cast<int>((pointer.position.y - Bounds.y - Padding.top) / ItemHeight);
                int row = 0;
                TreeViewItem* item = index >= 0 ? FindVisibleItemAtRow(Items, index, row) : nullptr;
                if (item)
                {
                    SelectedIndex = index;
                    if (!item->Children.empty() && pointer.position.x <= Bounds.x + Padding.left + 28.0f)
                        item->IsExpanded = !item->IsExpanded;
                    if (OnSelectionChanged)
                        OnSelectionChanged(index);
                }
                args.handled = true;
            }
        }

    private:
        void RenderItem(ImDrawList& drawList, TreeViewItem& item, int depth, int& row, int& flat)
        {
            float y = Bounds.y + Padding.top + ItemHeight * static_cast<float>(row);
            if (y >= Bounds.Bottom())
                return;
            Rect rect = { Bounds.x + Padding.left, y, Bounds.width - Padding.Horizontal(), ItemHeight };
            if (flat == SelectedIndex)
            {
                ControlPartStyle selection = Style.PART_SelectionVisual;
                if (!HasStyledPartVisual(selection))
                {
                    selection.BackgroundColor = { 0.27f, 0.41f, 0.65f, 0.82f };
                    selection.CornerRadius = 2.0f;
                }
                DrawStyledPart(drawList, rect, selection, VisualState::Selected);
            }
            float x = rect.x + 6.0f + static_cast<float>(depth) * 16.0f;
            Color textColor = ResolvePartForeground(Style.PART_Text, Foreground);
            if (!item.Children.empty())
                drawList.AddText(Vec2{ x, y + 5.0f }, textColor.ToU32(Opacity), item.IsExpanded ? "v" : ">");
            drawList.AddText(Vec2{ x + 14.0f, y + 5.0f }, textColor.ToU32(Opacity), item.Text.c_str());
            ++row;
            ++flat;
            if (item.IsExpanded)
            {
                for (auto& child : item.Children)
                    RenderItem(drawList, child, depth + 1, row, flat);
            }
        }

        TreeViewItem* FindVisibleItemAtRow(std::vector<TreeViewItem>& items, int targetRow, int& row)
        {
            for (auto& item : items)
            {
                if (row == targetRow)
                    return &item;
                ++row;
                if (item.IsExpanded)
                {
                    if (TreeViewItem* child = FindVisibleItemAtRow(item.Children, targetRow, row))
                        return child;
                }
            }
            return nullptr;
        }
    };

}


// ---- Begin inlined ScrollViewer control ----
// Inlined after Border and ScrollViewerStyle are declared.
namespace FyGUI
{
    class ScrollViewer : public Border
    {
    public:
        Vec2 Offset = {};
        Vec2 Extent = {};
        Vec2 Viewport = {};
        ScrollViewerStyle ViewerStyle;
        ScrollBarVisibility VerticalScrollBarVisibility = ScrollBarVisibility::Auto;
        ScrollBarVisibility HorizontalScrollBarVisibility = ScrollBarVisibility::Disabled;
        bool ShowScrollBarButtons = false;
        float LineScrollAmount = 96.0f;
        float PageScrollFactor = 0.85f;
        bool IsVerticalScrollBarDragging = false;
        bool IsHorizontalScrollBarDragging = false;
        float VerticalThumbGrabOffset = 0.0f;
        float HorizontalThumbGrabOffset = 0.0f;
        Vec2 TargetOffset = {};
        bool UseSmoothWheelScrolling = false;
        float WheelScrollSmoothing = 48.0f;
        std::function<void(Vec2)> OnViewChanged;

        void ScrollBy(Vec2 delta);

    protected:
        UIElement* HitTest(Vec2 point) override;
        UIElement* HitTestOverlay(Vec2 point) override;
        void Update(float deltaTime) override;
        Vec2 MeasureOverride(Vec2 availableSize) override;
        void ArrangeOverride(Rect finalRect) override;
        void RenderOverride(ImDrawList& drawList) override;
        void OnEvent(EventArgs& args) override;

    private:
        static constexpr float ScrollBarThickness() { return 10.0f; }

        Rect VerticalScrollBarRect() const;
        Rect HorizontalScrollBarRect() const;
        bool ShouldShowVerticalScrollBar() const;
        bool ShouldShowHorizontalScrollBar() const;
        float ButtonExtent() const;
        Rect VerticalDecreaseButtonRect() const;
        Rect VerticalIncreaseButtonRect() const;
        Rect VerticalTrackRect() const;
        Rect HorizontalDecreaseButtonRect() const;
        Rect HorizontalIncreaseButtonRect() const;
        Rect HorizontalTrackRect() const;
        Rect VerticalThumbRect() const;
        Rect VerticalDecreasePageAreaRect(Rect thumb) const;
        Rect VerticalIncreasePageAreaRect(Rect thumb) const;
        Rect HorizontalThumbRect() const;
        Rect HorizontalDecreasePageAreaRect(Rect thumb) const;
        Rect HorizontalIncreasePageAreaRect(Rect thumb) const;
        bool HandleVerticalScrollBarPress(Vec2 point, bool& startDrag);
        bool HandleHorizontalScrollBarPress(Vec2 point, bool& startDrag);
        void UpdateVerticalScrollFromPointer(float y);
        void UpdateHorizontalScrollFromPointer(float x);
        void DrawScrollBarButton(ImDrawList& drawList, Rect rect, ControlPartStyle button, ControlPartStyle glyph, const char* marker, VisualState state);
        void DrawScrollPageArea(ImDrawList& drawList, Rect rect, ControlPartStyle pageArea, VisualState state);
        Rect ContentViewportRect() const;
        Vec2 ClampOffset(Vec2 value) const;
        void SetOffset(Vec2 value);
        void ScrollWheelBy(Vec2 delta);
        void ArrangeChildForOffset();
    };
}

namespace FyGUI
{
    void ScrollViewer::ScrollBy(Vec2 delta)
    {
        SetOffset({ Offset.x + delta.x, Offset.y + delta.y });
    }

    void ScrollViewer::ScrollWheelBy(Vec2 delta)
    {
        if (!UseSmoothWheelScrolling)
        {
            ScrollBy(delta);
            return;
        }
        TargetOffset = ClampOffset({ TargetOffset.x + delta.x, TargetOffset.y + delta.y });
    }

    Vec2 ScrollViewer::ClampOffset(Vec2 value) const
    {
        Vec2 result = value;
        if (HorizontalScrollBarVisibility == ScrollBarVisibility::Disabled || HorizontalScrollBarVisibility == ScrollBarVisibility::Hidden)
            result.x = 0.0f;
        else
            result.x = std::clamp(result.x, 0.0f, std::max(0.0f, Extent.x - Viewport.x));
        if (VerticalScrollBarVisibility == ScrollBarVisibility::Disabled || VerticalScrollBarVisibility == ScrollBarVisibility::Hidden)
            result.y = 0.0f;
        else
            result.y = std::clamp(result.y, 0.0f, std::max(0.0f, Extent.y - Viewport.y));
        return result;
    }

    void ScrollViewer::SetOffset(Vec2 value)
    {
        const Vec2 old = Offset;
        Offset = ClampOffset(value);
        TargetOffset = Offset;
        if (old.x != Offset.x || old.y != Offset.y)
        {
            InvalidateArrange();
            ArrangeChildForOffset();
            ScrollChangedEventArgs args;
            args.kind = EventKind::ScrollChanged;
            args.route = RoutingStrategy::Bubble;
            args.originalSource = this;
            args.oldOffset = old;
            args.newOffset = Offset;
            args.extent = Extent;
            args.viewport = Viewport;
            RaiseEvent(args);
            if (OnViewChanged)
                OnViewChanged(Offset);
        }
    }

    void ScrollViewer::Update(float deltaTime)
    {
        Border::Update(deltaTime);
        TargetOffset = ClampOffset(TargetOffset);
        const float dx = TargetOffset.x - Offset.x;
        const float dy = TargetOffset.y - Offset.y;
        if (std::abs(dx) < 0.05f && std::abs(dy) < 0.05f)
            return;

        const Vec2 target = TargetOffset;
        const float t = std::clamp(deltaTime * WheelScrollSmoothing, 0.0f, 1.0f);
        SetOffset({ Offset.x + dx * t, Offset.y + dy * t });
        TargetOffset = target;
    }

    Rect ScrollViewer::ContentViewportRect() const
    {
        const bool showVertical = ShouldShowVerticalScrollBar();
        const bool showHorizontal = ShouldShowHorizontalScrollBar();
        return {
            Bounds.Left(),
            Bounds.Top(),
            std::max(0.0f, Bounds.width - (showVertical ? ScrollBarThickness() : 0.0f)),
            std::max(0.0f, Bounds.height - (showHorizontal ? ScrollBarThickness() : 0.0f))
        };
    }

    UIElement* ScrollViewer::HitTest(Vec2 point)
    {
        if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible || !Bounds.Contains(point))
            return nullptr;
        if ((ShouldShowVerticalScrollBar() && VerticalScrollBarRect().Contains(point)) ||
            (ShouldShowHorizontalScrollBar() && HorizontalScrollBarRect().Contains(point)))
            return this;

        Rect viewport = ContentViewportRect();
        if (viewport.Contains(point) && Child)
        {
            if (UIElement* overlay = Child->HitTestOverlay(point))
                return overlay;
            if (UIElement* hit = Child->HitTest(point))
                return hit;
        }
        return this;
    }

    UIElement* ScrollViewer::HitTestOverlay(Vec2 point)
    {
        if (Visibility != FyGUI::Visibility::Visible || !IsEnabled || !IsHitTestVisible || !Bounds.Contains(point))
            return nullptr;
        if ((ShouldShowVerticalScrollBar() && VerticalScrollBarRect().Contains(point)) ||
            (ShouldShowHorizontalScrollBar() && HorizontalScrollBarRect().Contains(point)))
            return this;

        Rect viewport = ContentViewportRect();
        return viewport.Contains(point) && Child ? Child->HitTestOverlay(point) : nullptr;
    }

    Vec2 ScrollViewer::MeasureOverride(Vec2 availableSize)
    {
        if (Child)
        {
            const float childWidth = (HorizontalScrollBarVisibility == ScrollBarVisibility::Disabled || HorizontalScrollBarVisibility == ScrollBarVisibility::Hidden)
                ? std::max(0.0f, Width >= 0.0f ? Width : availableSize.x)
                : InfiniteSize;
            const float childHeight = (VerticalScrollBarVisibility == ScrollBarVisibility::Disabled || VerticalScrollBarVisibility == ScrollBarVisibility::Hidden)
                ? std::max(0.0f, Height >= 0.0f ? Height : availableSize.y)
                : InfiniteSize;
            Child->Measure({ childWidth, childHeight });
            Extent = Child->DesiredSize;
        }
        Viewport = {
            Width >= 0.0f ? Width : availableSize.x,
            Height >= 0.0f ? Height : availableSize.y
        };
        return Viewport;
    }

    void ScrollViewer::ArrangeOverride(Rect finalRect)
    {
        if (!Child)
            return;
        Viewport = { finalRect.width, finalRect.height };
        ArrangeChildForOffset();
    }

    void ScrollViewer::ArrangeChildForOffset()
    {
        if (!Child)
            return;
        Rect viewportRect = ContentViewportRect();
        Viewport = { viewportRect.width, viewportRect.height };
        Offset.x = (HorizontalScrollBarVisibility == ScrollBarVisibility::Disabled || HorizontalScrollBarVisibility == ScrollBarVisibility::Hidden)
            ? 0.0f
            : std::clamp(Offset.x, 0.0f, std::max(0.0f, Extent.x - Viewport.x));
        Offset.y = (VerticalScrollBarVisibility == ScrollBarVisibility::Disabled || VerticalScrollBarVisibility == ScrollBarVisibility::Hidden)
            ? 0.0f
            : std::clamp(Offset.y, 0.0f, std::max(0.0f, Extent.y - Viewport.y));
        Child->Arrange({ viewportRect.x - Offset.x, viewportRect.y - Offset.y, std::max(Extent.x, viewportRect.width), std::max(Extent.y, viewportRect.height) });
    }

    void ScrollViewer::RenderOverride(ImDrawList& drawList)
    {
        VisualState state = GetVisualState();
        if (HasStyledPartVisual(ViewerStyle.PART_Root))
            DrawStyledPart(drawList, Bounds, ViewerStyle.PART_Root, state);
        else
            DrawBackgroundAndBorder(drawList);
        DrawStyledPart(drawList, Bounds, ViewerStyle.PART_ContentPresenter, state);
        Rect viewportRect = ContentViewportRect();
        drawList.PushClipRect(Vec2(viewportRect.Left(), viewportRect.Top()), Vec2(viewportRect.Right(), viewportRect.Bottom()), true);
        if (Child)
            Child->Render(drawList);
        drawList.PopClipRect();

        const bool showVertical = ShouldShowVerticalScrollBar();
        const bool showHorizontal = ShouldShowHorizontalScrollBar();

        if (showVertical)
        {
            ControlPartStyle track = ViewerStyle.VerticalScrollBarStyle.PART_Track;
            if (!HasStyledPartVisual(track))
            {
                track.BackgroundColor = ColorFromBytes(0, 0, 0, 0);
                track.CornerRadius = 5.0f;
            }
            DrawStyledPart(drawList, VerticalTrackRect(), track, state);

            Rect thumbRect = VerticalThumbRect();
            DrawScrollPageArea(drawList, VerticalDecreasePageAreaRect(thumbRect), ViewerStyle.VerticalScrollBarStyle.PART_DecreasePageArea, state);
            DrawScrollPageArea(drawList, VerticalIncreasePageAreaRect(thumbRect), ViewerStyle.VerticalScrollBarStyle.PART_IncreasePageArea, state);

            ControlPartStyle thumb = ViewerStyle.VerticalScrollBarStyle.PART_Thumb;
            if (!HasStyledPartVisual(thumb))
            {
                thumb.BackgroundColor = IsVerticalScrollBarDragging ? ColorFromBytes(96, 94, 92, 220) : ColorFromBytes(96, 94, 92, 150);
                thumb.CornerRadius = 5.0f;
            }
            DrawStyledPart(drawList, thumbRect, thumb, IsVerticalScrollBarDragging ? VisualState::Pressed : state);

            DrawScrollBarButton(drawList, VerticalDecreaseButtonRect(), ViewerStyle.VerticalScrollBarStyle.PART_DecreaseButton, ViewerStyle.VerticalScrollBarStyle.PART_DecreaseGlyph, "^", state);
            DrawScrollBarButton(drawList, VerticalIncreaseButtonRect(), ViewerStyle.VerticalScrollBarStyle.PART_IncreaseButton, ViewerStyle.VerticalScrollBarStyle.PART_IncreaseGlyph, "v", state);
        }

        if (showHorizontal)
        {
            ControlPartStyle track = ViewerStyle.HorizontalScrollBarStyle.PART_Track;
            if (!HasStyledPartVisual(track))
            {
                track.BackgroundColor = ColorFromBytes(0, 0, 0, 0);
                track.CornerRadius = 5.0f;
            }
            DrawStyledPart(drawList, HorizontalTrackRect(), track, state);

            Rect thumbRect = HorizontalThumbRect();
            DrawScrollPageArea(drawList, HorizontalDecreasePageAreaRect(thumbRect), ViewerStyle.HorizontalScrollBarStyle.PART_DecreasePageArea, state);
            DrawScrollPageArea(drawList, HorizontalIncreasePageAreaRect(thumbRect), ViewerStyle.HorizontalScrollBarStyle.PART_IncreasePageArea, state);

            ControlPartStyle thumb = ViewerStyle.HorizontalScrollBarStyle.PART_Thumb;
            if (!HasStyledPartVisual(thumb))
            {
                thumb.BackgroundColor = IsHorizontalScrollBarDragging ? ColorFromBytes(96, 94, 92, 220) : ColorFromBytes(96, 94, 92, 150);
                thumb.CornerRadius = 5.0f;
            }
            DrawStyledPart(drawList, thumbRect, thumb, IsHorizontalScrollBarDragging ? VisualState::Pressed : state);

            DrawScrollBarButton(drawList, HorizontalDecreaseButtonRect(), ViewerStyle.HorizontalScrollBarStyle.PART_DecreaseButton, ViewerStyle.HorizontalScrollBarStyle.PART_DecreaseGlyph, "<", state);
            DrawScrollBarButton(drawList, HorizontalIncreaseButtonRect(), ViewerStyle.HorizontalScrollBarStyle.PART_IncreaseButton, ViewerStyle.HorizontalScrollBarStyle.PART_IncreaseGlyph, ">", state);
        }

        if (showHorizontal && showVertical)
        {
            ControlPartStyle corner = ViewerStyle.PART_ScrollCorner;
            if (!HasStyledPartVisual(corner))
                corner.BackgroundColor = ColorFromBytes(0, 0, 0, 0);
            DrawStyledPart(drawList, { Bounds.Right() - ScrollBarThickness(), Bounds.Bottom() - ScrollBarThickness(), ScrollBarThickness(), ScrollBarThickness() }, corner, state);
        }
        if (Child)
            Child->RenderOverlay(drawList);
    }

    void ScrollViewer::OnEvent(EventArgs& args)
    {
        if (args.kind == EventKind::PointerWheelChanged)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            ScrollWheelBy({ -pointer.wheelDelta.x * LineScrollAmount, -pointer.wheelDelta.y * LineScrollAmount });
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerPressed)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            bool startDrag = false;
            if (HandleVerticalScrollBarPress(pointer.position, startDrag))
            {
                IsVerticalScrollBarDragging = startDrag;
                if (startDrag)
                    CapturePointer(pointer.pointerId);
                args.handled = true;
            }
            else if (HandleHorizontalScrollBarPress(pointer.position, startDrag))
            {
                IsHorizontalScrollBarDragging = startDrag;
                if (startDrag)
                    CapturePointer(pointer.pointerId);
                args.handled = true;
            }
        }
        else if (args.kind == EventKind::PointerMoved && IsVerticalScrollBarDragging)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            UpdateVerticalScrollFromPointer(pointer.position.y);
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerMoved && IsHorizontalScrollBarDragging)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            UpdateHorizontalScrollFromPointer(pointer.position.x);
            args.handled = true;
        }
        else if (args.kind == EventKind::PointerReleased)
        {
            IsVerticalScrollBarDragging = false;
            IsHorizontalScrollBarDragging = false;
            VerticalThumbGrabOffset = 0.0f;
            HorizontalThumbGrabOffset = 0.0f;
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            ReleasePointer(pointer.pointerId);
        }
    }

    Rect ScrollViewer::VerticalScrollBarRect() const
    {
        const float bottomReserve = ShouldShowHorizontalScrollBar() ? ScrollBarThickness() : 0.0f;
        return { Bounds.Right() - ScrollBarThickness(), Bounds.Top(), ScrollBarThickness(), std::max(0.0f, Bounds.height - bottomReserve) };
    }

    Rect ScrollViewer::HorizontalScrollBarRect() const
    {
        const float rightReserve = ShouldShowVerticalScrollBar() ? ScrollBarThickness() : 0.0f;
        return { Bounds.Left(), Bounds.Bottom() - ScrollBarThickness(), std::max(0.0f, Bounds.width - rightReserve), ScrollBarThickness() };
    }

    bool ScrollViewer::ShouldShowVerticalScrollBar() const
    {
        if (VerticalScrollBarVisibility == ScrollBarVisibility::Disabled || VerticalScrollBarVisibility == ScrollBarVisibility::Hidden)
            return false;
        return VerticalScrollBarVisibility == ScrollBarVisibility::Visible || Extent.y > Viewport.y;
    }

    bool ScrollViewer::ShouldShowHorizontalScrollBar() const
    {
        if (HorizontalScrollBarVisibility == ScrollBarVisibility::Disabled || HorizontalScrollBarVisibility == ScrollBarVisibility::Hidden)
            return false;
        return HorizontalScrollBarVisibility == ScrollBarVisibility::Visible || Extent.x > Viewport.x;
    }

    float ScrollViewer::ButtonExtent() const
    {
        return ShowScrollBarButtons ? ScrollBarThickness() : 0.0f;
    }

    Rect ScrollViewer::VerticalDecreaseButtonRect() const
    {
        Rect bar = VerticalScrollBarRect();
        return { bar.x, bar.y, bar.width, ButtonExtent() };
    }

    Rect ScrollViewer::VerticalIncreaseButtonRect() const
    {
        Rect bar = VerticalScrollBarRect();
        const float button = ButtonExtent();
        return { bar.x, bar.Bottom() - button, bar.width, button };
    }

    Rect ScrollViewer::VerticalTrackRect() const
    {
        Rect bar = VerticalScrollBarRect();
        const float button = ButtonExtent();
        return { bar.x, bar.y + button, bar.width, std::max(0.0f, bar.height - button * 2.0f) };
    }

    Rect ScrollViewer::HorizontalDecreaseButtonRect() const
    {
        Rect bar = HorizontalScrollBarRect();
        return { bar.x, bar.y, ButtonExtent(), bar.height };
    }

    Rect ScrollViewer::HorizontalIncreaseButtonRect() const
    {
        Rect bar = HorizontalScrollBarRect();
        const float button = ButtonExtent();
        return { bar.Right() - button, bar.y, button, bar.height };
    }

    Rect ScrollViewer::HorizontalTrackRect() const
    {
        Rect bar = HorizontalScrollBarRect();
        const float button = ButtonExtent();
        return { bar.x + button, bar.y, std::max(0.0f, bar.width - button * 2.0f), bar.height };
    }

    Rect ScrollViewer::VerticalThumbRect() const
    {
        Rect bar = VerticalTrackRect();
        const float ratio = std::clamp(Viewport.y / std::max(1.0f, Extent.y), 0.05f, 1.0f);
        const float thumbH = std::min(bar.height, std::max(0.0f, std::max(18.0f, bar.height * ratio)));
        const float maxOffset = std::max(1.0f, Extent.y - Viewport.y);
        const float thumbY = bar.y + (bar.height - thumbH) * std::clamp(Offset.y / maxOffset, 0.0f, 1.0f);
        return { bar.x + 1.0f, thumbY, 8.0f, thumbH };
    }

    Rect ScrollViewer::VerticalDecreasePageAreaRect(Rect thumb) const
    {
        Rect track = VerticalTrackRect();
        return { track.x, track.y, track.width, std::max(0.0f, thumb.y - track.y) };
    }

    Rect ScrollViewer::VerticalIncreasePageAreaRect(Rect thumb) const
    {
        Rect track = VerticalTrackRect();
        return { track.x, thumb.Bottom(), track.width, std::max(0.0f, track.Bottom() - thumb.Bottom()) };
    }

    Rect ScrollViewer::HorizontalThumbRect() const
    {
        Rect bar = HorizontalTrackRect();
        const float ratio = std::clamp(Viewport.x / std::max(1.0f, Extent.x), 0.05f, 1.0f);
        const float thumbW = std::min(bar.width, std::max(0.0f, std::max(18.0f, bar.width * ratio)));
        const float maxOffset = std::max(1.0f, Extent.x - Viewport.x);
        const float thumbX = bar.x + (bar.width - thumbW) * std::clamp(Offset.x / maxOffset, 0.0f, 1.0f);
        return { thumbX, bar.y + 1.0f, thumbW, 8.0f };
    }

    Rect ScrollViewer::HorizontalDecreasePageAreaRect(Rect thumb) const
    {
        Rect track = HorizontalTrackRect();
        return { track.x, track.y, std::max(0.0f, thumb.x - track.x), track.height };
    }

    Rect ScrollViewer::HorizontalIncreasePageAreaRect(Rect thumb) const
    {
        Rect track = HorizontalTrackRect();
        return { thumb.Right(), track.y, std::max(0.0f, track.Right() - thumb.Right()), track.height };
    }

    bool ScrollViewer::HandleVerticalScrollBarPress(Vec2 point, bool& startDrag)
    {
        startDrag = false;
        if (!ShouldShowVerticalScrollBar())
            return false;
        Rect bar = VerticalScrollBarRect();
        if (!bar.Contains(point))
            return false;

        if (ShowScrollBarButtons && VerticalDecreaseButtonRect().Contains(point))
        {
            ScrollBy({ 0.0f, -LineScrollAmount });
            return true;
        }
        if (ShowScrollBarButtons && VerticalIncreaseButtonRect().Contains(point))
        {
            ScrollBy({ 0.0f, LineScrollAmount });
            return true;
        }

        Rect thumb = VerticalThumbRect();
        if (thumb.Contains(point))
        {
            startDrag = true;
            VerticalThumbGrabOffset = point.y - thumb.y;
            UpdateVerticalScrollFromPointer(point.y);
            return true;
        }

        ScrollBy({ 0.0f, point.y < thumb.y ? -Viewport.y * PageScrollFactor : Viewport.y * PageScrollFactor });
        return true;
    }

    bool ScrollViewer::HandleHorizontalScrollBarPress(Vec2 point, bool& startDrag)
    {
        startDrag = false;
        if (!ShouldShowHorizontalScrollBar())
            return false;
        Rect bar = HorizontalScrollBarRect();
        if (!bar.Contains(point))
            return false;

        if (ShowScrollBarButtons && HorizontalDecreaseButtonRect().Contains(point))
        {
            ScrollBy({ -LineScrollAmount, 0.0f });
            return true;
        }
        if (ShowScrollBarButtons && HorizontalIncreaseButtonRect().Contains(point))
        {
            ScrollBy({ LineScrollAmount, 0.0f });
            return true;
        }

        Rect thumb = HorizontalThumbRect();
        if (thumb.Contains(point))
        {
            startDrag = true;
            HorizontalThumbGrabOffset = point.x - thumb.x;
            UpdateHorizontalScrollFromPointer(point.x);
            return true;
        }

        ScrollBy({ point.x < thumb.x ? -Viewport.x * PageScrollFactor : Viewport.x * PageScrollFactor, 0.0f });
        return true;
    }

    void ScrollViewer::UpdateVerticalScrollFromPointer(float y)
    {
        Rect bar = VerticalTrackRect();
        Rect thumb = VerticalThumbRect();
        const float maxOffset = std::max(0.0f, Extent.y - Viewport.y);
        const float track = std::max(1.0f, bar.height - thumb.height);
        const float t = std::clamp((y - VerticalThumbGrabOffset - bar.y) / track, 0.0f, 1.0f);
        SetOffset({ Offset.x, maxOffset * t });
    }

    void ScrollViewer::UpdateHorizontalScrollFromPointer(float x)
    {
        Rect bar = HorizontalTrackRect();
        Rect thumb = HorizontalThumbRect();
        const float maxOffset = std::max(0.0f, Extent.x - Viewport.x);
        const float track = std::max(1.0f, bar.width - thumb.width);
        const float t = std::clamp((x - HorizontalThumbGrabOffset - bar.x) / track, 0.0f, 1.0f);
        SetOffset({ maxOffset * t, Offset.y });
    }

    void ScrollViewer::DrawScrollBarButton(ImDrawList& drawList, Rect rect, ControlPartStyle button, ControlPartStyle glyph, const char* marker, VisualState state)
    {
        if (!ShowScrollBarButtons || rect.width <= 0.0f || rect.height <= 0.0f)
            return;
        if (!HasStyledPartVisual(button))
        {
            button.BackgroundColor = { 0.10f, 0.12f, 0.16f, 0.86f };
            button.BorderColor = { 0.30f, 0.36f, 0.46f, 0.80f };
            button.BorderThickness = 1.0f;
            button.CornerRadius = 3.0f;
        }
        DrawStyledPart(drawList, rect, button, state);
        Rect glyphRect = { rect.x + rect.width * 0.22f, rect.y + rect.height * 0.22f, rect.width * 0.56f, rect.height * 0.56f };
        if (HasStyledPartVisual(glyph))
        {
            DrawStyledPart(drawList, glyphRect, glyph, state);
            if (glyph.UseImage || glyph.UseNineSlice || glyph.BackgroundColor.a > 0.0f)
                return;
        }
        Color glyphColor = glyph.ForegroundColor.a > 0.0f ? glyph.ForegroundColor : Color{ 0.82f, 0.88f, 0.96f, 1.0f };
        const Vec2 center = { rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f };
        const float s = std::max(3.0f, std::min(rect.width, rect.height) * 0.22f);
        if (marker && marker[0] == '^')
        {
            drawList.AddLine({ center.x - s, center.y + s * 0.45f }, { center.x, center.y - s * 0.55f }, glyphColor, 1.25f);
            drawList.AddLine({ center.x, center.y - s * 0.55f }, { center.x + s, center.y + s * 0.45f }, glyphColor, 1.25f);
        }
        else if (marker && marker[0] == 'v')
        {
            drawList.AddLine({ center.x - s, center.y - s * 0.45f }, { center.x, center.y + s * 0.55f }, glyphColor, 1.25f);
            drawList.AddLine({ center.x, center.y + s * 0.55f }, { center.x + s, center.y - s * 0.45f }, glyphColor, 1.25f);
        }
        else if (marker && marker[0] == '<')
        {
            drawList.AddLine({ center.x + s * 0.45f, center.y - s }, { center.x - s * 0.55f, center.y }, glyphColor, 1.25f);
            drawList.AddLine({ center.x - s * 0.55f, center.y }, { center.x + s * 0.45f, center.y + s }, glyphColor, 1.25f);
        }
        else
        {
            drawList.AddLine({ center.x - s * 0.45f, center.y - s }, { center.x + s * 0.55f, center.y }, glyphColor, 1.25f);
            drawList.AddLine({ center.x + s * 0.55f, center.y }, { center.x - s * 0.45f, center.y + s }, glyphColor, 1.25f);
        }
    }

    void ScrollViewer::DrawScrollPageArea(ImDrawList& drawList, Rect rect, ControlPartStyle pageArea, VisualState state)
    {
        if (rect.width <= 0.0f || rect.height <= 0.0f)
            return;
        if (!HasStyledPartVisual(pageArea))
        {
            pageArea.BackgroundColor = { 0.75f, 0.82f, 0.92f, 0.035f };
            pageArea.CornerRadius = 3.0f;
        }
        DrawStyledPart(drawList, rect, pageArea, state);
    }
}

// ---- End inlined ScrollViewer control ----


namespace FyGUI
{

    class ItemsControl : public StackPanel
    {
    public:
        std::vector<std::string> Items;
        std::function<std::shared_ptr<UIElement>(const std::string&, int32_t)> ItemTemplate;

        void RefreshItems()
        {
            Children.clear();
            for (int32_t i = 0; i < static_cast<int32_t>(Items.size()); ++i)
            {
                std::shared_ptr<UIElement> child;
                if (ItemTemplate)
                    child = ItemTemplate(Items[static_cast<size_t>(i)], i);
                else
                    child = std::make_shared<TextBlock>(Items[static_cast<size_t>(i)]);
                AddChild(child);
            }
        }
    };

    class ScrollBar : public RangeBase
    {
    public:
        FyGUI::Orientation Orientation = FyGUI::Orientation::Vertical;
        double ViewportSize = 0.2;
        ScrollBarStyle Style;

        ScrollBar()
        {
            Width = 14.0f;
            Height = 160.0f;
            IsTabStop = true;
            BackgroundNormal = { 0.08f, 0.09f, 0.11f, 0.80f };
            BackgroundHover = BackgroundNormal;
            BackgroundPressed = BackgroundNormal;
        }

        void SetValue(double value) override
        {
            double old = Value;
            Value = std::clamp(value, Minimum, Maximum);
            if (IsPressed || !AnimateValueChanges || !m_hasAnimatedValue)
                AnimatedValue = Value;
            if (old != Value && OnValueChanged)
                OnValueChanged(Value);
        }

    protected:
        void Update(float deltaTime) override
        {
            Control::Update(deltaTime);
            if (!m_hasAnimatedValue)
            {
                AnimatedValue = Value;
                m_hasAnimatedValue = true;
                return;
            }

            if (IsPressed)
            {
                AnimatedValue = Value;
                return;
            }

            const float step = std::clamp(deltaTime * ValueTransitionSpeed, 0.0f, 1.0f);
            AnimatedValue += (Value - AnimatedValue) * static_cast<double>(step);
            if (std::abs(AnimatedValue - Value) < 0.0001)
                AnimatedValue = Value;
        }

        Vec2 MeasureOverride(Vec2) override
        {
            return { Width >= 0.0f ? Width : 14.0f, Height >= 0.0f ? Height : 160.0f };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            VisualState state = GetVisualState();
            DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
            ControlPartStyle track = Style.PART_Track;
            if (!HasStyledPartVisual(track))
                track.BackgroundColor = BackgroundNormal;
            DrawStyledPart(drawList, Bounds, track, state);

            const bool vertical = Orientation == FyGUI::Orientation::Vertical;
            const float length = vertical ? Bounds.height : Bounds.width;
            const float thickness = vertical ? Bounds.width : Bounds.height;
            const float thumbLength = std::min(length, std::max(0.0f, std::max(18.0f, static_cast<float>(ViewportSize) * length)));
            const double range = std::max(0.0001, Maximum - Minimum);
            const float t = static_cast<float>((AnimatedValue - Minimum) / range);
            Rect thumbRect = vertical
                ? Rect{ Bounds.x, Bounds.y + (length - thumbLength) * std::clamp(t, 0.0f, 1.0f), thickness, thumbLength }
                : Rect{ Bounds.x + (length - thumbLength) * std::clamp(t, 0.0f, 1.0f), Bounds.y, thumbLength, thickness };

            ControlPartStyle thumb = Style.PART_Thumb;
            if (!HasStyledPartVisual(thumb))
            {
                thumb.BackgroundColor = { 0.50f, 0.58f, 0.70f, 0.90f };
                thumb.CornerRadius = thickness * 0.35f;
            }
            DrawStyledPart(drawList, thumbRect, thumb, state);
            DrawStyledPart(drawList, thumbRect, Style.PART_ThumbGrip, state);
        }

        void OnEvent(EventArgs& args) override
        {
            if (args.kind == EventKind::PointerPressed || args.kind == EventKind::PointerMoved)
            {
                if (args.kind == EventKind::PointerMoved && !IsPressed)
                    return;
                const auto& pointer = static_cast<PointerEventArgs&>(args);
                IsPressed = true;
                if (args.kind == EventKind::PointerPressed)
                    CapturePointer(pointer.pointerId);
                const bool vertical = Orientation == FyGUI::Orientation::Vertical;
                const float pos = vertical ? pointer.position.y - Bounds.y : pointer.position.x - Bounds.x;
                const float length = vertical ? Bounds.height : Bounds.width;
                const float t = length > 0.0f ? std::clamp(pos / length, 0.0f, 1.0f) : 0.0f;
                SetValue(Minimum + (Maximum - Minimum) * t);
                args.handled = true;
            }
            else if (args.kind == EventKind::PointerReleased)
            {
                IsPressed = false;
                ReleasePointer(static_cast<PointerEventArgs&>(args).pointerId);
            }
        }

    private:
        bool m_hasAnimatedValue = false;
    };

    class InventorySlot : public Control
    {
    public:
        using ItemVisualRendererCallback = std::function<void(ImDrawList&, const Rect&, const InventorySlot&, VisualState)>;

        ImageSource ItemIcon;
        std::string ItemName;
        std::string QuantityText;
        std::string BadgeText;
        std::string Rarity;
        Color ItemTint = { 1.0f, 1.0f, 1.0f, 1.0f };
        bool HasItemTint = false;
        ItemVisualRendererCallback ItemVisualRenderer;
        double Cooldown = 0.0;
        bool IsSelected = false;
        bool IsDropHighlighted = false;
        InventorySlotStyle Style;
        std::function<bool()> OnUseRequested;

        InventorySlot()
        {
            Width = 64.0f;
            Height = 64.0f;
            IsTabStop = true;
            BorderThickness = Thickness(1.0f);
            CornerRadius = 6.0f;
            BackgroundNormal = { 0.05f, 0.06f, 0.08f, 0.92f };
            BackgroundHover = { 0.08f, 0.10f, 0.13f, 0.96f };
            BackgroundPressed = { 0.03f, 0.04f, 0.06f, 1.0f };
            BorderBrush = { 0.30f, 0.36f, 0.46f, 0.90f };
        }

        VisualState GetVisualState() const override
        {
            if (!IsEnabled)
                return VisualState::Disabled;
            if (IsPressed)
                return VisualState::Pressed;
            if (IsSelected)
                return VisualState::Selected;
            if (IsHovered)
                return VisualState::PointerOver;
            if (IsFocused())
                return VisualState::Focused;
            return VisualState::Normal;
        }

    protected:
        void Update(float deltaTime) override
        {
            Control::Update(deltaTime);
            const float target = IsDropHighlighted ? 1.0f : 0.0f;
            const float step = std::clamp(deltaTime * 18.0f, 0.0f, 1.0f);
            m_dropTransition += (target - m_dropTransition) * step;
            if (std::abs(m_dropTransition - target) < 0.001f)
                m_dropTransition = target;
        }

        Vec2 MeasureOverride(Vec2) override
        {
            return { Width >= 0.0f ? Width : 64.0f, Height >= 0.0f ? Height : 64.0f };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            VisualState state = GetVisualState();
            const float dropGlow = EaseOutCubic(m_dropTransition);
            Rect visualBounds = AnimatedVisualBounds(2.0f, 1.2f);
            if (dropGlow > 0.0f)
                visualBounds = ScaleRectFromCenter(visualBounds, 1.0f + dropGlow * 0.055f);

            if (dropGlow > 0.001f)
            {
                const Color glow = Rarity.empty() ? ColorFromBytes(0, 180, 170, 255) : RarityAccentColor(Rarity);
                for (int i = 2; i >= 1; --i)
                {
                    const float grow = static_cast<float>(i) * (2.0f + dropGlow * 3.5f);
                    drawList.AddRect({ visualBounds.Left() - grow, visualBounds.Top() - grow }, { visualBounds.Right() + grow, visualBounds.Bottom() + grow }, Color{ glow.r, glow.g, glow.b, 0.13f * dropGlow }.ToU32(Opacity), CornerRadius + grow, ImDrawFlags_None, 2.0f);
                }
            }

            DrawStyledPart(drawList, visualBounds, Style.PART_Root, state);
            ControlPartStyle bg = Style.PART_Background;
            if (!HasStyledPartVisual(bg))
            {
                bg.BackgroundColor = IsPressed ? BackgroundPressed : (IsHovered ? BackgroundHover : BackgroundNormal);
                bg.BorderColor = BorderBrush;
                bg.BorderThickness = BorderThickness.left;
                bg.CornerRadius = CornerRadius;
            }
            if (dropGlow > 0.001f)
            {
                const Color accent = Rarity.empty() ? ColorFromBytes(0, 180, 170, 255) : RarityAccentColor(Rarity);
                bg.BackgroundColor = LerpColor(bg.BackgroundColor, Color{ accent.r, accent.g, accent.b, 0.22f }, 0.45f * dropGlow);
                bg.BorderColor = LerpColor(bg.BorderColor, accent, dropGlow);
                bg.BorderThickness = std::max(bg.BorderThickness, 1.5f + dropGlow * 1.5f);
            }
            bg = ApplyStateTransition(bg);
            DrawStyledPart(drawList, visualBounds, bg, state);
            DrawStyledPart(drawList, visualBounds, ApplyStateTransition(Style.PART_Border), state);

            Rect iconRect = { visualBounds.x + 8.0f, visualBounds.y + 8.0f, std::max(0.0f, visualBounds.width - 16.0f), std::max(0.0f, visualBounds.height - 16.0f) };
            ControlPartStyle icon = Style.PART_ItemIcon;
            if (!icon.Image)
                icon.Image = ItemIcon.texture;
            icon.UseImage = icon.UseImage || icon.Image != 0;
            DrawStyledPart(drawList, iconRect, icon, state, HasItemTint ? ItemTint : Color{ 1.0f, 1.0f, 1.0f, 1.0f });
            const bool hasImage = icon.UseImage && ResolveImageForState(icon, state) != 0;
            if (!hasImage && ItemVisualRenderer)
                ItemVisualRenderer(drawList, iconRect, *this, state);

            ControlPartStyle rarityFrame = Style.PART_RarityFrame;
            if (!Rarity.empty() && !HasStyledPartVisual(rarityFrame))
            {
                const Color accent = RarityAccentColor(Rarity);
                rarityFrame.BorderColor = accent;
                rarityFrame.BorderThickness = 2.0f;
                rarityFrame.CornerRadius = CornerRadius;
                rarityFrame.BackgroundColor = { accent.r, accent.g, accent.b, 0.06f };
            }
            DrawStyledPart(drawList, visualBounds, ApplyStateTransition(rarityFrame), state);

            if (Cooldown > 0.0)
            {
                Rect cooldownRect = visualBounds;
                cooldownRect.y = visualBounds.y + visualBounds.height * static_cast<float>(1.0 - std::clamp(Cooldown, 0.0, 1.0));
                cooldownRect.height = visualBounds.Bottom() - cooldownRect.y;
                ControlPartStyle cooldown = Style.PART_CooldownOverlay;
                if (!HasStyledPartVisual(cooldown))
                    cooldown.BackgroundColor = { 0.0f, 0.0f, 0.0f, 0.55f };
                DrawStyledPart(drawList, cooldownRect, cooldown, state);
            }

            if (IsSelected)
                DrawStyledPart(drawList, visualBounds, ApplyPartOpacity(Style.PART_SelectionGlow, 0.62f + 0.38f * std::max(FocusTransition(), HoverTransition())), VisualState::Selected);
            if (IsHovered || IsDropHighlighted || dropGlow > 0.001f)
                DrawStyledPart(drawList, visualBounds, ApplyPartOpacity(Style.PART_HoverGlow, std::max(dropGlow, std::max(0.25f, HoverTransition()))), VisualState::PointerOver);
            if (!IsEnabled)
                DrawStyledPart(drawList, visualBounds, Style.PART_DisabledOverlay, VisualState::Disabled);

            if (!QuantityText.empty())
            {
                ControlPartStyle quantity = Style.PART_QuantityText;
                Color textColor = ResolvePartForeground(quantity, ColorFromBytes(245, 248, 255));
                Vec2 size = MeasureText(QuantityText);
                const Vec2 textPos = Vec2(Bounds.Right() - size.x - 5.0f, Bounds.Bottom() - size.y - 4.0f);
                const Rect chip = { textPos.x - 4.0f, textPos.y - 2.0f, size.x + 7.0f, size.y + 4.0f };
                drawList.AddRectFilled({ chip.Left(), chip.Top() }, { chip.Right(), chip.Bottom() }, ColorFromBytes(0, 0, 0, 92), 4.0f);
                drawList.AddText(textPos, textColor.ToU32(Opacity), QuantityText.c_str());
            }

            if (!BadgeText.empty())
            {
                Vec2 badgeTextSize = MeasureText(BadgeText);
                Rect badgeRect = { Bounds.x + 5.0f, Bounds.y + 5.0f, std::max(22.0f, badgeTextSize.x + 10.0f), badgeTextSize.y + 6.0f };
                const Color accent = Rarity.empty() ? Color{ 0.38f, 0.72f, 1.0f, 1.0f } : RarityAccentColor(Rarity);
                drawList.AddRectFilled(Vec2(badgeRect.Left(), badgeRect.Top()), Vec2(badgeRect.Right(), badgeRect.Bottom()), Color{ accent.r, accent.g, accent.b, 0.82f }.ToU32(Opacity), badgeRect.height * 0.45f);
                drawList.AddText(Vec2(badgeRect.x + (badgeRect.width - badgeTextSize.x) * 0.5f, badgeRect.y + 3.0f), ColorFromBytes(10, 16, 24, 255), BadgeText.c_str());
            }
        }

        void OnEvent(EventArgs& args) override
        {
            if (args.kind == EventKind::PointerEntered)
                IsHovered = true;
            else if (args.kind == EventKind::PointerExited)
                IsHovered = false;
            else if (args.kind == EventKind::PointerPressed)
            {
                IsPressed = true;
                IsSelected = true;
                args.handled = true;
            }
            else if (args.kind == EventKind::PointerReleased)
            {
                IsPressed = false;
                args.handled = true;
            }
            else if (args.kind == EventKind::DragEnter)
            {
                IsDropHighlighted = true;
                args.handled = true;
            }
            else if (args.kind == EventKind::DragLeave || args.kind == EventKind::Drop || args.kind == EventKind::DragCompleted)
            {
                IsDropHighlighted = false;
            }
        }

        bool OnAction(UIAction action) override
        {
            if (action != UIAction::Accept)
                return false;
            return OnUseRequested ? OnUseRequested() : false;
        }

    private:
        float m_dropTransition = 0.0f;
    };

    struct InventoryItem
    {
        std::string Name;
        ImageSource Icon;
        std::string QuantityText;
        std::string BadgeText;
        std::string Rarity;
        Color Tint = { 1.0f, 1.0f, 1.0f, 1.0f };
        bool HasTint = false;
        InventorySlot::ItemVisualRendererCallback CustomVisualRenderer;
        int32_t StackCount = 1;
        int32_t MaxStack = 1;
        double Cooldown = 0.0;
        bool IsEmpty = true;
        void* UserData = nullptr;
    };

    enum class InventoryTransferMode : uint8_t
    {
        Swap,
        Move,
        Copy
    };

    class InventoryGrid : public WrapPanel
    {
    public:
        std::vector<InventoryItem> Items;
        std::vector<std::shared_ptr<InventorySlot>> Slots;
        InventoryGridStyle Style;
        float SlotWidth = 58.0f;
        float SlotHeight = 58.0f;
        int32_t SelectedIndex = -1;
        bool AllowSwap = true;
        bool AllowMove = true;
        bool AllowCopy = false;
        bool AllowSplitStack = true;
        InventoryTransferMode TransferMode = InventoryTransferMode::Swap;
        std::string SlotTemplateName = "Default";
        std::function<bool(int32_t, int32_t)> CanSwap;
        std::function<void(int32_t, int32_t)> OnSwap;
        std::function<void(int32_t, int32_t)> OnMove;
        std::function<void(int32_t, int32_t)> OnCopy;
        std::function<void(int32_t, int32_t)> OnDrop;
        std::function<void(int32_t, int32_t, int32_t)> OnSplitStack;
        std::function<void(int32_t)> OnUse;
        std::function<void(int32_t)> OnInspect;
        std::function<void(int32_t)> OnDragStarted;
        std::function<void(int32_t)> OnDragCompleted;
        std::function<void(int32_t)> OnSelectionChanged;
        std::function<std::shared_ptr<InventorySlot>(const InventoryItem&, int32_t)> SlotTemplate;

        InventoryGrid()
        {
            Width = 360.0f;
            Spacing = 10.0f;
            LineSpacing = 10.0f;
            IsTabStop = true;
        }

        void SetItems(std::vector<InventoryItem> items)
        {
            Items = std::move(items);
            RebuildSlots();
        }

        void RebuildSlots()
        {
            Children.clear();
            Slots.clear();
            Slots.reserve(Items.size());

            for (int32_t i = 0; i < static_cast<int32_t>(Items.size()); ++i)
            {
                auto slot = SlotTemplate ? SlotTemplate(Items[static_cast<size_t>(i)], i) : std::make_shared<InventorySlot>();
                if (!slot)
                    slot = std::make_shared<InventorySlot>();
                slot->Width = SlotWidth;
                slot->Height = SlotHeight;
                slot->Foreground = Foreground;
                slot->Style = Style.SlotStyle;
                slot->AllowDrop = true;
                slot->IsDragSource = true;
                slot->DragPayload = SlotPayload(i);
                slot->Name = Items[static_cast<size_t>(i)].Name;
                slot->ToolTip = Items[static_cast<size_t>(i)].Name.empty() ? "Inventory slot" : "Drag " + Items[static_cast<size_t>(i)].Name + " onto another slot.";
                slot->OnUseRequested = [this, i]()
                {
                    SetSelectedIndex(i);
                    return UseSelected();
                };

                slot->AddHandler(EventKind::Drop, [this, i](UIElement&, EventArgs& args)
                {
                    auto& drop = static_cast<DragDropEventArgs&>(args);
                    int32_t source = -1;
                    if (!TryParseSlotPayload(drop.payload, source))
                        return;
                    TryTransfer(source, i);
                    args.handled = true;
                }, RoutingStrategy::Bubble, true);

                slot->AddHandler(EventKind::PointerPressed, [this, i](UIElement&, EventArgs&)
                {
                    SetSelectedIndex(i);
                }, RoutingStrategy::Bubble, true);

                slot->AddHandler(EventKind::DragStarted, [this, i](UIElement&, EventArgs&)
                {
                    SetSelectedIndex(i);
                    if (OnDragStarted)
                        OnDragStarted(i);
                }, RoutingStrategy::Bubble, true);

                slot->AddHandler(EventKind::DragCompleted, [this, i](UIElement&, EventArgs&)
                {
                    if (OnDragCompleted)
                        OnDragCompleted(i);
                }, RoutingStrategy::Bubble, true);

                slot->AddHandler(EventKind::GotFocus, [this, i](UIElement&, EventArgs&)
                {
                    SetSelectedIndex(i);
                }, RoutingStrategy::Bubble, true);

                Slots.push_back(slot);
                AddChild(slot);
            }

            RefreshSlotVisuals();
        }

        void SetSelectedIndex(int32_t index)
        {
            const int32_t clamped = Items.empty() ? -1 : std::clamp(index, 0, static_cast<int32_t>(Items.size()) - 1);
            if (SelectedIndex == clamped)
                return;
            SelectedIndex = clamped;
            RefreshSlotVisuals();
            if (OnSelectionChanged)
                OnSelectionChanged(SelectedIndex);
        }

        bool TrySwap(int32_t sourceIndex, int32_t targetIndex)
        {
            InventoryTransferMode previous = TransferMode;
            TransferMode = InventoryTransferMode::Swap;
            bool result = TryTransfer(sourceIndex, targetIndex);
            TransferMode = previous;
            return result;
        }

        bool TryTransfer(int32_t sourceIndex, int32_t targetIndex)
        {
            if (sourceIndex == targetIndex)
                return false;
            if (sourceIndex < 0 || targetIndex < 0 || sourceIndex >= static_cast<int32_t>(Items.size()) || targetIndex >= static_cast<int32_t>(Items.size()))
                return false;
            if (CanSwap && !CanSwap(sourceIndex, targetIndex))
                return false;

            auto& source = Items[static_cast<size_t>(sourceIndex)];
            auto& target = Items[static_cast<size_t>(targetIndex)];
            if (source.IsEmpty)
                return false;

            const bool splitRequested = false && AllowSplitStack && target.IsEmpty && source.StackCount > 1;
            if (splitRequested)
            {
                const int32_t moved = std::max(1, source.StackCount / 2);
                target = source;
                target.StackCount = moved;
                source.StackCount -= moved;
                if (source.StackCount <= 0)
                    source = {};
                SelectedIndex = targetIndex;
                RefreshSlotVisuals();
                if (OnSplitStack)
                    OnSplitStack(sourceIndex, targetIndex, moved);
                if (OnDrop)
                    OnDrop(sourceIndex, targetIndex);
                return true;
            }

            if (TransferMode == InventoryTransferMode::Copy)
            {
                if (!AllowCopy)
                    return false;
                target = source;
                SelectedIndex = targetIndex;
                RefreshSlotVisuals();
                if (OnCopy)
                    OnCopy(sourceIndex, targetIndex);
                if (OnDrop)
                    OnDrop(sourceIndex, targetIndex);
                return true;
            }

            if ((TransferMode == InventoryTransferMode::Move || target.IsEmpty) && AllowMove)
            {
                target = source;
                source = {};
                source.IsEmpty = true;
                source.StackCount = 0;
                SelectedIndex = targetIndex;
                RefreshSlotVisuals();
                if (OnMove)
                    OnMove(sourceIndex, targetIndex);
                if (OnDrop)
                    OnDrop(sourceIndex, targetIndex);
                return true;
            }

            if (!AllowSwap)
                return false;
            std::swap(source, target);
            SelectedIndex = targetIndex;
            RefreshSlotVisuals();
            if (OnSwap)
                OnSwap(sourceIndex, targetIndex);
            if (OnDrop)
                OnDrop(sourceIndex, targetIndex);
            return true;
        }

        bool UseSelected()
        {
            if (SelectedIndex < 0 || SelectedIndex >= static_cast<int32_t>(Items.size()) || Items[static_cast<size_t>(SelectedIndex)].IsEmpty)
                return false;
            if (OnUse)
                OnUse(SelectedIndex);
            return true;
        }

        void RefreshSlotVisuals()
        {
            for (size_t i = 0; i < Slots.size(); ++i)
            {
                auto& slot = Slots[i];
                const auto& item = Items[i];
                slot->ItemName = item.Name;
                slot->Name = item.Name;
                slot->ItemIcon = item.Icon;
                slot->QuantityText = !item.QuantityText.empty() ? item.QuantityText : (item.StackCount > 1 ? std::string("x") + std::to_string(item.StackCount) : std::string{});
                slot->BadgeText = item.BadgeText;
                slot->Rarity = item.Rarity;
                slot->ItemTint = item.Tint;
                slot->HasItemTint = item.HasTint;
                slot->ItemVisualRenderer = item.CustomVisualRenderer;
                slot->Cooldown = item.Cooldown;
                slot->IsEnabled = true;
                slot->IsSelected = static_cast<int32_t>(i) == SelectedIndex;
                slot->DragPayload = SlotPayload(static_cast<int32_t>(i));
                slot->IsDragSource = !item.IsEmpty;
                slot->AllowDrop = true;
                slot->ToolTip = item.IsEmpty ? "Empty slot" : "Drag " + item.Name + " onto another slot.";
            }
        }

        bool OnAction(UIAction action) override
        {
            if (action == UIAction::Accept)
                return UseSelected();
            if (action == UIAction::Details)
            {
                if (SelectedIndex >= 0 && SelectedIndex < static_cast<int32_t>(Items.size()) && OnInspect)
                {
                    OnInspect(SelectedIndex);
                    return true;
                }
                return false;
            }
            if (action == UIAction::NavigateLeft || action == UIAction::NavigateRight ||
                action == UIAction::NavigateUp || action == UIAction::NavigateDown)
                return MoveSelection(action);
            return WrapPanel::OnAction(action);
        }

        UIElement* HitTest(Vec2 point) override
        {
            if (Visibility == FyGUI::Visibility::Collapsed || !IsHitTestVisible || !IsEnabled || !Bounds.Contains(point))
                return nullptr;

            for (size_t i = Slots.size(); i-- > 0;)
            {
                auto& slot = Slots[i];
                if (!slot || !slot->Bounds.Contains(point))
                    continue;
                if (auto* childHit = slot->HitTest(point))
                    return childHit;
                return slot.get();
            }

            return this;
        }

    protected:
        void RenderOverride(ImDrawList& drawList) override
        {
            VisualState state = GetVisualState();
            if (HasStyledPartVisual(Style.PART_Root))
                DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
            if (HasStyledPartVisual(Style.PART_Background))
                DrawStyledPart(drawList, Bounds, Style.PART_Background, state);
            else
                DrawBackgroundAndBorder(drawList);
            DrawStyledPart(drawList, Bounds, Style.PART_ItemsPresenter, state);
            for (auto& child : Children)
                child->Render(drawList);
            DrawStyledPart(drawList, Bounds, Style.PART_Border, state);
        }

    private:
        static std::string SlotPayload(int32_t index)
        {
            return "inventory-slot:" + std::to_string(index);
        }

        static bool TryParseSlotPayload(const std::string& payload, int32_t& index)
        {
            const std::string prefix = "inventory-slot:";
            if (payload.rfind(prefix, 0) != 0)
                return false;
            char* end = nullptr;
            const long value = std::strtol(payload.c_str() + prefix.size(), &end, 10);
            if (!end || *end != '\0' || value < 0 || value > std::numeric_limits<int32_t>::max())
                return false;
            index = static_cast<int32_t>(value);
            return true;
        }

        int32_t ColumnCount() const
        {
            const float availableWidth = Bounds.width > 1.0f ? Bounds.width : (Width >= 0.0f ? Width : 360.0f);
            const float stride = std::max(1.0f, SlotWidth + Spacing);
            return std::max(1, static_cast<int32_t>((availableWidth + Spacing) / stride));
        }

        bool MoveSelection(UIAction action)
        {
            if (Items.empty())
                return false;

            const int32_t count = static_cast<int32_t>(Items.size());
            const int32_t columns = ColumnCount();
            int32_t index = SelectedIndex >= 0 ? SelectedIndex : 0;

            if (action == UIAction::NavigateLeft)
                index = std::max(0, index - 1);
            else if (action == UIAction::NavigateRight)
                index = std::min(count - 1, index + 1);
            else if (action == UIAction::NavigateUp)
                index = std::max(0, index - columns);
            else if (action == UIAction::NavigateDown)
                index = std::min(count - 1, index + columns);
            else
                return false;

            SetSelectedIndex(index);
            return true;
        }
    };

    struct CooldownButtonStyle
    {
        ControlPartStyle PART_CooldownOverlay;
        ControlPartStyle PART_CooldownText;
        ControlPartStyle PART_HotkeyBadge;
        ControlPartStyle PART_HotkeyText;
    };

    class CooldownButton : public Button
    {
    public:
        double Cooldown = 0.0;
        double MaxCooldown = 1.0;
        std::string HotkeyText;
        bool ShowCooldownText = true;
        CooldownButtonStyle CooldownStyle;

        explicit CooldownButton(std::string text = {}) : Button(std::move(text))
        {
            Width = 64.0f;
            Height = 64.0f;
            CornerRadius = 8.0f;
        }

        void StartCooldown(double seconds)
        {
            MaxCooldown = std::max(0.001, seconds);
            Cooldown = MaxCooldown;
        }

        void Update(float deltaTime) override
        {
            Button::Update(deltaTime);
            if (Cooldown > 0.0)
                Cooldown = std::max(0.0, Cooldown - static_cast<double>(deltaTime));
        }

    protected:
        void RenderOverride(ImDrawList& drawList) override
        {
            Button::RenderOverride(drawList);
            VisualState state = GetVisualState();

            const double ratio = MaxCooldown > 0.0 ? std::clamp(Cooldown / MaxCooldown, 0.0, 1.0) : 0.0;
            if (ratio > 0.0)
            {
                Rect overlay = Bounds;
                overlay.y = Bounds.y + Bounds.height * static_cast<float>(1.0 - ratio);
                overlay.height = Bounds.Bottom() - overlay.y;
                ControlPartStyle cooldown = CooldownStyle.PART_CooldownOverlay;
                if (!HasStyledPartVisual(cooldown))
                {
                    cooldown.BackgroundColor = { 0.0f, 0.0f, 0.0f, 0.56f };
                    cooldown.CornerRadius = CornerRadius;
                }
                DrawStyledPart(drawList, overlay, cooldown, VisualState::Pressed);

                if (ShowCooldownText)
                {
                    const std::string value = std::to_string(static_cast<int>(std::ceil(Cooldown)));
                    Vec2 size = MeasureText(value);
                    ControlPartStyle text = CooldownStyle.PART_CooldownText;
                    Color color = text.ForegroundColor.a > 0.0f ? text.ForegroundColor : Color{ 1.0f, 1.0f, 1.0f, 1.0f };
                    drawList.AddText(Vec2(Bounds.x + (Bounds.width - size.x) * 0.5f, Bounds.y + (Bounds.height - size.y) * 0.5f), color.ToU32(Opacity), value.c_str());
                }
            }

            if (!HotkeyText.empty())
            {
                Rect badge = { Bounds.x + 4.0f, Bounds.Bottom() - 20.0f, std::min(28.0f, Bounds.width - 8.0f), 16.0f };
                ControlPartStyle badgeStyle = CooldownStyle.PART_HotkeyBadge;
                if (!HasStyledPartVisual(badgeStyle))
                {
                    badgeStyle.BackgroundColor = { 0.0f, 0.0f, 0.0f, 0.64f };
                    badgeStyle.BorderColor = { 1.0f, 1.0f, 1.0f, 0.18f };
                    badgeStyle.BorderThickness = 1.0f;
                    badgeStyle.CornerRadius = 4.0f;
                }
                DrawStyledPart(drawList, badge, badgeStyle, state);
                ControlPartStyle hotkeyText = CooldownStyle.PART_HotkeyText;
                Color color = hotkeyText.ForegroundColor.a > 0.0f ? hotkeyText.ForegroundColor : Color{ 0.92f, 0.96f, 1.0f, 1.0f };
                Vec2 size = MeasureText(HotkeyText);
                drawList.AddText(Vec2(badge.x + (badge.width - size.x) * 0.5f, badge.y + (badge.height - size.y) * 0.5f), color.ToU32(Opacity), HotkeyText.c_str());
            }
        }
    };

    struct HotbarItem
    {
        std::string Label;
        std::string Hotkey;
        TextureId Icon = 0;
        double Cooldown = 0.0;
        double MaxCooldown = 1.0;
        bool Enabled = true;
    };

    struct HotbarStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Background;
        ControlPartStyle PART_SelectedSlot;
        ButtonStyle SlotButtonStyle;
        CooldownButtonStyle CooldownStyle;
    };

    class Hotbar : public WrapPanel
    {
    public:
        std::vector<HotbarItem> Items;
        std::vector<std::shared_ptr<CooldownButton>> Slots;
        int32_t SelectedIndex = 0;
        float SlotWidth = 64.0f;
        float SlotHeight = 64.0f;
        HotbarStyle Style;
        std::function<void(int32_t)> OnSelectionChanged;
        std::function<void(int32_t)> OnUseSlot;
        std::function<void(int32_t)> OnAssignSlot;

        Hotbar()
        {
            Width = 520.0f;
            Height = 78.0f;
            Padding = Thickness(8.0f);
            Spacing = 8.0f;
            LineSpacing = 8.0f;
            IsTabStop = true;
        }

        void RebuildSlots()
        {
            Children.clear();
            Slots.clear();
            Slots.reserve(Items.size());
            for (int32_t i = 0; i < static_cast<int32_t>(Items.size()); ++i)
            {
                const HotbarItem& item = Items[static_cast<size_t>(i)];
                auto slot = std::make_shared<CooldownButton>(item.Label);
                slot->Width = SlotWidth;
                slot->Height = SlotHeight;
                slot->HotkeyText = item.Hotkey;
                slot->IconTexture = item.Icon;
                slot->Cooldown = item.Cooldown;
                slot->MaxCooldown = item.MaxCooldown;
                slot->IsEnabled = item.Enabled;
                slot->ToolTipTitle = item.Label.empty() ? "Hotbar Slot" : item.Label;
                slot->ToolTip = item.Enabled ? "Click or focus + Enter to activate this command." : "Disabled slot with no command assigned.";
                slot->ToolTipAccent = item.Enabled ? Color{ 0.42f, 0.88f, 1.0f, 1.0f } : Color{ 1.0f, 0.48f, 0.56f, 1.0f };
                slot->Style = Style.SlotButtonStyle;
                slot->CooldownStyle = Style.CooldownStyle;
                slot->OnClick = [this, i]()
                {
                    SetSelectedIndex(i);
                    if (OnUseSlot)
                        OnUseSlot(i);
                };
                Slots.push_back(slot);
                AddChild(slot);
            }
            RefreshSelection();
        }

        void SetSelectedIndex(int32_t index)
        {
            const int32_t clamped = Items.empty() ? -1 : std::clamp(index, 0, static_cast<int32_t>(Items.size()) - 1);
            if (SelectedIndex == clamped)
                return;
            SelectedIndex = clamped;
            RefreshSelection();
            if (OnSelectionChanged)
                OnSelectionChanged(SelectedIndex);
        }

        bool ActivateSelected()
        {
            if (SelectedIndex < 0 || SelectedIndex >= static_cast<int32_t>(Slots.size()))
                return false;
            if (SelectedIndex >= static_cast<int32_t>(Items.size()) || !Items[static_cast<size_t>(SelectedIndex)].Enabled)
                return false;
            if (OnUseSlot)
                OnUseSlot(SelectedIndex);
            return true;
        }

        void Update(float deltaTime) override
        {
            WrapPanel::Update(deltaTime);
            for (size_t i = 0; i < Slots.size() && i < Items.size(); ++i)
                Items[i].Cooldown = Slots[i]->Cooldown;
        }

    protected:
        void RenderOverride(ImDrawList& drawList) override
        {
            VisualState state = GetVisualState();
            DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
            ControlPartStyle background = Style.PART_Background;
            if (!HasStyledPartVisual(background))
            {
                background.BackgroundColor = { 0.015f, 0.020f, 0.030f, 0.82f };
                background.BorderColor = { 0.28f, 0.52f, 0.72f, 0.62f };
                background.BorderThickness = 1.0f;
                background.CornerRadius = 10.0f;
            }
            DrawStyledPart(drawList, Bounds, background, state);
            for (auto& child : Children)
                child->Render(drawList);
            if (SelectedIndex >= 0 && SelectedIndex < static_cast<int32_t>(Slots.size()))
            {
                ControlPartStyle selected = Style.PART_SelectedSlot;
                if (!HasStyledPartVisual(selected))
                {
                    selected.BorderColor = { 1.0f, 0.82f, 0.28f, 1.0f };
                    selected.BorderThickness = 2.0f;
                    selected.CornerRadius = 9.0f;
                }
                DrawStyledPart(drawList, Slots[static_cast<size_t>(SelectedIndex)]->Bounds, selected, VisualState::Selected);
            }
        }

        bool OnAction(UIAction action) override
        {
            if (action == UIAction::NavigateLeft)
            {
                SetSelectedIndex(SelectedIndex - 1);
                return true;
            }
            if (action == UIAction::NavigateRight)
            {
                SetSelectedIndex(SelectedIndex + 1);
                return true;
            }
            if (action == UIAction::Accept)
                return ActivateSelected();
            if (action == UIAction::Secondary)
            {
                if (SelectedIndex < 0 || SelectedIndex >= static_cast<int32_t>(Items.size()))
                    return false;
                if (OnAssignSlot)
                    OnAssignSlot(SelectedIndex);
                return OnAssignSlot != nullptr;
            }
            return WrapPanel::OnAction(action);
        }

    private:
        void RefreshSelection()
        {
            for (size_t i = 0; i < Slots.size(); ++i)
            {
                Slots[i]->BorderBrush = static_cast<int32_t>(i) == SelectedIndex
                    ? Color{ 1.0f, 0.82f, 0.28f, 1.0f }
                    : Color{ 0.32f, 0.48f, 0.62f, 0.85f };
            }
        }
    };

    struct RadialMenuItem
    {
        std::string Label;
        TextureId Icon = 0;
        bool Enabled = true;
    };

    struct RadialMenuStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_Slice;
        ControlPartStyle PART_SelectedSlice;
        ControlPartStyle PART_Center;
        ControlPartStyle PART_Text;
        ControlPartStyle PART_Icon;
    };

    class RadialMenu : public Control
    {
    public:
        std::vector<RadialMenuItem> Items;
        int32_t SelectedIndex = 0;
        float Radius = 120.0f;
        float InnerRadius = 34.0f;
        bool IsOpen = true;
        RadialMenuStyle Style;
        std::function<void()> OnOpened;
        std::function<void()> OnClosed;
        std::function<void(int32_t)> OnSelectionChanged;
        std::function<void(int32_t)> OnConfirm;

        RadialMenu()
        {
            Width = 260.0f;
            Height = 260.0f;
            IsTabStop = true;
            BackgroundNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
            BorderBrush = BackgroundNormal;
        }

        void SetSelectedIndex(int32_t index)
        {
            const int32_t clamped = Items.empty() ? -1 : std::clamp(index, 0, static_cast<int32_t>(Items.size()) - 1);
            if (SelectedIndex == clamped)
                return;
            SelectedIndex = clamped;
            if (OnSelectionChanged)
                OnSelectionChanged(SelectedIndex);
        }

        void Open()
        {
            if (IsOpen)
                return;
            IsOpen = true;
            OpenTween.SetTarget(1.0f);
            if (OnOpened)
                OnOpened();
        }

        void Close()
        {
            if (!IsOpen)
                return;
            IsOpen = false;
            OpenTween.SetTarget(0.0f);
            if (OnClosed)
                OnClosed();
        }

        bool ConfirmSelected()
        {
            if (!IsOpen || SelectedIndex < 0 || SelectedIndex >= static_cast<int32_t>(Items.size()) || !Items[static_cast<size_t>(SelectedIndex)].Enabled)
                return false;
            if (OnConfirm)
                OnConfirm(SelectedIndex);
            return true;
        }

    protected:
        Vec2 MeasureOverride(Vec2) override
        {
            const float size = Radius * 2.0f + Padding.Horizontal();
            return { Width >= 0.0f ? Width : size, Height >= 0.0f ? Height : size };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            if (!IsOpen || Items.empty())
                return;
            constexpr float Pi = 3.14159265358979323846f;
            VisualState state = GetVisualState();
            DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
            const Vec2 center(Bounds.x + Bounds.width * 0.5f, Bounds.y + Bounds.height * 0.5f);
            const float radius = std::min({ Radius, Bounds.width * 0.5f, Bounds.height * 0.5f });
            const float step = (Pi * 2.0f) / static_cast<float>(Items.size());

            for (size_t i = 0; i < Items.size(); ++i)
            {
                const bool selected = static_cast<int32_t>(i) == SelectedIndex;
                const float a0 = -Pi * 0.5f + step * static_cast<float>(i);
                const float a1 = a0 + step;
                ControlPartStyle slice = selected ? Style.PART_SelectedSlice : Style.PART_Slice;
                if (!HasStyledPartVisual(slice))
                {
                    slice.BackgroundColor = selected ? Color{ 0.22f, 0.58f, 0.82f, 0.82f } : Color{ 0.06f, 0.09f, 0.13f, 0.78f };
                    slice.BorderColor = { 0.42f, 0.66f, 0.84f, 0.72f };
                    slice.BorderThickness = 1.0f;
                }

                drawList.PathClear();
                drawList.PathLineTo(center);
                drawList.PathArcTo(center, radius, a0, a1, 12);
                drawList.PathFillConvex(slice.BackgroundColor.ToU32(slice.Opacity * Opacity));
                drawList.AddLine(center, Vec2(center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius), slice.BorderColor.ToU32(slice.Opacity * Opacity), slice.BorderThickness > 0.0f ? slice.BorderThickness : 1.0f);

                const float labelAngle = (a0 + a1) * 0.5f;
                Vec2 textSize = MeasureText(Items[i].Label);
                Vec2 textPos(center.x + std::cos(labelAngle) * radius * 0.62f - textSize.x * 0.5f, center.y + std::sin(labelAngle) * radius * 0.62f - textSize.y * 0.5f);
                Color text = ResolvePartForeground(Style.PART_Text, Color{ 0.92f, 0.96f, 1.0f, Items[i].Enabled ? 1.0f : 0.42f });
                drawList.AddText(textPos, text.ToU32(Opacity), Items[i].Label.c_str());
            }

            ControlPartStyle centerStyle = Style.PART_Center;
            if (!HasStyledPartVisual(centerStyle))
            {
                centerStyle.BackgroundColor = { 0.010f, 0.014f, 0.020f, 0.92f };
                centerStyle.BorderColor = { 0.56f, 0.80f, 0.96f, 0.90f };
                centerStyle.BorderThickness = 2.0f;
                centerStyle.CornerRadius = InnerRadius;
            }
            DrawStyledPart(drawList, { center.x - InnerRadius, center.y - InnerRadius, InnerRadius * 2.0f, InnerRadius * 2.0f }, centerStyle, state);
            drawList.AddCircle(center, radius, Color{ 0.42f, 0.66f, 0.84f, 0.72f }.ToU32(Opacity), 96, 1.0f);
        }

        void OnEvent(EventArgs& args) override
        {
            if (args.kind == EventKind::PointerMoved || args.kind == EventKind::PointerPressed || args.kind == EventKind::PointerReleased)
            {
                const auto& pointer = static_cast<PointerEventArgs&>(args);
                const int32_t hit = HitSlice(pointer.position);
                if (hit >= 0)
                    SetSelectedIndex(hit);
                if (args.kind == EventKind::PointerPressed)
                {
                    IsPressed = true;
                    args.handled = true;
                }
                else if (args.kind == EventKind::PointerReleased)
                {
                    IsPressed = false;
                    ConfirmSelected();
                    args.handled = true;
                }
            }
            else if (args.kind == EventKind::PointerExited)
            {
                IsPressed = false;
            }
        }

        bool OnAction(UIAction action) override
        {
            if (action == UIAction::NavigateLeft || action == UIAction::NavigateUp)
            {
                SetSelectedIndex(SelectedIndex - 1);
                return true;
            }
            if (action == UIAction::NavigateRight || action == UIAction::NavigateDown)
            {
                SetSelectedIndex(SelectedIndex + 1);
                return true;
            }
            if (action == UIAction::Accept)
            {
                return ConfirmSelected();
            }
            if (action == UIAction::Cancel || action == UIAction::Back)
            {
                Close();
                return true;
            }
            return false;
        }

    private:
        int32_t HitSlice(Vec2 point) const
        {
            if (Items.empty())
                return -1;
            constexpr float Pi = 3.14159265358979323846f;
            const Vec2 center{ Bounds.x + Bounds.width * 0.5f, Bounds.y + Bounds.height * 0.5f };
            const float dx = point.x - center.x;
            const float dy = point.y - center.y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (distance > Radius || distance < InnerRadius * 0.5f)
                return -1;
            float angle = std::atan2(dy, dx) + Pi * 0.5f;
            while (angle < 0.0f)
                angle += Pi * 2.0f;
            while (angle >= Pi * 2.0f)
                angle -= Pi * 2.0f;
            const float step = (Pi * 2.0f) / static_cast<float>(Items.size());
            return std::clamp(static_cast<int32_t>(angle / step), 0, static_cast<int32_t>(Items.size()) - 1);
        }
    };

    class CommandWheel : public RadialMenu
    {
    public:
        bool CloseOnActivate = false;
        std::string CenterLabel = "COMMAND";
        float OpenProgress = 1.0f;
        float OpenAnimationSpeed = 12.0f;

        CommandWheel()
        {
            Width = 300.0f;
            Height = 300.0f;
            Radius = 142.0f;
            InnerRadius = 44.0f;
        }

        void Open()
        {
            RadialMenu::Open();
            Visibility = FyGUI::Visibility::Visible;
            if (OpenProgress <= 0.0f)
                OpenProgress = 0.001f;
        }

        void Close()
        {
            RadialMenu::Close();
            IsPressed = false;
            IsHovered = false;
        }

        void Toggle()
        {
            if (IsOpen)
                Close();
            else
                Open();
        }

        void Update(float deltaTime) override
        {
            RadialMenu::Update(deltaTime);
            const float target = IsOpen ? 1.0f : 0.0f;
            const float step = std::clamp(deltaTime * OpenAnimationSpeed, 0.0f, 1.0f);
            OpenProgress += (target - OpenProgress) * step;
            if (std::abs(OpenProgress - target) < 0.001f)
                OpenProgress = target;
            Visibility = OpenProgress > 0.0f ? FyGUI::Visibility::Visible : FyGUI::Visibility::Collapsed;
        }

        UIElement* HitTest(Vec2 point) override
        {
            if (!IsOpen)
                return nullptr;
            return RadialMenu::HitTest(point);
        }

    protected:
        void RenderOverride(ImDrawList& drawList) override
        {
            if (OpenProgress <= 0.0f)
                return;
            const Rect previousBounds = Bounds;
            const float previousOpacity = Opacity;
            Bounds = ScaleRectFromCenter(Bounds, 0.88f + EaseOutCubic(OpenProgress) * 0.12f);
            Opacity = previousOpacity * OpenProgress;
            RadialMenu::RenderOverride(drawList);
            if (!CenterLabel.empty())
            {
                Vec2 size = MeasureText(CenterLabel);
                Vec2 center(Bounds.x + Bounds.width * 0.5f, Bounds.y + Bounds.height * 0.5f);
                drawList.AddText(Vec2{ center.x - size.x * 0.5f, center.y - size.y * 0.5f }, ResolvePartForeground(Style.PART_Text, Color{ 0.92f, 0.97f, 1.0f, 1.0f }).ToU32(Opacity), CenterLabel.c_str());
            }
            Bounds = previousBounds;
            Opacity = previousOpacity;
        }

        void OnEvent(EventArgs& args) override
        {
            RadialMenu::OnEvent(args);
            if (CloseOnActivate && args.kind == EventKind::PointerReleased && args.handled)
                Close();
        }

        bool OnAction(UIAction action) override
        {
            if (action == UIAction::Cancel || action == UIAction::Back)
            {
                Close();
                return true;
            }
            const bool handled = RadialMenu::OnAction(action);
            if (handled && CloseOnActivate && action == UIAction::Accept)
                Close();
            return handled;
        }
    };

    struct ToastNotification
    {
        std::string Title;
        std::string Message;
        float Duration = 3.0f;
        float TimeRemaining = 3.0f;
        Color Accent = { 0.30f, 0.76f, 0.96f, 1.0f };
    };

    struct ToastHostStyle
    {
        ControlPartStyle PART_Root;
        ControlPartStyle PART_ToastBackground;
        ControlPartStyle PART_ToastBorder;
        ControlPartStyle PART_TitleText;
        ControlPartStyle PART_MessageText;
        ControlPartStyle PART_Progress;
    };

    class ToastHost : public UIElement
    {
    public:
        std::vector<ToastNotification> Toasts;
        ToastHostStyle Style;
        float ToastWidth = 320.0f;
        float ToastHeight = 74.0f;
        float Spacing = 10.0f;
        bool AnchorBottomRight = true;
        int32_t MaxVisibleToasts = 4;
        int32_t MaxQueuedToasts = 10;

        ToastHost()
        {
            Width = 360.0f;
            Height = 260.0f;
            IsHitTestVisible = false;
        }

        void AddToast(std::string title, std::string message, float duration = 3.0f, Color accent = Color{ 0.30f, 0.76f, 0.96f, 1.0f })
        {
            ToastNotification toast;
            toast.Title = std::move(title);
            toast.Message = std::move(message);
            toast.Duration = std::max(0.1f, duration);
            toast.TimeRemaining = toast.Duration;
            toast.Accent = accent;
            Toasts.push_back(std::move(toast));
            while (MaxQueuedToasts > 0 && static_cast<int32_t>(Toasts.size()) > MaxQueuedToasts)
                Toasts.erase(Toasts.begin());
        }

        void Update(float deltaTime) override
        {
            for (auto& toast : Toasts)
                toast.TimeRemaining -= deltaTime;
            Toasts.erase(std::remove_if(Toasts.begin(), Toasts.end(), [](const ToastNotification& toast) { return toast.TimeRemaining <= 0.0f; }), Toasts.end());
        }

    protected:
        Vec2 MeasureOverride(Vec2) override
        {
            return { Width >= 0.0f ? Width : ToastWidth, Height >= 0.0f ? Height : ToastHeight * 3.0f + Spacing * 2.0f };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            DrawStyledPart(drawList, Bounds, Style.PART_Root, VisualState::Normal);
            float y = AnchorBottomRight ? Bounds.Bottom() - ToastHeight : Bounds.Top();
            int32_t visible = 0;
            for (auto it = Toasts.rbegin(); it != Toasts.rend(); ++it)
            {
                if (MaxVisibleToasts > 0 && visible >= MaxVisibleToasts)
                    break;
                const float life = std::max(0.0f, it->Duration - it->TimeRemaining);
                const float fadeIn = EaseOutCubic(std::clamp(life * 8.0f, 0.0f, 1.0f));
                const float fadeOut = std::clamp(it->TimeRemaining * 4.0f, 0.0f, 1.0f);
                const float visibility = std::min(fadeIn, fadeOut);
                const float slide = (1.0f - visibility) * 36.0f;
                const float x = (AnchorBottomRight ? Bounds.Right() - ToastWidth : Bounds.Left()) + (AnchorBottomRight ? slide : -slide);
                Rect rect = { x, y, ToastWidth, ToastHeight };
                ControlPartStyle bg = Style.PART_ToastBackground;
                if (!HasStyledPartVisual(bg))
                {
                    bg.BackgroundColor = { 0.018f, 0.024f, 0.034f, 0.94f };
                    bg.BorderColor = it->Accent;
                    bg.BorderThickness = 1.0f;
                    bg.CornerRadius = 9.0f;
                }
                DrawStyledPart(drawList, rect, ApplyPartOpacity(bg, visibility), VisualState::Normal);
                DrawStyledPart(drawList, rect, ApplyPartOpacity(Style.PART_ToastBorder, visibility), VisualState::Normal);

                ControlPartStyle title = Style.PART_TitleText;
                ControlPartStyle message = Style.PART_MessageText;
                const Color titleColor = title.ForegroundColor.a > 0.0f ? title.ForegroundColor : it->Accent;
                const Color messageColor = message.ForegroundColor.a > 0.0f ? message.ForegroundColor : Color{ 0.86f, 0.90f, 0.96f, 1.0f };
                drawList.AddText(Vec2{ rect.x + 12.0f, rect.y + 9.0f }, titleColor.ToU32(Opacity * visibility), it->Title.c_str());
                drawList.AddText(GetDefaultFont(), GetFontSize(), Vec2{ rect.x + 12.0f, rect.y + 33.0f }, messageColor.ToU32(Opacity * visibility), it->Message.c_str(), nullptr, rect.width - 24.0f);

                const float ratio = std::clamp(it->TimeRemaining / std::max(0.1f, it->Duration), 0.0f, 1.0f);
                ControlPartStyle progress = Style.PART_Progress;
                if (!HasStyledPartVisual(progress))
                {
                    progress.BackgroundColor = it->Accent;
                    progress.CornerRadius = 2.0f;
                }
                DrawStyledPart(drawList, { rect.x, rect.Bottom() - 3.0f, rect.width * ratio, 3.0f }, ApplyPartOpacity(progress, visibility), VisualState::Normal);
                y += AnchorBottomRight ? -(ToastHeight + Spacing) : (ToastHeight + Spacing);
                ++visible;
            }
        }
    };

    using NotificationQueue = ToastHost;

    enum class ContentDialogButton
    {
        None,
        Primary,
        Secondary,
        Close
    };

    class ContentDialog : public Control
    {
    public:
        bool IsOpen = false;
        std::string Title = "Dialog";
        std::string Content = "Message";
        std::string PrimaryButtonText = "Accept";
        std::string SecondaryButtonText = "Cancel";
        std::string CloseButtonText;
        ContentDialogButton DefaultButton = ContentDialogButton::None;
        bool IsPrimaryButtonEnabled = true;
        bool IsSecondaryButtonEnabled = true;
        ContentDialogStyle Style;
        std::function<void()> OnPrimaryButtonClick;
        std::function<void()> OnSecondaryButtonClick;
        std::function<void()> OnCloseButtonClick;
        std::function<void()> OnOpened;
        std::function<void()> OnClosed;
        float AnimationProgress = 0.0f;
        float AnimationSpeed = 12.0f;

        ContentDialog()
        {
            Width = 420.0f;
            Height = 220.0f;
            Padding = Thickness(18.0f);
            CornerRadius = 12.0f;
            IsTabStop = true;
            Visibility = FyGUI::Visibility::Collapsed;
        }

        void Open()
        {
            IsOpen = true;
            Visibility = FyGUI::Visibility::Visible;
            if (AnimationProgress <= 0.0f)
                AnimationProgress = 0.001f;
            IsPressed = false;
            IsHovered = false;
            if (OnOpened)
                OnOpened();
        }

        void Close()
        {
            const bool wasOpen = IsOpen;
            IsOpen = false;
            IsPressed = false;
            IsHovered = false;
            if (wasOpen && OnClosed)
                OnClosed();
        }

        void Update(float deltaTime) override
        {
            Control::Update(deltaTime);
            const float target = IsOpen ? 1.0f : 0.0f;
            const float step = std::clamp(deltaTime * AnimationSpeed, 0.0f, 1.0f);
            AnimationProgress += (target - AnimationProgress) * step;
            if (std::abs(AnimationProgress - target) < 0.001f)
                AnimationProgress = target;
            Visibility = AnimationProgress > 0.0f ? FyGUI::Visibility::Visible : FyGUI::Visibility::Collapsed;
            if (!IsOpen)
            {
                IsPressed = false;
                IsHovered = false;
            }
        }

        UIElement* HitTest(Vec2 point) override
        {
            if (!IsOpen)
                return nullptr;
            return Control::HitTest(point);
        }

    protected:
        Vec2 MeasureOverride(Vec2) override
        {
            if (!IsOpen && AnimationProgress <= 0.0f)
                return {};
            return { Width >= 0.0f ? Width : 420.0f, Height >= 0.0f ? Height : 220.0f };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            if (AnimationProgress <= 0.0f)
                return;
            VisualState state = GetVisualState();
            const Rect previousBounds = Bounds;
            const float fade = std::clamp(AnimationProgress, 0.0f, 1.0f);
            Bounds = ScaleRectFromCenter(Bounds, 0.94f + EaseOutCubic(fade) * 0.06f);
            DrawStyledPart(drawList, Bounds, ApplyPartOpacity(Style.PART_Shadow, fade), state);
            ControlPartStyle bg = Style.PART_Background;
            if (!HasStyledPartVisual(bg))
            {
                bg.BackgroundColor = { 0.018f, 0.024f, 0.034f, 0.98f };
                bg.BorderColor = { 0.44f, 0.72f, 0.92f, 0.88f };
                bg.BorderThickness = 1.0f;
                bg.CornerRadius = CornerRadius;
            }
            DrawStyledPart(drawList, Bounds, ApplyPartOpacity(bg, fade), state);
            DrawStyledPart(drawList, { Bounds.x, Bounds.y, Bounds.width, 44.0f }, ApplyPartOpacity(Style.PART_TitleBar, fade), state);

            Color titleColor = Style.PART_TitleText.ForegroundColor.a > 0.0f ? Style.PART_TitleText.ForegroundColor : Color{ 0.76f, 0.95f, 1.0f, 1.0f };
            drawList.AddText(Vec2{ Bounds.x + Padding.left, Bounds.y + 13.0f }, titleColor.ToU32(Opacity * fade), Title.c_str());
            Color textColor = Style.PART_ContentPresenter.ForegroundColor.a > 0.0f ? Style.PART_ContentPresenter.ForegroundColor : Color{ 0.86f, 0.90f, 0.96f, 1.0f };
            drawList.AddText(GetDefaultFont(), GetFontSize(), Vec2{ Bounds.x + Padding.left, Bounds.y + 64.0f }, textColor.ToU32(Opacity * fade), Content.c_str(), nullptr, Bounds.width - Padding.Horizontal());

            DrawStyledPart(drawList, CommandSpaceRect(), ApplyPartOpacity(Style.PART_CommandSpace, fade), state);
            if (!CloseButtonText.empty())
                DrawDialogButton(drawList, CloseButtonRect(), CloseButtonText, DefaultButton == ContentDialogButton::Close, true, fade, Style.PART_CloseButton);
            if (!SecondaryButtonText.empty())
                DrawDialogButton(drawList, SecondaryButtonRect(), SecondaryButtonText, DefaultButton == ContentDialogButton::Secondary, IsSecondaryButtonEnabled, fade, Style.PART_SecondaryButton);
            if (!PrimaryButtonText.empty())
                DrawDialogButton(drawList, PrimaryButtonRect(), PrimaryButtonText, DefaultButton == ContentDialogButton::Primary || DefaultButton == ContentDialogButton::None, IsPrimaryButtonEnabled, fade, Style.PART_PrimaryButton);
            DrawStyledPart(drawList, Bounds, ApplyPartOpacity(Style.PART_Border, fade), state);
            Bounds = previousBounds;
        }

        void OnEvent(EventArgs& args) override
        {
            if (!IsOpen)
                return;
            if (args.kind == EventKind::PointerPressed)
            {
                IsPressed = true;
                args.handled = true;
            }
            else if (args.kind == EventKind::PointerReleased)
            {
                const auto& pointer = static_cast<PointerEventArgs&>(args);
                IsPressed = false;
                if (!PrimaryButtonText.empty() && IsPrimaryButtonEnabled && PrimaryButtonRect().Contains(pointer.position))
                {
                    Close();
                    if (OnPrimaryButtonClick)
                        OnPrimaryButtonClick();
                    args.handled = true;
                }
                else if (!SecondaryButtonText.empty() && IsSecondaryButtonEnabled && SecondaryButtonRect().Contains(pointer.position))
                {
                    Close();
                    if (OnSecondaryButtonClick)
                        OnSecondaryButtonClick();
                    args.handled = true;
                }
                else if (!CloseButtonText.empty() && CloseButtonRect().Contains(pointer.position))
                {
                    Close();
                    if (OnCloseButtonClick)
                        OnCloseButtonClick();
                    args.handled = true;
                }
            }
        }

        bool OnAction(UIAction action) override
        {
            if (!IsOpen)
                return false;
            if (action == UIAction::Accept)
            {
                if (!IsPrimaryButtonEnabled)
                    return true;
                Close();
                if (OnPrimaryButtonClick)
                    OnPrimaryButtonClick();
                return true;
            }
            if (action == UIAction::Cancel || action == UIAction::Back)
            {
                Close();
                if (OnCloseButtonClick)
                    OnCloseButtonClick();
                else if (OnSecondaryButtonClick)
                    OnSecondaryButtonClick();
                return true;
            }
            return false;
        }

    private:
        Rect PrimaryButtonRect() const
        {
            return { Bounds.Right() - Padding.right - 118.0f, Bounds.Bottom() - Padding.bottom - 38.0f, 118.0f, 38.0f };
        }

        Rect SecondaryButtonRect() const
        {
            Rect accept = PrimaryButtonRect();
            return { accept.x - 130.0f, accept.y, 118.0f, 38.0f };
        }

        Rect CloseButtonRect() const
        {
            Rect secondary = SecondaryButtonRect();
            return { secondary.x - 130.0f, secondary.y, 118.0f, 38.0f };
        }

        Rect CommandSpaceRect() const
        {
            return { Bounds.x + Padding.left, Bounds.Bottom() - Padding.bottom - 44.0f, std::max(0.0f, Bounds.width - Padding.Horizontal()), 44.0f };
        }

        void DrawDialogButton(ImDrawList& drawList, Rect rect, const std::string& text, bool accent, bool enabled, float visualOpacity, const ControlPartStyle& stylePart)
        {
            ControlPartStyle part = stylePart;
            if (!HasStyledPartVisual(part))
            {
                part.BackgroundColor = accent ? Color{ 0.18f, 0.42f, 0.62f, 0.96f } : Color{ 0.08f, 0.10f, 0.14f, 0.94f };
                part.BorderColor = accent ? Color{ 0.44f, 0.78f, 1.0f, 0.82f } : Color{ 0.34f, 0.44f, 0.56f, 0.82f };
                part.BorderThickness = 1.0f;
                part.CornerRadius = 7.0f;
            }
            if (!enabled)
                part.Opacity *= 0.45f;
            DrawStyledPart(drawList, rect, ApplyPartOpacity(part, visualOpacity), enabled ? (accent ? VisualState::Selected : VisualState::Normal) : VisualState::Disabled);
            Vec2 size = MeasureText(text);
            const Color textColor = enabled ? Color{ 0.94f, 0.97f, 1.0f, 1.0f } : Color{ 0.94f, 0.97f, 1.0f, 0.54f };
            drawList.AddText(Vec2(rect.x + (rect.width - size.x) * 0.5f, rect.y + (rect.height - size.y) * 0.5f), textColor.ToU32(Opacity * visualOpacity), text.c_str());
        }
    };

    inline CooldownButtonStyle MakeCooldownButtonStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        CooldownButtonStyle style {};
        style.PART_CooldownOverlay = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.58f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius);
        style.PART_CooldownText.ForegroundColor = p.Text;
        style.PART_HotkeyBadge = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.82f }, p.Border, 1.0f, p.Radius * 0.5f);
        style.PART_HotkeyText.ForegroundColor = p.Text;
        return style;
    }

    inline HotbarStyle MakeHotbarStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        HotbarStyle style {};
        style.PART_Background = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.82f }, p.Border, 1.0f, p.Radius + 2.0f);
        style.PART_SelectedSlot = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.10f }, p.AccentAlt, 2.0f, p.Radius + 1.0f);
        style.SlotButtonStyle = MakeButtonStyle(preset);
        style.CooldownStyle = MakeCooldownButtonStyle(preset);
        return style;
    }

    inline RadialMenuStyle MakeRadialMenuStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        RadialMenuStyle style {};
        style.PART_Slice = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.78f }, p.Border, 1.0f, p.Radius);
        style.PART_SelectedSlice = SolidPart(Color{ p.Accent.r, p.Accent.g, p.Accent.b, 0.74f }, p.AccentAlt, 1.0f, p.Radius);
        style.PART_Center = SolidPart(p.SurfaceAlt, p.Accent, 2.0f, 999.0f);
        style.PART_Text.ForegroundColor = p.Text;
        return style;
    }

    inline ToastHostStyle MakeToastHostStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        ToastHostStyle style {};
        style.PART_ToastBackground = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.94f }, p.Border, 1.0f, p.Radius + 2.0f);
        style.PART_TitleText.ForegroundColor = p.Accent;
        style.PART_MessageText.ForegroundColor = p.Text;
        style.PART_Progress = SolidPart(p.Accent, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, 2.0f);
        return style;
    }

    inline ContentDialogStyle MakeContentDialogStyle(ThemePreset preset)
    {
        const StylePalette p = MakeStylePalette(preset);
        ContentDialogStyle style {};
        style.PART_Background = SolidPart(Color{ p.Surface.r, p.Surface.g, p.Surface.b, 0.98f }, p.Border, 1.0f, p.Radius + 4.0f);
        style.PART_TitleBar = SolidPart(p.SurfaceAlt, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius);
        style.PART_TitleText.ForegroundColor = p.Accent;
        style.PART_ContentPresenter.ForegroundColor = p.Text;
        style.PART_CloseButton = SolidPart(p.SurfaceAlt, p.Border, 1.0f, p.Radius);
        style.PART_Shadow = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.28f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, p.Radius + 6.0f);
        return style;
    }

    struct SmokeTestResult
    {
        bool Passed = false;
        int32_t ControlsCreated = 0;
        std::string Message;
    };

    inline SmokeTestResult RunSmokeTest()
    {
        SmokeTestResult result {};
        auto root = std::make_shared<Canvas>();
        root->Width = 800.0f;
        root->Height = 600.0f;

        auto stack = std::make_shared<StackPanel>();
        stack->Width = 760.0f;
        stack->Spacing = 8.0f;

        auto button = std::make_shared<Button>("Smoke Button");
        button->Style = MakeButtonStyle(ThemePreset::SciFi);
        auto slider = std::make_shared<Slider>();
        slider->Orientation = FyGUI::Orientation::Vertical;
        slider->Height = 120.0f;
        slider->Style = MakeSliderStyle(ThemePreset::Minimal);
        auto progress = std::make_shared<ProgressBar>();
        progress->Width = 180.0f;
        progress->Value = 0.5;
        progress->Style = MakeProgressBarStyle(ThemePreset::Tactical);
        auto list = std::make_shared<ListBox>();
        list->Items = { "A", "B", "C" };
        list->VisibleItems = 2;
        list->Style = MakeListBoxStyle(ThemePreset::Fantasy);
        list->SetSelectedIndex(2);
        const bool listKeepsSelectionVisibleOk = list->ScrollOffsetY > 0.0f;
        auto combo = std::make_shared<ComboBox>();
        combo->Items = { "Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "White", "Black" };
        combo->SelectedIndex = 1;
        combo->SetIsOpen(true);
        combo->SetSelectedIndex(7);
        const bool comboKeepsSelectionVisibleOk = combo->ScrollOffsetY > 0.0f;
        auto textBox = std::make_shared<TextBox>();
        textBox->Text = "Smoke text";
        textBox->Width = 180.0f;
        textBox->SelectionStart = 0;
        textBox->SelectionLength = 5;
        TextInputEventArgs textReplaceArgs;
        textReplaceArgs.kind = EventKind::TextInput;
        textReplaceArgs.route = RoutingStrategy::Bubble;
        textReplaceArgs.originalSource = textBox.get();
        textReplaceArgs.codepoint = 'A';
        textBox->SetFocused(true);
        textBox->RaiseEvent(textReplaceArgs);
        const bool textBoxSelectionOk = textBox->Text == "A text";
        textBox->AcceptsReturn = true;
        textBox->TextWrapping = true;
        textBox->SelectionStart = static_cast<int32_t>(textBox->Text.size());
        textBox->SelectionLength = 0;
        KeyEventArgs enterArgs;
        enterArgs.kind = EventKind::KeyDown;
        enterArgs.route = RoutingStrategy::Bubble;
        enterArgs.originalSource = textBox.get();
        enterArgs.key = Key::Enter;
        textBox->RaiseEvent(enterArgs);
        const bool textBoxMultilineOk = textBox->Text.find('\n') != std::string::npos;
        auto scrollViewer = std::make_shared<ScrollViewer>();
        scrollViewer->Width = 220.0f;
        scrollViewer->Height = 80.0f;
        auto scrollContent = std::make_shared<StackPanel>();
        scrollContent->AddChild(std::make_shared<TextBlock>("Scroll line A"));
        scrollContent->AddChild(std::make_shared<TextBlock>("Scroll line B"));
        scrollContent->AddChild(std::make_shared<TextBlock>("Scroll line C"));
        scrollViewer->SetChild(scrollContent);
        auto scrollHitTest = std::make_shared<ScrollViewer>();
        scrollHitTest->Width = 220.0f;
        scrollHitTest->Height = 80.0f;
        scrollHitTest->VerticalScrollBarVisibility = ScrollBarVisibility::Visible;
        auto scrollHitContent = std::make_shared<StackPanel>();
        scrollHitContent->Spacing = 6.0f;
        scrollHitContent->AddChild(std::make_shared<TextBlock>("Hit line A"));
        scrollHitContent->AddChild(std::make_shared<TextBlock>("Hit line B"));
        scrollHitContent->AddChild(std::make_shared<TextBlock>("Hit line C"));
        scrollHitContent->AddChild(std::make_shared<TextBlock>("Hit line D"));
        scrollHitContent->AddChild(std::make_shared<TextBlock>("Hit line E"));
        scrollHitContent->AddChild(std::make_shared<TextBlock>("Hit line F"));
        scrollHitTest->SetChild(scrollHitContent);
        scrollHitTest->Measure({ 220.0f, 80.0f });
        scrollHitTest->Arrange({ 10.0f, 10.0f, 220.0f, 80.0f });
        const float scrollContentBeforeY = scrollHitContent->Bounds.y;
        PointerEventArgs wheelArgs;
        wheelArgs.kind = EventKind::PointerWheelChanged;
        wheelArgs.route = RoutingStrategy::Bubble;
        wheelArgs.originalSource = scrollHitContent->Children.front().get();
        wheelArgs.position = { 20.0f, 20.0f };
        wheelArgs.screenPosition = wheelArgs.position;
        wheelArgs.wheelDelta = { 0.0f, -1.0f };
        scrollHitContent->Children.front()->RaiseEvent(wheelArgs);
        const bool scrollWheelRoutesOk = scrollHitContent->Bounds.y < scrollContentBeforeY && scrollHitTest->Offset.y > 0.0f;
        const float scrollContentAfterWheelY = scrollHitContent->Bounds.y;
        scrollHitTest->ScrollBy({ 0.0f, 32.0f });
        UIElement* scrollHitBase = scrollHitTest.get();
        const bool scrollContentHitOk = scrollHitBase->HitTest({ 20.0f, 20.0f }) != nullptr;
        const bool scrollBarHitOk = scrollHitBase->HitTest({ 225.0f, 20.0f }) == scrollHitBase;
        const bool scrollOutsideHitOk = scrollHitBase->HitTest({ 500.0f, 500.0f }) == nullptr;
        const bool scrollMovesContentOk = scrollHitContent->Bounds.y < scrollContentAfterWheelY;
        auto tree = std::make_shared<TreeView>();
        tree->Items = { { "Root", true, { { "Child A" }, { "Child B" } } } };
        auto hotbar = std::make_shared<Hotbar>();
        hotbar->Items = { { "A", "1", 0, 0.0, 1.0, true }, { "B", "2", 0, 0.5, 1.0, true } };
        hotbar->Style.SlotButtonStyle = MakeButtonStyle(ThemePreset::SciFi);
        hotbar->RebuildSlots();
        auto radial = std::make_shared<RadialMenu>();
        radial->Items = { { "Map" }, { "Items" }, { "Skills" }, { "Exit" } };
        auto toast = std::make_shared<ToastHost>();
        toast->AddToast("Smoke", "ToastHost ready.", 1.0f);

        stack->AddChild(button);
        stack->AddChild(slider);
        stack->AddChild(progress);
        stack->AddChild(list);
        stack->AddChild(combo);
        stack->AddChild(textBox);
        stack->AddChild(scrollViewer);
        stack->AddChild(tree);
        stack->AddChild(hotbar);
        stack->AddChild(radial);
        stack->AddChild(toast);
        root->AddChild(stack, 20.0f, 20.0f);

        root->Measure({ 800.0f, 600.0f });
        root->Arrange({ 0.0f, 0.0f, 800.0f, 600.0f });
        root->Update(0.016f);

        result.ControlsCreated = 11;
        result.Passed = root->DesiredSize.x >= 0.0f && root->DesiredSize.y >= 0.0f && !hotbar->Slots.empty() && combo->SelectedIndex == 7 && !tree->Items.empty() && scrollContentHitOk && scrollBarHitOk && scrollOutsideHitOk && scrollWheelRoutesOk && scrollMovesContentOk && textBoxSelectionOk && textBoxMultilineOk && listKeepsSelectionVisibleOk && comboKeepsSelectionVisibleOk;
        result.Message = result.Passed ? "FriendlyGUI smoke test passed." : "FriendlyGUI smoke test failed.";
        return result;
    }

    inline SmokeTestResult RunFriendlyControlsSmokeTest()
    {
        return RunSmokeTest();
    }

    class Separator : public UIElement
    {
    public:
        FyGUI::Orientation Orientation = FyGUI::Orientation::Horizontal;
        Color LineColor = { 0.32f, 0.38f, 0.48f, 1.0f };
        float ThicknessValue = 1.0f;
        SeparatorStyle Style;

        Separator()
        {
            IsHitTestVisible = false;
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            if (Orientation == FyGUI::Orientation::Horizontal)
                return { Width >= 0.0f ? Width : availableSize.x, ThicknessValue };
            return { ThicknessValue, Height >= 0.0f ? Height : availableSize.y };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            DrawStyledPart(drawList, Bounds, Style.PART_Root, GetVisualState());
            const Color lineColor = Style.PART_Line.BackgroundColor.a > 0.0f ? Style.PART_Line.BackgroundColor : LineColor;
            const float thickness = Style.PART_Line.BorderThickness > 0.0f ? Style.PART_Line.BorderThickness : ThicknessValue;
            if (Orientation == FyGUI::Orientation::Horizontal)
            {
                const float y = Bounds.y + Bounds.height * 0.5f;
                drawList.AddLine(Vec2(Bounds.Left(), y), Vec2(Bounds.Right(), y), lineColor.ToU32(Opacity), thickness);
            }
            else
            {
                const float x = Bounds.x + Bounds.width * 0.5f;
                drawList.AddLine(Vec2(x, Bounds.Top()), Vec2(x, Bounds.Bottom()), lineColor.ToU32(Opacity), thickness);
            }
        }
    };

    class InfoBadge : public Control
    {
    public:
        std::string Value;

        explicit InfoBadge(std::string text = {}) : Value(std::move(text))
        {
            Padding = Thickness(10.0f, 4.0f, 10.0f, 4.0f);
            CornerRadius = 10.0f;
            BorderThickness = Thickness(1.0f);
            BackgroundNormal = { 0.12f, 0.28f, 0.32f, 0.95f };
            BackgroundHover = BackgroundNormal;
            BackgroundPressed = BackgroundNormal;
            BorderBrush = { 0.38f, 0.78f, 0.84f, 0.95f };
            IsHitTestVisible = false;
        }

    protected:
        Vec2 MeasureOverride(Vec2) override
        {
            Vec2 size = MeasureText(Value);
            return { size.x + Padding.Horizontal(), size.y + Padding.Vertical() };
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            Control::RenderOverride(drawList);
            Vec2 size = MeasureText(Value);
            drawList.AddText(Vec2(Bounds.x + (Bounds.width - size.x) * 0.5f, Bounds.y + (Bounds.height - size.y) * 0.5f), Foreground.ToU32(Opacity), Value.c_str());
        }
    };

    class MenuItem : public Button
    {
    public:
        std::vector<std::shared_ptr<MenuItem>> Items;
        bool IsOpen = false;

        explicit MenuItem(std::string text = {}) : Button(std::move(text))
        {
            Width = 140.0f;
            Height = 30.0f;
            Padding = Thickness(10.0f, 6.0f, 10.0f, 6.0f);
            CornerRadius = 2.0f;
        }

        void AddItem(std::shared_ptr<MenuItem> item)
        {
            if (!item)
                return;
            AdoptChild(item);
            Items.push_back(std::move(item));
            InvalidateMeasure();
        }

        void VisitChildren(const std::function<void(UIElement*)>& visitor) override
        {
            for (auto& item : Items)
                if (item)
                    visitor(item.get());
        }

        UIElement* HitTest(Vec2 point) override
        {
            if (UIElement::HitTest(point))
                return this;
            if (IsOpen)
            {
                for (auto it = Items.rbegin(); it != Items.rend(); ++it)
                {
                    if (auto* hit = (*it)->HitTest(point))
                        return hit;
                }
            }
            return nullptr;
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            Vec2 base = Button::MeasureOverride(availableSize);
            for (auto& item : Items)
                item->Measure({ Width >= 0.0f ? Width : 160.0f, InfiniteSize });
            return base;
        }

        void ArrangeOverride(Rect finalRect) override
        {
            float y = finalRect.Bottom();
            for (auto& item : Items)
            {
                item->Arrange({ finalRect.x, y, finalRect.width, item->DesiredSize.y });
                y += item->DesiredSize.y;
            }
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            Button::RenderOverride(drawList);
            if (IsOpen)
            {
                for (auto& item : Items)
                    item->Render(drawList);
            }
        }

        void OnEvent(EventArgs& args) override
        {
            if (args.kind == EventKind::PointerReleased && IsPressed && !Items.empty())
            {
                IsOpen = !IsOpen;
                args.handled = true;
            }
            Button::OnEvent(args);
        }
    };

    class Menu : public StackPanel
    {
    public:
        Menu()
        {
            Orientation = FyGUI::Orientation::Horizontal;
            Spacing = 2.0f;
            Background = { 0.04f, 0.045f, 0.055f, 0.95f };
            BorderBrush = { 0.25f, 0.30f, 0.38f, 1.0f };
            BorderThickness = Thickness(1.0f);
            Padding = Thickness(4.0f);
        }
    };

    class CommandBar : public StackPanel
    {
    public:
        CommandBarStyle Style;

        CommandBar()
        {
            Orientation = FyGUI::Orientation::Horizontal;
            Spacing = 6.0f;
            Padding = Thickness(8.0f, 4.0f, 8.0f, 4.0f);
            Height = 48.0f;
            Background = ColorFromBytes(250, 249, 248, 255);
            BorderBrush = ColorFromBytes(218, 218, 218, 255);
            BorderThickness = Thickness(1.0f);
            CornerRadius = 4.0f;
            VerticalAlignment = FyGUI::VerticalAlignment::Top;
        }

    protected:
        void RenderOverride(ImDrawList& drawList) override
        {
            DrawStyledPart(drawList, Bounds, Style.PART_Root, GetVisualState());
            ControlPartStyle background = Style.PART_Background;
            if (!HasStyledPartVisual(background))
                background = SolidPart(Background, BorderBrush, BorderThickness.left, CornerRadius);
            DrawStyledPart(drawList, Bounds, background, GetVisualState());
            DrawStyledPart(drawList, DeflateRect(Bounds, BorderThickness), Style.PART_ContentPresenter, GetVisualState());
            for (auto& child : Children)
                child->Render(drawList);
            for (auto& child : Children)
                child->RenderOverlay(drawList);
            DrawStyledPart(drawList, Bounds, Style.PART_Border, GetVisualState());
        }
    };

    class AppBarButton : public Button
    {
    public:
        std::string Label;
        std::string Icon;
        AppBarButtonStyle AppBarStyle;

        AppBarButton() : Button()
        {
            Width = 108.0f;
            Height = 36.0f;
            Padding = Thickness(10.0f, 6.0f, 10.0f, 6.0f);
            CornerRadius = 4.0f;
            BackgroundNormal = ColorFromBytes(255, 255, 255, 0);
            BackgroundHover = ColorFromBytes(243, 242, 241, 255);
            BackgroundPressed = ColorFromBytes(237, 235, 233, 255);
            BorderBrush = ColorFromBytes(0, 0, 0, 0);
            BorderThickness = Thickness(0.0f);
            Foreground = ColorFromBytes(32, 31, 30, 255);
            HorizontalContentAlignment = FyGUI::HorizontalAlignment::Center;
            VerticalContentAlignment = FyGUI::VerticalAlignment::Center;
        }

    protected:
        void RenderOverride(ImDrawList& drawList) override
        {
            const ButtonStyle previous = Style;
            ButtonStyle mapped = Style;
            if (HasStyledPartVisual(AppBarStyle.PART_Background))
                mapped.PART_Background = AppBarStyle.PART_Background;
            if (HasStyledPartVisual(AppBarStyle.PART_Border))
                mapped.PART_Border = AppBarStyle.PART_Border;
            if (HasStyledPartVisual(AppBarStyle.PART_Icon))
                mapped.PART_Icon = AppBarStyle.PART_Icon;
            if (AppBarStyle.PART_Text.ForegroundColor.a > 0.0f)
                mapped.PART_Text = AppBarStyle.PART_Text;
            if (HasStyledPartVisual(AppBarStyle.PART_FocusVisual))
                mapped.PART_FocusVisual = AppBarStyle.PART_FocusVisual;
            Style = mapped;
            Button::RenderOverride(drawList);
            Style = previous;
        }
    };

    class AppBarSeparator : public Separator
    {
    public:
        AppBarSeparator()
        {
            Orientation = FyGUI::Orientation::Vertical;
            Width = 1.0f;
            Height = 28.0f;
            Margin = Thickness(4.0f, 4.0f, 4.0f, 4.0f);
            LineColor = ColorFromBytes(218, 218, 218, 255);
            ThicknessValue = 1.0f;
        }
    };

}


// ---- Begin inlined TabView control ----
// Inlined after Menu, Button and TextBlock are declared.
namespace FyGUI
{
    struct TabViewItem
    {
        std::string Header;
        std::shared_ptr<UIElement> Content;
    };

    class TabView : public Control
    {
    public:
        std::vector<TabViewItem> Items;
        int32_t SelectedIndex = 0;
        float HeaderHeight = 40.0f;
        bool IsAddTabButtonVisible = false;
        bool CanCloseTabs = false;
        float AddButtonWidth = 40.0f;
        Thickness ContentPadding = Thickness(12.0f, 8.0f, 12.0f, 8.0f);
        std::function<void()> OnAddTabClick;
        std::function<void(int32_t)> OnTabCloseRequested;
        std::function<void(int32_t)> OnSelectionChanged;
        TabViewStyle Style;

        TabView();
        void AddTab(std::string header, std::shared_ptr<UIElement> content);
        void RemoveTab(int32_t index);
        void SetSelectedIndex(int32_t index);
        UIElement* HitTest(Vec2 point) override;
        void Update(float deltaTime) override;
        void VisitChildren(const std::function<void(UIElement*)>& visitor) override;

    protected:
        float ContentTransitionSpeed = 16.0f;

        Vec2 MeasureOverride(Vec2 availableSize) override;
        void ArrangeOverride(Rect finalRect) override;
        void RenderOverride(ImDrawList& drawList) override;
        void OnEvent(EventArgs& args) override;
        bool OnAction(UIAction action) override;

    private:
        float TabWidthFor(const TabViewItem& item) const;
        float TabStartX(int32_t index) const;
        Rect CloseButtonRectFor(int32_t index) const;
        int32_t HitCloseIndex(Vec2 point) const;
        Rect AddButtonRect() const;
        int32_t HitHeaderIndex(float px) const;

        int32_t m_previousSelectedIndex = 0;
        float m_contentTransition = 1.0f;
    };

}

namespace FyGUI
{
    TabView::TabView()
    {
        Width = 520.0f;
        Height = 320.0f;
        BorderThickness = Thickness(1.0f);
        CornerRadius = 4.0f;
        BackgroundNormal = ColorFromBytes(255, 255, 255, 255);
        BackgroundHover = ColorFromBytes(255, 255, 255, 255);
        BackgroundPressed = ColorFromBytes(250, 250, 250, 255);
        Foreground = ColorFromBytes(32, 31, 30, 255);
        BorderBrush = ColorFromBytes(218, 218, 218, 255);
        IsTabStop = true;
        UseSystemFocusVisuals = false;
    }

    void TabView::AddTab(std::string header, std::shared_ptr<UIElement> content)
    {
        if (content)
            AdoptChild(content);
        Items.push_back({ std::move(header), std::move(content) });
        InvalidateMeasure();
    }

    void TabView::RemoveTab(int32_t index)
    {
        if (index < 0 || index >= static_cast<int32_t>(Items.size()))
            return;
        if (Items[static_cast<size_t>(index)].Content)
            DetachChild(Items[static_cast<size_t>(index)].Content.get());
        Items.erase(Items.begin() + index);
        if (Items.empty())
            SelectedIndex = -1;
        else
            SetSelectedIndex(std::min(SelectedIndex, static_cast<int32_t>(Items.size()) - 1));
        InvalidateMeasure();
    }

    void TabView::VisitChildren(const std::function<void(UIElement*)>& visitor)
    {
        for (auto& item : Items)
            if (item.Content)
                visitor(item.Content.get());
    }

    void TabView::SetSelectedIndex(int32_t index)
    {
        int32_t clamped = Items.empty() ? -1 : std::clamp(index, 0, static_cast<int32_t>(Items.size()) - 1);
        if (SelectedIndex == clamped)
            return;

        int32_t old = SelectedIndex;
        SelectedIndex = clamped;
        InvalidateArrange();
        if (OnSelectionChanged)
            OnSelectionChanged(SelectedIndex);

        SelectionChangedEventArgs args;
        args.kind = EventKind::SelectionChanged;
        args.route = RoutingStrategy::Bubble;
        args.originalSource = this;
        args.oldIndex = old;
        args.newIndex = SelectedIndex;
        RaiseEvent(args);
    }

    UIElement* TabView::HitTest(Vec2 point)
    {
        if (!UIElement::HitTest(point))
            return nullptr;
        if (SelectedIndex >= 0 && SelectedIndex < static_cast<int32_t>(Items.size()) && Items[static_cast<size_t>(SelectedIndex)].Content)
        {
            if (auto* hit = Items[static_cast<size_t>(SelectedIndex)].Content->HitTest(point))
                return hit;
        }
        return this;
    }

    void TabView::Update(float deltaTime)
    {
        Control::Update(deltaTime);
        if (m_previousSelectedIndex != SelectedIndex)
        {
            m_previousSelectedIndex = SelectedIndex;
            m_contentTransition = 0.0f;
        }
        const float step = std::clamp(deltaTime * ContentTransitionSpeed, 0.0f, 1.0f);
        m_contentTransition += (1.0f - m_contentTransition) * step;
        if (std::abs(m_contentTransition - 1.0f) < 0.001f)
            m_contentTransition = 1.0f;

        if (SelectedIndex >= 0 && SelectedIndex < static_cast<int32_t>(Items.size()) && Items[static_cast<size_t>(SelectedIndex)].Content)
            Items[static_cast<size_t>(SelectedIndex)].Content->Update(deltaTime);
    }

    Vec2 TabView::MeasureOverride(Vec2)
    {
        Vec2 desired = { Width >= 0.0f ? Width : 520.0f, Height >= 0.0f ? Height : 320.0f };
        Vec2 contentAvailable = {
            std::max(0.0f, desired.x - Padding.Horizontal() - BorderThickness.Horizontal() - ContentPadding.Horizontal()),
            std::max(0.0f, desired.y - HeaderHeight - Padding.Vertical() - BorderThickness.Vertical() - ContentPadding.Vertical())
        };
        for (auto& item : Items)
        {
            if (item.Content)
                item.Content->Measure(contentAvailable);
        }
        return desired;
    }

    void TabView::ArrangeOverride(Rect finalRect)
    {
        if (SelectedIndex < 0 || SelectedIndex >= static_cast<int32_t>(Items.size()))
            return;

        auto& item = Items[static_cast<size_t>(SelectedIndex)];
        if (!item.Content)
            return;

        item.Content->Arrange({
            finalRect.x + Padding.left + BorderThickness.left + ContentPadding.left,
            finalRect.y + HeaderHeight + Padding.top + BorderThickness.top + ContentPadding.top,
            std::max(0.0f, finalRect.width - Padding.Horizontal() - BorderThickness.Horizontal() - ContentPadding.Horizontal()),
            std::max(0.0f, finalRect.height - HeaderHeight - Padding.Vertical() - BorderThickness.Vertical() - ContentPadding.Vertical())
        });
    }

    void TabView::RenderOverride(ImDrawList& drawList)
    {
        VisualState state = GetVisualState();
        if (HasStyledPartVisual(Style.PART_Root))
            DrawStyledPart(drawList, Bounds, Style.PART_Root, state);
        ControlPartStyle background = Style.PART_Background;
        if (!HasStyledPartVisual(background))
        {
            background.BackgroundColor = IsPressed ? BackgroundPressed : (IsHovered ? BackgroundHover : BackgroundNormal);
            background.BorderColor = BorderBrush;
            background.BorderThickness = BorderThickness.left;
            background.CornerRadius = CornerRadius;
        }
        DrawStyledPart(drawList, Bounds, background, state);
        Rect headerPanel = { Bounds.x, Bounds.y, Bounds.width, HeaderHeight };
        if (HasStyledPartVisual(Style.PART_HeaderPanel))
            DrawStyledPart(drawList, headerPanel, Style.PART_HeaderPanel, state);
        else
            drawList.AddRectFilled({ headerPanel.Left(), headerPanel.Top() }, { headerPanel.Right(), headerPanel.Bottom() }, ColorFromBytes(243, 243, 243, 255), CornerRadius);

        float x = Bounds.x;
        for (size_t i = 0; i < Items.size(); ++i)
        {
            const float tabWidth = TabWidthFor(Items[i]);
            const bool selected = static_cast<int32_t>(i) == SelectedIndex;
            ControlPartStyle tab = selected ? Style.PART_SelectedTabViewItem : Style.PART_TabViewItem;
            if (!HasStyledPartVisual(tab))
            {
                tab.BackgroundColor = selected ? ColorFromBytes(255, 255, 255, 255) : ColorFromBytes(0, 0, 0, 0);
                tab.BorderColor = selected ? ColorFromBytes(218, 218, 218, 255) : ColorFromBytes(0, 0, 0, 0);
                tab.BorderThickness = selected ? 1.0f : 0.0f;
                tab.CornerRadius = 5.0f;
            }
            DrawStyledPart(drawList, { x, Bounds.y, tabWidth, HeaderHeight }, tab, selected ? VisualState::Selected : state);
            Color textColor = ResolvePartForeground(Style.PART_TabText, Foreground);
            const float closeReserve = CanCloseTabs ? 30.0f : 0.0f;
            const Rect textClip = { x + 16.0f, Bounds.y, std::max(0.0f, tabWidth - 28.0f - closeReserve), HeaderHeight };
            drawList.PushClipRect({ textClip.Left(), textClip.Top() }, { textClip.Right(), textClip.Bottom() }, true);
            drawList.AddText(GetDefaultFont(), GetFontSize(), Vec2(textClip.x, Bounds.y + std::floor((HeaderHeight - GetFontSize()) * 0.5f)), textColor.ToU32(Opacity), Items[i].Header.c_str());
            drawList.PopClipRect();
            if (CanCloseTabs)
            {
                Rect close = CloseButtonRectFor(static_cast<int32_t>(i));
                drawList.AddLine({ close.x + 7.0f, close.y + 7.0f }, { close.Right() - 7.0f, close.Bottom() - 7.0f }, ColorFromBytes(96, 94, 92), 1.0f);
                drawList.AddLine({ close.Right() - 7.0f, close.y + 7.0f }, { close.x + 7.0f, close.Bottom() - 7.0f }, ColorFromBytes(96, 94, 92), 1.0f);
            }
            if (selected)
                drawList.AddRectFilled({ x + 8.0f, Bounds.y + HeaderHeight - 3.0f }, { x + tabWidth - 8.0f, Bounds.y + HeaderHeight - 1.0f }, ColorFromBytes(0, 120, 212, 255), 1.0f);
            x += tabWidth;
        }
        if (IsAddTabButtonVisible)
        {
            const Rect add = AddButtonRect();
            if (add.Right() <= Bounds.Right())
            {
                drawList.AddRectFilled({ add.Left(), add.Top() }, { add.Right(), add.Bottom() }, ColorFromBytes(0, 0, 0, 0), 3.0f);
                drawList.AddRect({ add.Left(), add.Top() }, { add.Right(), add.Bottom() }, ColorFromBytes(0, 0, 0, 0), 3.0f, ImDrawFlags_None, 1.0f);
                const Vec2 center = { add.x + add.width * 0.5f, add.y + add.height * 0.5f };
                drawList.AddLine({ center.x - 5.0f, center.y }, { center.x + 5.0f, center.y }, ColorFromBytes(50, 49, 48), 1.4f);
                drawList.AddLine({ center.x, center.y - 5.0f }, { center.x, center.y + 5.0f }, ColorFromBytes(50, 49, 48), 1.4f);
            }
        }
        drawList.AddLine({ Bounds.Left(), Bounds.y + HeaderHeight }, { Bounds.Right(), Bounds.y + HeaderHeight }, ColorFromBytes(218, 218, 218, 255), 1.0f);

        DrawStyledPart(drawList, { Bounds.x, Bounds.y + HeaderHeight, Bounds.width, std::max(0.0f, Bounds.height - HeaderHeight) }, Style.PART_ContentPresenter, state);
        if (SelectedIndex >= 0 && SelectedIndex < static_cast<int32_t>(Items.size()) && Items[static_cast<size_t>(SelectedIndex)].Content)
        {
            const float previousOpacity = Items[static_cast<size_t>(SelectedIndex)].Content->Opacity;
            Items[static_cast<size_t>(SelectedIndex)].Content->Opacity = previousOpacity * EaseOutCubic(m_contentTransition);
            Items[static_cast<size_t>(SelectedIndex)].Content->Render(drawList);
            Items[static_cast<size_t>(SelectedIndex)].Content->Opacity = previousOpacity;
        }
        DrawStyledPart(drawList, Bounds, Style.PART_Border, state);
        if (IsFocused() && GetFocusVisualsEnabled() && SelectedIndex >= 0 && SelectedIndex < static_cast<int32_t>(Items.size()))
        {
            const float selectedX = TabStartX(SelectedIndex);
            const float selectedWidth = TabWidthFor(Items[static_cast<size_t>(SelectedIndex)]);
            drawList.AddRect({ selectedX + 2.0f, Bounds.y + 2.0f }, { selectedX + selectedWidth - 2.0f, Bounds.y + HeaderHeight - 2.0f }, ColorFromBytes(0, 120, 212, 210), 4.0f, ImDrawFlags_None, 1.2f);
        }
        if (IsFocused() && HasStyledPartVisual(Style.PART_FocusVisual))
            DrawStyledPart(drawList, Bounds, Style.PART_FocusVisual, VisualState::Focused);
    }

    void TabView::OnEvent(EventArgs& args)
    {
        if (args.kind == EventKind::PointerPressed)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            if (pointer.position.y >= Bounds.y && pointer.position.y <= Bounds.y + HeaderHeight)
            {
                if (IsAddTabButtonVisible && AddButtonRect().Contains(pointer.position))
                {
                    if (OnAddTabClick)
                        OnAddTabClick();
                    else
                    {
                        auto content = std::make_shared<TextBlock>("New tab content.");
                        content->Foreground = Foreground;
                        AddTab("New tab", content);
                        SetSelectedIndex(static_cast<int32_t>(Items.size()) - 1);
                    }
                    args.handled = true;
                    return;
                }
                if (CanCloseTabs)
                {
                    const int32_t closeIndex = HitCloseIndex(pointer.position);
                    if (closeIndex >= 0)
                    {
                        if (OnTabCloseRequested)
                            OnTabCloseRequested(closeIndex);
                        RemoveTab(closeIndex);
                        args.handled = true;
                        return;
                    }
                }
                int32_t index = HitHeaderIndex(pointer.position.x);
                SetSelectedIndex(index);
                args.handled = true;
            }
        }
    }

    bool TabView::OnAction(UIAction action)
    {
        if (action == UIAction::NavigateLeft || action == UIAction::PageLeft || action == UIAction::PageUp)
        {
            SetSelectedIndex(SelectedIndex - 1);
            return true;
        }
        if (action == UIAction::NavigateRight || action == UIAction::PageRight || action == UIAction::PageDown)
        {
            SetSelectedIndex(SelectedIndex + 1);
            return true;
        }
        return false;
    }

    float TabView::TabWidthFor(const TabViewItem& item) const
    {
        const float textWidth = MeasureText(item.Header).x;
        return std::clamp(textWidth + 56.0f + (CanCloseTabs ? 24.0f : 0.0f), 104.0f, 260.0f);
    }

    float TabView::TabStartX(int32_t index) const
    {
        float x = Bounds.x;
        for (int32_t i = 0; i < index && i < static_cast<int32_t>(Items.size()); ++i)
            x += TabWidthFor(Items[static_cast<size_t>(i)]);
        return x;
    }

    Rect TabView::CloseButtonRectFor(int32_t index) const
    {
        if (index < 0 || index >= static_cast<int32_t>(Items.size()))
            return {};
        const float x = TabStartX(index);
        const float width = TabWidthFor(Items[static_cast<size_t>(index)]);
        return { x + width - 32.0f, Bounds.y + 8.0f, 24.0f, HeaderHeight - 16.0f };
    }

    int32_t TabView::HitCloseIndex(Vec2 point) const
    {
        for (int32_t i = 0; i < static_cast<int32_t>(Items.size()); ++i)
        {
            if (CloseButtonRectFor(i).Contains(point))
                return i;
        }
        return -1;
    }

    Rect TabView::AddButtonRect() const
    {
        float x = Bounds.x;
        for (const auto& item : Items)
            x += TabWidthFor(item);
        return { x + 4.0f, Bounds.y + 4.0f, AddButtonWidth - 8.0f, HeaderHeight - 8.0f };
    }

    int32_t TabView::HitHeaderIndex(float px) const
    {
        float x = Bounds.x;
        for (size_t i = 0; i < Items.size(); ++i)
        {
            const float width = TabWidthFor(Items[i]);
            if (px >= x && px <= x + width)
                return static_cast<int32_t>(i);
            x += width;
        }
        return static_cast<int32_t>(Items.empty() ? -1 : Items.size() - 1);
    }
}

// ---- End inlined TabView control ----


namespace FyGUI
{

}


// ---- Begin inlined Expander control ----
// Inlined after Border, ComboBox and TabView are declared.
namespace FyGUI
{
    class Expander : public Border
    {
    public:
        std::string Header = "Expander";
        bool IsExpanded = true;
        FyGUI::ExpandDirection ExpandDirection = FyGUI::ExpandDirection::Down;
        FyGUI::HorizontalAlignment HorizontalContentAlignment = FyGUI::HorizontalAlignment::Stretch;
        FyGUI::VerticalAlignment VerticalContentAlignment = FyGUI::VerticalAlignment::Stretch;
        float HeaderHeight = 32.0f;
        float ExpansionProgress = 1.0f;
        float ExpansionAnimationSpeed = 14.0f;
        ExpanderStyle ExpanderParts;
        std::function<void()> OnExpanded;
        std::function<void()> OnCollapsed;

        Expander();

        void Update(float deltaTime) override;
        void SetIsExpanded(bool expanded);
        void SetExpandDirection(FyGUI::ExpandDirection direction);
        void Toggle();

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override;
        void ArrangeOverride(Rect finalRect) override;
        void RenderOverride(ImDrawList& drawList) override;
        void OnEvent(EventArgs& args) override;
        bool OnAction(UIAction action) override;

    private:
        bool IsVerticalExpand() const;
        Rect HeaderSlotRect(Rect rect) const;
        Rect ContentPresenterRect(Rect rect) const;
        Rect ContentSlotRect(Rect rect) const;

        FyGUI::ExpandDirection m_lastExpandDirection = FyGUI::ExpandDirection::Down;
    };
}

namespace FyGUI
{
    Expander::Expander()
    {
        Width = 260.0f;
        BorderThickness = Thickness(1.0f);
        CornerRadius = 5.0f;
        Padding = Thickness(10.0f);
        Background = ColorFromBytes(255, 255, 255, 255);
        Foreground = ColorFromBytes(32, 31, 30, 255);
        BorderBrush = ColorFromBytes(218, 218, 218, 255);
        IsTabStop = true;
        UseSystemFocusVisuals = false;
        VerticalAlignment = FyGUI::VerticalAlignment::Top;
        m_lastExpandDirection = ExpandDirection;
    }

    void Expander::Update(float deltaTime)
    {
        Border::Update(deltaTime);
        if (m_lastExpandDirection != ExpandDirection)
        {
            m_lastExpandDirection = ExpandDirection;
            InvalidateMeasure();
        }

        const float target = IsExpanded ? 1.0f : 0.0f;
        const float step = std::clamp(deltaTime * ExpansionAnimationSpeed, 0.0f, 1.0f);
        const float previous = ExpansionProgress;
        ExpansionProgress += (target - ExpansionProgress) * step;
        if (std::abs(ExpansionProgress - target) < 0.001f)
            ExpansionProgress = target;
        if (std::abs(previous - ExpansionProgress) > 0.0001f)
            InvalidateMeasure();
    }

    void Expander::SetIsExpanded(bool expanded)
    {
        if (IsExpanded == expanded)
            return;
        IsExpanded = expanded;
        InvalidateMeasure();
        EventArgs routed;
        routed.kind = IsExpanded ? EventKind::Expanded : EventKind::Collapsed;
        routed.route = RoutingStrategy::Bubble;
        routed.originalSource = this;
        RaiseEvent(routed);
        if (IsExpanded)
        {
            if (OnExpanded)
                OnExpanded();
        }
        else if (OnCollapsed)
        {
            OnCollapsed();
        }
    }

    void Expander::SetExpandDirection(FyGUI::ExpandDirection direction)
    {
        if (ExpandDirection == direction)
            return;
        ExpandDirection = direction;
        m_lastExpandDirection = direction;
        InvalidateMeasure();
    }

    void Expander::Toggle()
    {
        SetIsExpanded(!IsExpanded);
    }

    Vec2 Expander::MeasureOverride(Vec2 availableSize)
    {
        const bool vertical = IsVerticalExpand();
        Vec2 desired = {
            Width >= 0.0f ? Width : std::min(availableSize.x, 500.0f),
            Height >= 0.0f ? Height : HeaderHeight + BorderThickness.Vertical()
        };
        if (Child && ExpansionProgress > 0.0f)
        {
            const float contentPaddingX = Padding.Horizontal() + BorderThickness.Horizontal();
            const float contentPaddingY = Padding.Vertical() + BorderThickness.Vertical();
            const Vec2 childAvailable = vertical ?
                Vec2{ std::max(0.0f, desired.x - contentPaddingX), InfiniteSize } :
                Vec2{ InfiniteSize, std::max(0.0f, desired.y - contentPaddingY) };
            Child->Measure(childAvailable);
            const float eased = EaseOutCubic(ExpansionProgress);
            if (vertical)
            {
                desired.x = std::max(desired.x, Child->DesiredSize.x + Padding.Horizontal() + BorderThickness.Horizontal());
                desired.y += (Child->DesiredSize.y + Padding.Vertical()) * eased;
            }
            else
            {
                desired.x += (Child->DesiredSize.x + Padding.Horizontal()) * eased;
                desired.y = std::max(desired.y, Child->DesiredSize.y + Padding.Vertical() + BorderThickness.Vertical());
            }
        }
        return desired;
    }

    void Expander::ArrangeOverride(Rect finalRect)
    {
        if (!Child || ExpansionProgress <= 0.0f)
            return;
        Rect content = ContentSlotRect(finalRect);
        float childWidth = HorizontalContentAlignment == FyGUI::HorizontalAlignment::Stretch ? content.width : std::min(content.width, Child->DesiredSize.x);
        float childHeight = VerticalContentAlignment == FyGUI::VerticalAlignment::Stretch ? content.height : std::min(content.height, Child->DesiredSize.y);
        float childX = content.x;
        float childY = content.y;
        if (HorizontalContentAlignment == FyGUI::HorizontalAlignment::Center)
            childX += (content.width - childWidth) * 0.5f;
        else if (HorizontalContentAlignment == FyGUI::HorizontalAlignment::Right)
            childX += content.width - childWidth;
        if (VerticalContentAlignment == FyGUI::VerticalAlignment::Center)
            childY += (content.height - childHeight) * 0.5f;
        else if (VerticalContentAlignment == FyGUI::VerticalAlignment::Bottom)
            childY += content.height - childHeight;
        Child->Arrange({ childX, childY, std::max(0.0f, childWidth), std::max(0.0f, childHeight) });
    }

    void Expander::RenderOverride(ImDrawList& drawList)
    {
        VisualState state = GetVisualState();
        DrawStyledPart(drawList, Bounds, ExpanderParts.PART_Root, state);
        ControlPartStyle background = Style.PART_Background;
        if (!HasStyledPartVisual(background))
        {
            background.BackgroundColor = Background;
            background.BorderColor = BorderBrush;
            background.BorderThickness = BorderThickness.left;
            background.CornerRadius = CornerRadius;
        }
        DrawStyledPart(drawList, Bounds, background, state);

        Rect headerRect = HeaderSlotRect(Bounds);
        ControlPartStyle header = ExpanderParts.PART_Header;
        if (!HasStyledPartVisual(header))
        {
            header.BackgroundColor = IsFocused() ? Color{ 0.92f, 0.95f, 0.98f, 1.0f } : Color{ 0.98f, 0.98f, 0.98f, 1.0f };
            header.BorderColor = Color{ 0.88f, 0.88f, 0.88f, 1.0f };
            header.BorderThickness = 1.0f;
            header.CornerRadius = CornerRadius;
        }
        DrawStyledPart(drawList, headerRect, header, state);

        Rect glyphRect = { headerRect.Right() - Padding.right - 22.0f, headerRect.y + (HeaderHeight - 16.0f) * 0.5f, 16.0f, 16.0f };
        if (HasStyledPartVisual(ExpanderParts.PART_ExpanderGlyph))
        {
            DrawStyledPart(drawList, glyphRect, ExpanderParts.PART_ExpanderGlyph, IsExpanded ? VisualState::Selected : state);
        }
        else
        {
            const Vec2 center = { glyphRect.x + glyphRect.width * 0.5f, glyphRect.y + glyphRect.height * 0.5f };
            const Color glyphColor = ColorFromBytes(50, 49, 48, 255);
            auto drawChevron = [&](char direction)
            {
                if (direction == 'v')
                {
                    drawList.AddLine({ center.x - 4.5f, center.y - 1.5f }, { center.x, center.y + 3.0f }, glyphColor, 1.4f);
                    drawList.AddLine({ center.x, center.y + 3.0f }, { center.x + 4.5f, center.y - 1.5f }, glyphColor, 1.4f);
                }
                else if (direction == '^')
                {
                    drawList.AddLine({ center.x - 4.5f, center.y + 2.0f }, { center.x, center.y - 2.5f }, glyphColor, 1.4f);
                    drawList.AddLine({ center.x, center.y - 2.5f }, { center.x + 4.5f, center.y + 2.0f }, glyphColor, 1.4f);
                }
                else if (direction == '<')
                {
                    drawList.AddLine({ center.x + 2.0f, center.y - 4.5f }, { center.x - 2.5f, center.y }, glyphColor, 1.4f);
                    drawList.AddLine({ center.x - 2.5f, center.y }, { center.x + 2.0f, center.y + 4.5f }, glyphColor, 1.4f);
                }
                else
                {
                    drawList.AddLine({ center.x - 2.0f, center.y - 4.5f }, { center.x + 2.5f, center.y }, glyphColor, 1.4f);
                    drawList.AddLine({ center.x + 2.5f, center.y }, { center.x - 2.0f, center.y + 4.5f }, glyphColor, 1.4f);
                }
            };
            char direction = '>';
            if (IsExpanded)
            {
                if (ExpandDirection == FyGUI::ExpandDirection::Down) direction = 'v';
                else if (ExpandDirection == FyGUI::ExpandDirection::Up) direction = '^';
                else if (ExpandDirection == FyGUI::ExpandDirection::Left) direction = '<';
                else direction = '>';
            }
            drawChevron(direction);
        }

        ControlPartStyle headerText = ExpanderParts.PART_HeaderText;
        Color textColor = headerText.ForegroundColor.a > 0.0f ? headerText.ForegroundColor : Foreground;
        drawList.AddText(Vec2(headerRect.x + Padding.left, headerRect.y + (HeaderHeight - GetFontSize()) * 0.5f), textColor.ToU32(Opacity), Header.c_str());

        if (ExpansionProgress > 0.0f)
        {
            Rect contentRect = ContentPresenterRect(Bounds);
            DrawStyledPart(drawList, contentRect, ApplyPartOpacity(ExpanderParts.PART_ContentPresenter, ExpansionProgress), state);
        }
        if (ExpansionProgress > 0.0f && Child)
        {
            Rect clipRect = ContentPresenterRect(Bounds);
            drawList.PushClipRect(Vec2(clipRect.Left(), clipRect.Top()), Vec2(clipRect.Right(), clipRect.Bottom()), true);
            const float previousOpacity = Child->Opacity;
            Child->Opacity = previousOpacity * EaseOutCubic(ExpansionProgress);
            Child->Render(drawList);
            Child->Opacity = previousOpacity;
            drawList.PopClipRect();
        }
        DrawStyledPart(drawList, Bounds, ExpanderParts.PART_Border, state);
        if (IsFocused() && GetFocusVisualsEnabled())
        {
            if (HasStyledPartVisual(ExpanderParts.PART_FocusVisual))
                DrawStyledPart(drawList, headerRect, ExpanderParts.PART_FocusVisual, VisualState::Focused);
            else
                drawList.AddRect({ headerRect.Left() + 0.5f, headerRect.Top() + 0.5f }, { headerRect.Right() - 0.5f, headerRect.Bottom() - 0.5f }, ColorFromBytes(0, 120, 212, 210), CornerRadius, ImDrawFlags_None, 1.2f);
        }
    }

    void Expander::OnEvent(EventArgs& args)
    {
        if (args.kind == EventKind::PointerPressed)
        {
            const auto& pointer = static_cast<PointerEventArgs&>(args);
            if (HeaderSlotRect(Bounds).Contains(pointer.position))
            {
                Toggle();
                args.handled = true;
            }
        }
    }

    bool Expander::OnAction(UIAction action)
    {
        if (action != UIAction::Accept)
            return false;
        Toggle();
        return true;
    }

    bool Expander::IsVerticalExpand() const
    {
        return ExpandDirection == FyGUI::ExpandDirection::Down || ExpandDirection == FyGUI::ExpandDirection::Up;
    }

    Rect Expander::HeaderSlotRect(Rect rect) const
    {
        if (ExpandDirection == FyGUI::ExpandDirection::Up)
            return { rect.x, rect.Bottom() - HeaderHeight, rect.width, HeaderHeight };
        if (ExpandDirection == FyGUI::ExpandDirection::Left)
            return { rect.Right() - HeaderHeight, rect.y, HeaderHeight, rect.height };
        if (ExpandDirection == FyGUI::ExpandDirection::Right)
            return { rect.x, rect.y, HeaderHeight, rect.height };
        return { rect.x, rect.y, rect.width, HeaderHeight };
    }

    Rect Expander::ContentPresenterRect(Rect rect) const
    {
        if (ExpandDirection == FyGUI::ExpandDirection::Up)
            return { rect.x + BorderThickness.left, rect.y + BorderThickness.top, std::max(0.0f, rect.width - BorderThickness.Horizontal()), std::max(0.0f, rect.height - HeaderHeight - BorderThickness.Vertical()) };
        if (ExpandDirection == FyGUI::ExpandDirection::Left)
            return { rect.x + BorderThickness.left, rect.y + BorderThickness.top, std::max(0.0f, rect.width - HeaderHeight - BorderThickness.Horizontal()), std::max(0.0f, rect.height - BorderThickness.Vertical()) };
        if (ExpandDirection == FyGUI::ExpandDirection::Right)
            return { rect.x + HeaderHeight + BorderThickness.left, rect.y + BorderThickness.top, std::max(0.0f, rect.width - HeaderHeight - BorderThickness.Horizontal()), std::max(0.0f, rect.height - BorderThickness.Vertical()) };
        return { rect.x + BorderThickness.left, rect.y + HeaderHeight, std::max(0.0f, rect.width - BorderThickness.Horizontal()), std::max(0.0f, rect.height - HeaderHeight - BorderThickness.bottom) };
    }

    Rect Expander::ContentSlotRect(Rect rect) const
    {
        Rect content = ContentPresenterRect(rect);
        return {
            content.x + Padding.left,
            content.y + Padding.top,
            std::max(0.0f, content.width - Padding.Horizontal()),
            std::max(0.0f, content.height - Padding.Vertical())
        };
    }
}

// ---- End inlined Expander control ----


namespace FyGUI
{

    class Popup : public Border
    {
    public:
        bool IsOpen = false;
        Vec2 Placement = {};

        Popup()
        {
            Width = 240.0f;
            Height = 140.0f;
            Padding = Thickness(12.0f);
            CornerRadius = 8.0f;
            BorderThickness = Thickness(1.0f);
            Background = { 0.03f, 0.035f, 0.045f, 0.98f };
            BorderBrush = { 0.46f, 0.58f, 0.74f, 0.95f };
        }

        void SetIsOpen(bool value)
        {
            if (IsOpen == value)
                return;
            IsOpen = value;
            InvalidateMeasure();
        }

    protected:
        Vec2 MeasureOverride(Vec2 availableSize) override
        {
            if (!IsOpen)
                return {};
            return Border::MeasureOverride(availableSize);
        }

        void ArrangeOverride(Rect finalRect) override
        {
            if (!IsOpen)
                return;
            const bool hasExplicitPlacement = Placement.x != 0.0f || Placement.y != 0.0f;
            Rect placed = { hasExplicitPlacement ? Placement.x : finalRect.x, hasExplicitPlacement ? Placement.y : finalRect.y, finalRect.width, finalRect.height };
            Bounds = placed;
            Border::ArrangeOverride(placed);
        }

        void RenderOverride(ImDrawList& drawList) override
        {
            if (!IsOpen)
                return;
            Border::RenderOverride(drawList);
        }
    };

    using Flyout = Popup;

    class FocusManager
    {
    public:
        UIElement* GetFocusedElement() const { return m_focused; }

        void SetFocusedElement(UIElement* element)
        {
            if (m_focused == element)
                return;

            UIElement* old = m_focused;
            if (old)
            {
                old->SetFocused(false);
                EventArgs lost;
                lost.kind = EventKind::LostFocus;
                lost.route = RoutingStrategy::Bubble;
                lost.originalSource = old;
                old->RaiseEvent(lost);
            }

            m_focused = element;
            if (m_focused)
            {
                m_focused->SetFocused(true);
                EventArgs got;
                got.kind = EventKind::GotFocus;
                got.route = RoutingStrategy::Bubble;
                got.originalSource = m_focused;
                m_focused->RaiseEvent(got);
            }
        }

    private:
        UIElement* m_focused = nullptr;
    };

    class Context
    {
    public:
        void SetRoot(std::shared_ptr<UIElement> root)
        {
            m_focusManager.SetFocusedElement(nullptr);
            m_hovered = nullptr;
            m_pressed = nullptr;
            m_root = std::move(root);
            if (m_root)
            {
                m_root->m_parent = nullptr;
                m_root->InvalidateMeasure();
            }
        }

        std::shared_ptr<UIElement> GetRoot() const { return m_root; }
        FocusManager& GetFocusManager() { return m_focusManager; }
        Cursor GetCursor() const { return m_hovered ? m_hovered->ProtectedCursor : Cursor::Arrow; }
        bool ShowDragGhost = true;

        UIElement* FindName(std::string_view name) const
        {
            return m_root ? m_root->FindName(name) : nullptr;
        }

        template<class T>
        T* FindName(std::string_view name) const
        {
            return dynamic_cast<T*>(FindName(name));
        }

        void InvalidateLayout()
        {
            if (m_root)
                m_root->InvalidateMeasure();
            m_activeUpdateSeconds = 0.35f;
        }

        void FocusNext(bool backwards = false)
        {
            if (!m_root)
                return;

            std::vector<UIElement*> tabStops;
            CollectTabStops(m_root.get(), tabStops);
            if (tabStops.empty())
                return;

            UIElement* current = m_focusManager.GetFocusedElement();
            auto it = std::find(tabStops.begin(), tabStops.end(), current);
            if (it == tabStops.end())
            {
                m_focusManager.SetFocusedElement(tabStops.front());
                return;
            }

            int index = static_cast<int>(std::distance(tabStops.begin(), it));
            index += backwards ? -1 : 1;
            if (index < 0)
                index = static_cast<int>(tabStops.size()) - 1;
            if (index >= static_cast<int>(tabStops.size()))
                index = 0;
            m_focusManager.SetFocusedElement(tabStops[static_cast<size_t>(index)]);
        }

        void Update(const InputSnapshot& input, Vec2 size, float deltaTime = 0.0f, bool processInput = true)
        {
            if (!m_root)
                return;

            TimeSecondsStorage() += deltaTime;
            const bool inputActive = processInput && HasInputActivity(input);
            if (inputActive || m_activeUpdateSeconds > 0.0f)
            {
                m_root->Update(deltaTime);
                m_activeUpdateSeconds = std::max(0.0f, m_activeUpdateSeconds - deltaTime);
            }

            const Rect rootRect { 0.0f, 0.0f, size.x, size.y };
            if (!m_root->IsMeasureValidFor(size))
                m_root->Measure(size);
            if (!m_root->IsArrangeValidFor(rootRect))
                m_root->Arrange(rootRect);
            if (processInput)
            {
                ProcessInput(input);
                if (inputActive)
                    m_activeUpdateSeconds = 0.35f;
            }
            else
            {
                ClearPointerState();
            }
        }

        void Render(ImDrawList& drawList)
        {
            if (!m_root)
                return;
            m_root->Render(drawList);
            DrawDragGhost(drawList);
            DrawToolTip(drawList);
        }

        void Render(ImDrawList* drawList)
        {
            if (drawList)
                Render(*drawList);
        }

        void UpdateAndRender(ImDrawList& drawList, const InputSnapshot& input, Vec2 size, float deltaTime = 0.0f, bool processInput = true)
        {
            Update(input, size, deltaTime, processInput);
            Render(drawList);
        }

#if FRIENDLYGUI_USE_IMGUI_INPUT
        static void SetImGuiKey(InputSnapshot& input, Key key, ImGuiKey imguiKey)
        {
            const size_t index = static_cast<size_t>(key);
            if (index == 0 || index >= KeyCount)
                return;
            input.KeyDown[index] = ImGui::IsKeyDown(imguiKey);
            input.KeyPressed[index] = ImGui::IsKeyPressed(imguiKey, false);
            input.KeyReleased[index] = ImGui::IsKeyReleased(imguiKey);
        }

        static InputSnapshot BuildInputFromImGui()
        {
            InputSnapshot input {};
            if (!ImGui::GetCurrentContext())
                return input;

            ImGuiIO& io = ImGui::GetIO();
            input.PointerPosition = { io.MousePos.x, io.MousePos.y };
            input.PointerDelta = { io.MouseDelta.x, io.MouseDelta.y };
            input.WheelDelta = io.MouseWheel;
            for (int i = 0; i < 3; ++i)
            {
                input.MouseDown[i] = ImGui::IsMouseDown(i);
                input.MousePressed[i] = ImGui::IsMouseClicked(i, false);
                input.MouseReleased[i] = ImGui::IsMouseReleased(i);
            }

            input.ShiftDown = io.KeyShift;
            input.ControlDown = io.KeyCtrl;
            input.AltDown = io.KeyAlt;
            SetImGuiKey(input, Key::Tab, ImGuiKey_Tab);
            SetImGuiKey(input, Key::Enter, ImGuiKey_Enter);
            SetImGuiKey(input, Key::Escape, ImGuiKey_Escape);
            SetImGuiKey(input, Key::Space, ImGuiKey_Space);
            SetImGuiKey(input, Key::Backspace, ImGuiKey_Backspace);
            SetImGuiKey(input, Key::DeleteKey, ImGuiKey_Delete);
            SetImGuiKey(input, Key::Left, ImGuiKey_LeftArrow);
            SetImGuiKey(input, Key::Right, ImGuiKey_RightArrow);
            SetImGuiKey(input, Key::Up, ImGuiKey_UpArrow);
            SetImGuiKey(input, Key::Down, ImGuiKey_DownArrow);
            SetImGuiKey(input, Key::Home, ImGuiKey_Home);
            SetImGuiKey(input, Key::End, ImGuiKey_End);
            SetImGuiKey(input, Key::PageUp, ImGuiKey_PageUp);
            SetImGuiKey(input, Key::PageDown, ImGuiKey_PageDown);
            const size_t shiftIndex = static_cast<size_t>(Key::Shift);
            const size_t controlIndex = static_cast<size_t>(Key::Control);
            const size_t altIndex = static_cast<size_t>(Key::Alt);
            input.KeyDown[shiftIndex] = io.KeyShift;
            input.KeyPressed[shiftIndex] = ImGui::IsKeyPressed(ImGuiKey_LeftShift, false) || ImGui::IsKeyPressed(ImGuiKey_RightShift, false);
            input.KeyReleased[shiftIndex] = ImGui::IsKeyReleased(ImGuiKey_LeftShift) || ImGui::IsKeyReleased(ImGuiKey_RightShift);
            input.KeyDown[controlIndex] = io.KeyCtrl;
            input.KeyPressed[controlIndex] = ImGui::IsKeyPressed(ImGuiKey_LeftCtrl, false) || ImGui::IsKeyPressed(ImGuiKey_RightCtrl, false);
            input.KeyReleased[controlIndex] = ImGui::IsKeyReleased(ImGuiKey_LeftCtrl) || ImGui::IsKeyReleased(ImGuiKey_RightCtrl);
            input.KeyDown[altIndex] = io.KeyAlt;
            input.KeyPressed[altIndex] = ImGui::IsKeyPressed(ImGuiKey_LeftAlt, false) || ImGui::IsKeyPressed(ImGuiKey_RightAlt, false);
            input.KeyReleased[altIndex] = ImGui::IsKeyReleased(ImGuiKey_LeftAlt) || ImGui::IsKeyReleased(ImGuiKey_RightAlt);
            for (int i = 0; i < 26; ++i)
                SetImGuiKey(input, static_cast<Key>(static_cast<int>(Key::A) + i), static_cast<ImGuiKey>(static_cast<int>(ImGuiKey_A) + i));
            for (int i = 0; i < 10; ++i)
                SetImGuiKey(input, static_cast<Key>(static_cast<int>(Key::Num0) + i), static_cast<ImGuiKey>(static_cast<int>(ImGuiKey_0) + i));

            for (ImWchar c : io.InputQueueCharacters)
                if (c != 0)
                    input.TextInputCodepoints.push_back(static_cast<uint32_t>(c));

#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM >= 18700
            input.GamepadConnected = (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
            input.GamepadAcceptPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, false);
            input.GamepadCancelPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false);
            input.GamepadSecondaryPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft, false);
            input.GamepadDetailsPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp, false);
            input.GamepadBackPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadBack, false);
            input.GamepadMenuPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false);
            input.GamepadNavigateUpPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp, false);
            input.GamepadNavigateDownPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown, false);
            input.GamepadNavigateLeftPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickLeft, false);
            input.GamepadNavigateRightPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickRight, false);
            input.GamepadPageLeftPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false);
            input.GamepadPageRightPressed = ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false);
            input.GamepadFocusPreviousPressed = input.GamepadPageLeftPressed;
            input.GamepadFocusNextPressed = input.GamepadPageRightPressed;
#endif
            PopulateGenericActions(input);
            return input;
        }

        void UpdateAndRenderFromImGui(ImDrawList* drawList, Vec2 viewportSize, float deltaTime)
        {
            InputSnapshot input = BuildInputFromImGui();
            Update(input, viewportSize, deltaTime, true);
            Render(drawList);
        }
#endif

        void ProcessInput(const InputSnapshot& input)
        {
            m_lastInput = input;
            Vec2 mouse = input.PointerPosition;
            UIElement* hit = m_root ? m_root->HitTest(mouse) : nullptr;

            if (IsKeyPressed(input, Key::Tab))
                FocusNext(input.ShiftDown);
            if (input.GamepadFocusNextPressed)
                FocusNext(false);
            if (input.GamepadFocusPreviousPressed)
                FocusNext(true);
            if (!m_focusManager.GetFocusedElement() && IsGamepadNavigationPressed(input))
                FocusNext(false);

            if (hit != m_hovered)
            {
                if (m_hovered)
                    RaisePointer(EventKind::PointerExited, m_hovered, mouse, PointerButton::None, false);
                m_hovered = hit;
                if (m_hovered)
                    RaisePointer(EventKind::PointerEntered, m_hovered, mouse, PointerButton::None, false);
            }

            if (input.MousePressed[0] && hit)
            {
                if (hit->IsTabStop)
                    m_focusManager.SetFocusedElement(hit);
                else
                    m_focusManager.SetFocusedElement(nullptr);
                RaisePointer(EventKind::PointerPressed, hit, mouse, PointerButton::Left, true);
                m_pressed = hit;
                m_dragSource = IsElementDragSource(hit) ? hit : nullptr;
                m_dragStart = mouse;
                m_dragCurrent = mouse;
                m_dragTarget = nullptr;
                m_isDragging = false;
            }

            UIElement* moveTarget = ((input.MouseDown[0] || (m_pressed && m_pressed->IsPointerCaptured(0))) && m_pressed) ? m_pressed : m_hovered;
            if (moveTarget)
                RaisePointer(EventKind::PointerMoved, moveTarget, mouse, PointerButton::None, input.MouseDown[0]);

            UpdateDragState(input, hit, mouse);

            if (input.MouseReleased[0])
            {
                if (m_isDragging)
                {
                    UIElement* dropTarget = FindDropTarget(hit);
                    if (dropTarget)
                        RaiseDrag(EventKind::Drop, dropTarget, mouse);
                    if (m_dragSource)
                        RaiseDrag(EventKind::DragCompleted, m_dragSource, mouse);
                }

                UIElement* target = m_pressed ? m_pressed : hit;
                if (target)
                {
                    RaisePointer(EventKind::PointerReleased, target, mouse, PointerButton::Left, false);
                    target->ReleasePointer(0);
                }
                m_pressed = nullptr;
                m_dragSource = nullptr;
                m_dragTarget = nullptr;
                m_isDragging = false;
            }

            float wheelDelta = input.WheelDelta;
            if (std::fabs(wheelDelta) >= 10.0f)
                wheelDelta /= 120.0f;
            wheelDelta = std::clamp(wheelDelta, -3.0f, 3.0f);

            if (wheelDelta != 0.0f && hit)
            {
                PointerEventArgs args;
                args.kind = EventKind::PointerWheelChanged;
                args.route = RoutingStrategy::Bubble;
                args.originalSource = hit;
                args.position = mouse;
                args.screenPosition = mouse;
                args.wheelDelta = { 0.0f, wheelDelta };
                hit->RaiseEvent(args);
            }

            if (m_focusManager.GetFocusedElement())
                DispatchFocusedKeyboard(input);
        }

    private:
        void RaisePointer(EventKind kind, UIElement* target, Vec2 position, PointerButton button, bool inContact)
        {
            PointerEventArgs args;
            args.kind = kind;
            args.route = RoutingStrategy::Bubble;
            args.originalSource = target;
            args.position = position;
            args.screenPosition = position;
            args.changedButton = button;
            args.buttons = inContact ? PointerButtons::Left : PointerButtons::None;
            args.isInContact = inContact;
            target->RaiseEvent(args);
        }

        void ClearPointerState()
        {
            m_hovered = nullptr;
            m_pressed = nullptr;
            m_dragSource = nullptr;
            m_dragTarget = nullptr;
            m_isDragging = false;
        }

        void UpdateDragState(const InputSnapshot& input, UIElement* hit, Vec2 mouse)
        {
            if (!input.MouseDown[0] || !m_dragSource)
                return;

            m_dragCurrent = mouse;
            const float dx = mouse.x - m_dragStart.x;
            const float dy = mouse.y - m_dragStart.y;
            const bool beyondThreshold = (dx * dx + dy * dy) >= 36.0f;

            if (!m_isDragging && beyondThreshold)
            {
                m_isDragging = true;
                RaiseDrag(EventKind::DragStarted, m_dragSource, mouse);
            }

            if (!m_isDragging)
                return;

            UIElement* dropTarget = FindDropTarget(hit);
            if (dropTarget != m_dragTarget)
            {
                if (m_dragTarget)
                    RaiseDrag(EventKind::DragLeave, m_dragTarget, mouse);
                m_dragTarget = dropTarget;
                if (m_dragTarget)
                    RaiseDrag(EventKind::DragEnter, m_dragTarget, mouse);
            }

            if (m_dragTarget)
                RaiseDrag(EventKind::DragOver, m_dragTarget, mouse);
        }

        UIElement* FindDropTarget(UIElement* hit) const
        {
            for (UIElement* current = hit; current; current = current->GetParent())
            {
                if (current->AllowDrop)
                    return current;
            }
            return nullptr;
        }

        bool IsElementDragSource(UIElement* element) const
        {
            return element && (element->IsDragSource || !element->DragPayload.empty());
        }

        void DrawToolTip(ImDrawList& drawList)
        {
            if (!m_hovered || m_hovered->ToolTip.empty() || m_isDragging)
                return;

            const bool hasTitle = !m_hovered->ToolTipTitle.empty();
            Vec2 titleSize = hasTitle ? MeasureText(m_hovered->ToolTipTitle) : Vec2{ 0.0f, 0.0f };
            Vec2 textSize = MeasureText(m_hovered->ToolTip, 280.0f);
            const float width = std::max({ 160.0f, titleSize.x, textSize.x }) + 24.0f;
            const float height = textSize.y + 20.0f + (hasTitle ? titleSize.y + 8.0f : 0.0f);
            Vec2 pos(m_lastInput.PointerPosition.x + 14.0f, m_lastInput.PointerPosition.y + 18.0f);
            Vec2 max(pos.x + width, pos.y + height);
            drawList.AddRectFilled(pos, max, ColorFromBytes(14, 18, 31, 235), 12.0f);
            drawList.AddRect(pos, max, m_hovered->ToolTipAccent.ToU32(0.82f), 12.0f, ImDrawFlags_None, 1.5f);
            drawList.AddRectFilled(Vec2{ pos.x, pos.y }, Vec2{ pos.x + 4.0f, max.y }, m_hovered->ToolTipAccent.ToU32(0.90f), 12.0f);
            float y = pos.y + 9.0f;
            if (hasTitle)
            {
                drawList.AddText(Vec2{ pos.x + 12.0f, y }, m_hovered->ToolTipAccent.ToU32(1.0f), m_hovered->ToolTipTitle.c_str());
                y += titleSize.y + 6.0f;
            }
            drawList.AddText(GetDefaultFont(), GetFontSize(), Vec2{ pos.x + 12.0f, y }, ColorFromBytes(235, 241, 255, 255), m_hovered->ToolTip.c_str(), nullptr, 280.0f);
        }

        void DrawDragGhost(ImDrawList& drawList)
        {
            if (!ShowDragGhost || !m_isDragging || !m_dragSource)
                return;

            if (auto* slot = dynamic_cast<InventorySlot*>(m_dragSource))
            {
                const float size = 58.0f;
                Vec2 pos(m_dragCurrent.x + 18.0f, m_dragCurrent.y + 18.0f);
                Rect ghostRect = { pos.x, pos.y, size, size };
                const float pulse = 0.5f + 0.5f * std::sin(GetTimeSeconds() * 12.0f);
                ghostRect = ScaleRectFromCenter(ghostRect, 1.0f + pulse * 0.035f);
                pos = { ghostRect.x, ghostRect.y };
                VisualState state = m_dragTarget ? VisualState::Selected : VisualState::PointerOver;

                ControlPartStyle ghost = slot->Style.PART_DragGhost;
                if (!HasStyledPartVisual(ghost))
                {
                    ghost.BackgroundColor = m_dragTarget ? Color{ 0.10f, 0.32f, 0.34f, 0.88f } : Color{ 0.06f, 0.08f, 0.12f, 0.88f };
                    ghost.BorderColor = m_dragTarget ? Color{ 0.30f, 1.0f, 0.92f, 1.0f } : Color{ 0.62f, 0.76f, 0.92f, 0.92f };
                    ghost.BorderThickness = 2.0f;
                    ghost.CornerRadius = 10.0f;
                }
                drawList.AddRectFilled({ ghostRect.Left() + 5.0f, ghostRect.Top() + 7.0f }, { ghostRect.Right() + 5.0f, ghostRect.Bottom() + 7.0f }, ColorFromBytes(0, 0, 0, 86), ghost.CornerRadius);
                DrawStyledPart(drawList, ghostRect, ghost, state);

                if (slot->ItemIcon.texture)
                {
                    ControlPartStyle icon = slot->Style.PART_ItemIcon;
                    icon.Image = slot->ItemIcon.texture;
                    icon.UseImage = true;
                    DrawStyledPart(drawList, { pos.x + 9.0f, pos.y + 9.0f, size - 18.0f, size - 18.0f }, icon, state);
                }
                else if (slot->ItemVisualRenderer)
                {
                    slot->ItemVisualRenderer(drawList, { pos.x + 8.0f, pos.y + 8.0f, size - 16.0f, size - 16.0f }, *slot, state);
                }

                if (!slot->QuantityText.empty())
                {
                    Vec2 quantitySize = MeasureText(slot->QuantityText);
                    drawList.AddText(Vec2{ pos.x + size - quantitySize.x - 5.0f, pos.y + size - quantitySize.y - 4.0f }, ColorFromBytes(245, 248, 255, 255), slot->QuantityText.c_str());
                }

                if (m_dragTarget)
                    drawList.AddLine({ m_dragCurrent.x, m_dragCurrent.y }, { ghostRect.Left(), ghostRect.Top() + ghostRect.height * 0.5f }, ColorFromBytes(0, 180, 170, 190), 2.0f);

                const std::string label = !slot->ItemName.empty() ? slot->ItemName : (!slot->DragPayload.empty() ? slot->DragPayload : slot->Name);
                if (!label.empty())
                {
                    Vec2 labelSize = MeasureText(label);
                    Vec2 labelPos(pos.x + size + 10.0f, pos.y + (size - labelSize.y) * 0.5f);
                    Vec2 labelEnd(labelPos.x + labelSize.x + 16.0f, labelPos.y + labelSize.y + 10.0f);
                    drawList.AddRectFilled(Vec2{ labelPos.x - 8.0f, labelPos.y - 5.0f }, labelEnd, ColorFromBytes(18, 24, 34, 225), 6.0f);
                    drawList.AddRect(Vec2{ labelPos.x - 8.0f, labelPos.y - 5.0f }, labelEnd, m_dragTarget ? ColorFromBytes(80, 230, 220, 245) : ColorFromBytes(120, 150, 190, 230), 6.0f, ImDrawFlags_None, 1.0f);
                    drawList.AddText(labelPos, ColorFromBytes(240, 245, 255, 255), label.c_str());
                }
                return;
            }

            std::string text = m_dragSource->DragPayload.empty() ? m_dragSource->Name : m_dragSource->DragPayload;
            if (text.empty())
                text = "Dragging";

            Vec2 textSize = MeasureText(text);
            Vec2 pos(m_dragCurrent.x + 18.0f, m_dragCurrent.y + 18.0f);
            Vec2 end(pos.x + std::max(120.0f, textSize.x + 28.0f), pos.y + 42.0f);
            const Color fill = m_dragTarget ? ColorFromBytes(38, 90, 92, 220) : ColorFromBytes(28, 34, 46, 220);
            const Color border = m_dragTarget ? ColorFromBytes(80, 230, 220, 245) : ColorFromBytes(120, 150, 190, 230);
            drawList.AddRectFilled(pos, end, fill, 8.0f);
            drawList.AddRect(pos, end, border, 8.0f, ImDrawFlags_None, 2.0f);
            drawList.AddCircleFilled(Vec2{ pos.x + 17.0f, pos.y + 21.0f }, 6.0f, ColorFromBytes(245, 200, 90, 255), 18);
            drawList.AddText(Vec2(pos.x + 30.0f, pos.y + (42.0f - GetFontSize()) * 0.5f), ColorFromBytes(240, 245, 255, 255), text.c_str());
        }

        void RaiseDrag(EventKind kind, UIElement* target, Vec2 position)
        {
            DragDropEventArgs args;
            args.kind = kind;
            args.route = RoutingStrategy::Bubble;
            args.originalSource = target;
            args.dragSource = m_dragSource;
            args.dropTarget = FindDropTarget(target);
            args.position = position;
            if (m_dragSource)
                args.payload = m_dragSource->DragPayload;
            target->RaiseEvent(args);
        }

        void DispatchFocusedKeyboard(const InputSnapshot& input)
        {
            UIElement* focused = m_focusManager.GetFocusedElement();
            if (!focused)
                return;

            DispatchNavigationActions(focused, input);

            for (size_t i = 1; i < KeyCount; ++i)
            {
                Key key = static_cast<Key>(i);
                if (input.KeyPressed[i])
                    RaiseKey(EventKind::KeyDown, focused, key, true, input);
                if (input.KeyReleased[i])
                    RaiseKey(EventKind::KeyUp, focused, key, false, input);
            }

            for (uint32_t c : input.TextInputCodepoints)
            {
                if (c == 0)
                    continue;
                TextInputEventArgs args;
                args.kind = EventKind::TextInput;
                args.route = RoutingStrategy::Bubble;
                args.originalSource = focused;
                args.codepoint = c;
                focused->RaiseEvent(args);
            }
        }

        void RaiseKey(EventKind kind, UIElement* target, Key key, bool repeat, const InputSnapshot& input)
        {
            KeyEventArgs args;
            args.kind = kind;
            args.route = RoutingStrategy::Bubble;
            args.originalSource = target;
            args.key = key;
            args.repeat = repeat;
            uint32_t modifiers = 0;
            if (input.ShiftDown)
                modifiers |= static_cast<uint32_t>(KeyModifiers::Shift);
            if (input.ControlDown)
                modifiers |= static_cast<uint32_t>(KeyModifiers::Control);
            if (input.AltDown)
                modifiers |= static_cast<uint32_t>(KeyModifiers::Alt);
            args.modifiers = static_cast<KeyModifiers>(modifiers);
            target->RaiseEvent(args);
        }

        void DispatchNavigationActions(UIElement* focused, const InputSnapshot& input)
        {
            if (input.NavigateLeftPressed)
                DispatchDirectionalNavigation(focused, UIAction::NavigateLeft);
            if (input.NavigateRightPressed)
                DispatchDirectionalNavigation(focused, UIAction::NavigateRight);
            if (input.NavigateUpPressed)
                DispatchDirectionalNavigation(focused, UIAction::NavigateUp);
            if (input.NavigateDownPressed)
                DispatchDirectionalNavigation(focused, UIAction::NavigateDown);
            if (input.AcceptPressed)
                focused->OnAction(UIAction::Accept);
            if (input.CancelPressed || input.BackPressed)
                focused->OnAction(UIAction::Cancel);
            if (input.SecondaryPressed)
                focused->OnAction(UIAction::Secondary);
            if (input.DetailsPressed)
                focused->OnAction(UIAction::Details);
            if (input.PageLeftPressed)
                focused->OnAction(UIAction::PageLeft);
            if (input.PageRightPressed)
                focused->OnAction(UIAction::PageRight);
            if (input.MenuPressed)
                focused->OnAction(UIAction::Menu);
        }

        bool IsNavigationPressed(const InputSnapshot& input, Key keyboard, bool gamepadPressed) const
        {
            return IsKeyPressed(input, keyboard) || gamepadPressed;
        }

        bool IsGamepadNavigationPressed(const InputSnapshot& input) const
        {
            return input.GamepadAcceptPressed ||
                input.GamepadNavigateLeftPressed ||
                input.GamepadNavigateRightPressed ||
                input.GamepadNavigateUpPressed ||
                input.GamepadNavigateDownPressed;
        }

        static bool IsKeyPressed(const InputSnapshot& input, Key key)
        {
            const size_t index = static_cast<size_t>(key);
            return index < KeyCount && input.KeyPressed[index];
        }

        bool DispatchDirectionalNavigation(UIElement* focused, UIAction action)
        {
            if (!focused)
                return false;
            if (focused->OnAction(action))
                return true;
            return MoveFocusDirectional(action);
        }

        bool MoveFocusDirectional(UIAction action)
        {
            if (!m_root)
                return false;

            std::vector<UIElement*> tabStops;
            CollectTabStops(m_root.get(), tabStops);
            if (tabStops.empty())
                return false;

            UIElement* current = m_focusManager.GetFocusedElement();
            if (!current)
            {
                m_focusManager.SetFocusedElement(tabStops.front());
                return true;
            }

            const Vec2 from = { current->Bounds.x + current->Bounds.width * 0.5f, current->Bounds.y + current->Bounds.height * 0.5f };
            UIElement* best = nullptr;
            float bestScore = InfiniteSize;

            for (UIElement* candidate : tabStops)
            {
                if (!candidate || candidate == current)
                    continue;
                const Vec2 to = { candidate->Bounds.x + candidate->Bounds.width * 0.5f, candidate->Bounds.y + candidate->Bounds.height * 0.5f };
                const float dx = to.x - from.x;
                const float dy = to.y - from.y;
                float primary = 0.0f;
                float secondary = 0.0f;

                if (action == UIAction::NavigateLeft)
                {
                    if (dx >= -0.1f) continue;
                    primary = -dx;
                    secondary = std::abs(dy);
                }
                else if (action == UIAction::NavigateRight)
                {
                    if (dx <= 0.1f) continue;
                    primary = dx;
                    secondary = std::abs(dy);
                }
                else if (action == UIAction::NavigateUp)
                {
                    if (dy >= -0.1f) continue;
                    primary = -dy;
                    secondary = std::abs(dx);
                }
                else if (action == UIAction::NavigateDown)
                {
                    if (dy <= 0.1f) continue;
                    primary = dy;
                    secondary = std::abs(dx);
                }
                else
                {
                    return false;
                }

                const float score = primary * 1000.0f + secondary;
                if (score < bestScore)
                {
                    bestScore = score;
                    best = candidate;
                }
            }

            if (!best)
                return false;
            m_focusManager.SetFocusedElement(best);
            return true;
        }

        void CollectTabStops(UIElement* element, std::vector<UIElement*>& result)
        {
            if (!element || element->Visibility != FyGUI::Visibility::Visible || !element->IsEnabled)
                return;

            if (element->IsTabStop)
                result.push_back(element);

            element->VisitChildren([&](UIElement* child) { CollectTabStops(child, result); });
        }

        std::shared_ptr<UIElement> m_root;
        FocusManager m_focusManager;
        UIElement* m_hovered = nullptr;
        UIElement* m_pressed = nullptr;
        UIElement* m_dragSource = nullptr;
        UIElement* m_dragTarget = nullptr;
        Vec2 m_dragStart = {};
        Vec2 m_dragCurrent = {};
        InputSnapshot m_lastInput = {};
        bool m_isDragging = false;
        float m_activeUpdateSeconds = 0.35f;
    };
}

#endif

