#pragma once

#include "FriendlyControls.h"
#include "FriendlyXaml.h"

#include "imgui.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <exception>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Gallery
{
    struct FrameDiagnostics
    {
        float InputMs = 0.0f;
        float BackendMs = 0.0f;
        float FontBootMs = 0.0f;
        bool SimdReady = false;
        bool SimdPassed = true;
        std::string BackendName = "Backend";
    };

    class App
    {
    public:
        using TextureResolver = std::function<FyGUI::TextureId(const std::string&)>;

        void SetTextureResolver(TextureResolver resolver) { m_textureResolver = std::move(resolver); }
        void SetIconResolver(TextureResolver resolver) { m_iconResolver = std::move(resolver); }

        void SetInitialToggles(bool focusVisuals, bool dragGhost)
        {
            m_showFocusVisuals = focusVisuals;
            m_showDragGhost = dragGhost;
            FyGUI::SetFocusVisualsEnabled(m_showFocusVisuals);
            m_context.ShowDragGhost = m_showDragGhost;
        }

        void UpdateAndRender(const FyGUI::InputSnapshot& input, FyGUI::Vec2 viewportSize, float deltaTime, const FrameDiagnostics& diagnostics)
        {
            m_width = std::max(1.0f, viewportSize.x);
            m_height = std::max(1.0f, viewportSize.y);

            if (m_pendingExample >= 0)
            {
                m_activeExample = m_pendingExample;
                m_pendingExample = -1;
                m_needsRebuild = true;
            }

            EnsureExamples();
            if (m_needsRebuild || m_builtExample != m_activeExample || !m_context.GetRoot() || m_lastWidth != m_width || m_lastHeight != m_height)
                RebuildExample();

            if (m_validationLabel)
                m_validationLabel->Text = m_sampleValidationMessage;

            const auto updateStart = std::chrono::high_resolution_clock::now();
            m_context.Update(input, viewportSize, std::clamp(deltaTime, 0.0f, 0.1f), true);
            const auto updateEnd = std::chrono::high_resolution_clock::now();

            m_lastInputMs = diagnostics.InputMs;
            m_lastUpdateMs = std::chrono::duration<float, std::milli>(updateEnd - updateStart).count();
            m_lastDrawListMs = 0.0f;
            m_lastBackendMs = diagnostics.BackendMs;
            m_lastDrawCommands = 0;
            m_lastDrawVertices = 0;
            m_lastFriendlyGuiMs = m_lastInputMs + m_lastUpdateMs + m_lastDrawListMs;
        }

        void UpdateAndRenderImGui(const FyGUI::InputSnapshot& input, FyGUI::Vec2 viewportSize, float deltaTime, const FrameDiagnostics& diagnostics, ImDrawList* drawList)
        {
            UpdateAndRender(input, viewportSize, deltaTime, diagnostics);
            const auto drawStart = std::chrono::high_resolution_clock::now();
            if (drawList)
                m_context.Render(*drawList);
            const auto drawEnd = std::chrono::high_resolution_clock::now();
            m_lastDrawListMs = std::chrono::duration<float, std::milli>(drawEnd - drawStart).count();
            m_lastDrawCommands = drawList ? static_cast<uint32_t>(drawList->CmdBuffer.Size) : 0;
            m_lastDrawVertices = drawList ? static_cast<uint32_t>(drawList->VtxBuffer.Size) : 0;
            m_lastFriendlyGuiMs = m_lastInputMs + m_lastUpdateMs + m_lastDrawListMs;
            if (m_smoothedFriendlyGuiMs <= 0.0f)
                m_smoothedFriendlyGuiMs = m_lastFriendlyGuiMs;
            else
                m_smoothedFriendlyGuiMs += (m_lastFriendlyGuiMs - m_smoothedFriendlyGuiMs) * 0.28f;
            UpdatePerfText(diagnostics);
        }

        FyGUI::Context& GetContext() { return m_context; }

    private:
        struct ExampleDefinition
        {
            std::string Name;
            std::string Category;
            std::function<std::shared_ptr<FyGUI::UIElement>()> Build;
        };

        static FyGUI::Color UiColor(float r, float g, float b, float a = 1.0f)
        {
            return { r, g, b, a };
        }

        static std::string FormatMs(float value)
        {
            char buffer[32] {};
            std::snprintf(buffer, sizeof(buffer), "%.3f", value);
            return buffer;
        }

        static std::string NormalizeTextureKey(std::string key)
        {
            for (char& c : key)
            {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (c == '\\')
                    c = '/';
            }
            const size_t slash = key.find_last_of('/');
            if (slash != std::string::npos)
                key = key.substr(slash + 1);
            const size_t dot = key.find_last_of('.');
            if (dot != std::string::npos)
                key = key.substr(0, dot);
            key.erase(std::remove_if(key.begin(), key.end(), [](char c) { return c == ' ' || c == '-' || c == '_'; }), key.end());
            return key;
        }

        FyGUI::UIElement* FindElementByName(FyGUI::UIElement* root, std::string_view name) const
        {
            if (!root)
                return nullptr;
            if (root->Name == name)
                return root;
            if (auto* panel = dynamic_cast<FyGUI::Panel*>(root))
            {
                for (auto& child : panel->Children)
                {
                    if (auto* found = FindElementByName(child.get(), name))
                        return found;
                }
            }
            if (auto* border = dynamic_cast<FyGUI::Border*>(root))
                return FindElementByName(border->Child.get(), name);
            if (auto* example = dynamic_cast<FyGUI::ControlExample*>(root))
            {
                if (auto* found = FindElementByName(example->Example.get(), name))
                    return found;
                if (auto* found = FindElementByName(example->Options.get(), name))
                    return found;
                if (auto* found = FindElementByName(example->Output.get(), name))
                    return found;
            }
            if (auto* tab = dynamic_cast<FyGUI::TabView*>(root))
            {
                for (auto& item : tab->Items)
                {
                    if (auto* found = FindElementByName(item.Content.get(), name))
                        return found;
                }
            }
            return nullptr;
        }

        FyGUI::UIElement* FindElementByName(std::string_view name) const
        {
            return FindElementByName(m_context.GetRoot().get(), name);
        }

        static FyGUI::Color GalleryColorFromName(std::string_view colorName)
        {
            if (FyGUI::EqualsIgnoreCaseAscii(colorName, "Blue"))
                return FyGUI::ColorFromBytes(0, 120, 212);
            if (FyGUI::EqualsIgnoreCaseAscii(colorName, "Green"))
                return FyGUI::ColorFromBytes(16, 124, 16);
            if (FyGUI::EqualsIgnoreCaseAscii(colorName, "Yellow"))
                return FyGUI::ColorFromBytes(255, 255, 0);
            if (FyGUI::EqualsIgnoreCaseAscii(colorName, "Red"))
                return FyGUI::ColorFromBytes(232, 17, 35);
            if (FyGUI::EqualsIgnoreCaseAscii(colorName, "White"))
                return FyGUI::ColorFromBytes(255, 255, 255);
            return FyGUI::ColorFromBytes(0, 120, 212);
        }

        static FyGUI::Color GalleryBorderColorFromName(std::string_view colorName)
        {
            if (FyGUI::EqualsIgnoreCaseAscii(colorName, "Green"))
                return FyGUI::ColorFromBytes(0, 128, 0);
            if (FyGUI::EqualsIgnoreCaseAscii(colorName, "Yellow"))
                return FyGUI::ColorFromBytes(255, 215, 0);
            if (FyGUI::EqualsIgnoreCaseAscii(colorName, "White"))
                return FyGUI::ColorFromBytes(255, 255, 255);
            return GalleryColorFromName(colorName);
        }

        static std::string SelectedTextFrom(FyGUI::UIElement* source)
        {
            if (auto* combo = dynamic_cast<FyGUI::ComboBox*>(source))
                return combo->GetSelectedText();
            if (auto* list = dynamic_cast<FyGUI::ListBox*>(source))
                return list->GetSelectedText();
            if (auto* radios = dynamic_cast<FyGUI::RadioButtons*>(source))
                return radios->SelectedItem;
            return {};
        }

        void SetElementFill(FyGUI::UIElement* target, FyGUI::Color color)
        {
            if (!target)
                return;
            if (auto* rectangle = dynamic_cast<FyGUI::Rectangle*>(target))
            {
                rectangle->Fill = color;
                return;
            }
            target->Background = color;
        }

        void SetElementStroke(FyGUI::UIElement* target, FyGUI::Color color)
        {
            if (!target)
                return;
            if (auto* shape = dynamic_cast<FyGUI::Shape*>(target))
            {
                shape->Stroke = color;
                return;
            }
            target->BorderBrush = color;
        }

        void SetProgressState(FyGUI::UIElement* target, std::string_view state)
        {
            auto* progress = dynamic_cast<FyGUI::ProgressBar*>(target);
            if (!progress)
                return;
            progress->ShowPaused = FyGUI::EqualsIgnoreCaseAscii(state, "Paused");
            progress->ShowError = FyGUI::EqualsIgnoreCaseAscii(state, "Error");
        }

        void SetExpanderDirection(FyGUI::UIElement* target, std::string_view direction)
        {
            auto* expander = dynamic_cast<FyGUI::Expander*>(target);
            if (!expander)
                return;
            if (FyGUI::EqualsIgnoreCaseAscii(direction, "Up"))
                expander->SetExpandDirection(FyGUI::ExpandDirection::Up);
            else if (FyGUI::EqualsIgnoreCaseAscii(direction, "Left"))
                expander->SetExpandDirection(FyGUI::ExpandDirection::Left);
            else if (FyGUI::EqualsIgnoreCaseAscii(direction, "Right"))
                expander->SetExpandDirection(FyGUI::ExpandDirection::Right);
            else
                expander->SetExpandDirection(FyGUI::ExpandDirection::Down);
            expander->SetIsExpanded(true);
            expander->ExpansionProgress = 1.0f;
        }

        void SetElementText(FyGUI::UIElement* target, const std::string& text)
        {
            if (auto* textBlock = dynamic_cast<FyGUI::TextBlock*>(target))
                textBlock->Text = text;
        }

        void SetElementFontSize(FyGUI::UIElement* target, float fontSize)
        {
            if (auto* textBlock = dynamic_cast<FyGUI::TextBlock*>(target))
                textBlock->FontSize = fontSize;
        }

        void TogglePopup(FyGUI::UIElement* target)
        {
            if (auto* popup = dynamic_cast<FyGUI::Popup*>(target))
                popup->SetIsOpen(!popup->IsOpen);
        }

        std::shared_ptr<FyGUI::TextBlock> CreateText(std::string text, FyGUI::Color color = FyGUI::ColorFromBytes(32, 31, 30), float width = -1.0f, float fontSize = 0.0f)
        {
            auto label = std::make_shared<FyGUI::TextBlock>(std::move(text));
            label->Foreground = color;
            label->Width = width;
            label->TextWrapping = FyGUI::TextWrappingMode::Wrap;
            label->FontSize = fontSize;
            return label;
        }

        std::shared_ptr<FyGUI::TextBlock> CreateLabel(std::string text, FyGUI::Color color = UiColor(0.88f, 0.91f, 0.96f, 1.0f), float width = -1.0f)
        {
            auto label = std::make_shared<FyGUI::TextBlock>(std::move(text));
            label->Foreground = color;
            label->Width = width;
            label->TextWrapping = FyGUI::TextWrappingMode::Wrap;
            return label;
        }

        std::shared_ptr<FyGUI::Border> CreatePanel(float width, float height, FyGUI::Color background)
        {
            auto panel = std::make_shared<FyGUI::Border>();
            panel->Width = width;
            panel->Height = height;
            panel->Padding = FyGUI::Thickness(16.0f);
            panel->CornerRadius = 14.0f;
            panel->Background = background;
            panel->BorderBrush = UiColor(0.48f, 0.61f, 0.76f, 0.54f);
            panel->BorderThickness = FyGUI::Thickness(1.0f);
            return panel;
        }

        static FyGUI::Color ReadableSlotStroke(FyGUI::Color fill)
        {
            const float luminance = fill.r * 0.299f + fill.g * 0.587f + fill.b * 0.114f;
            return luminance > 0.55f ? FyGUI::ColorFromBytes(24, 26, 31) : FyGUI::ColorFromBytes(244, 247, 252);
        }

        static void DrawCheckerSlotVisual(ImDrawList& drawList, const FyGUI::Rect& rect, const FyGUI::InventorySlot& slot, FyGUI::VisualState)
        {
            const float minSide = std::min(rect.width, rect.height);
            const float inset = std::max(2.0f, minSide * 0.08f);
            const FyGUI::Rect box = { rect.x + inset, rect.y + inset, std::max(0.0f, rect.width - inset * 2.0f), std::max(0.0f, rect.height - inset * 2.0f) };
            FyGUI::Color tint = slot.HasItemTint ? slot.ItemTint : FyGUI::ColorFromBytes(14, 165, 233);
            const FyGUI::Color stroke = ReadableSlotStroke(tint);
            const float radius = std::max(4.0f, minSide * 0.12f);
            drawList.AddRectFilled({ box.x + 2.0f, box.y + 3.0f }, { box.Right() + 2.0f, box.Bottom() + 3.0f }, FyGUI::ColorFromBytes(0, 0, 0, 58), radius);
            drawList.AddRectFilled({ box.x, box.y }, { box.Right(), box.Bottom() }, FyGUI::Color{ tint.r * 0.42f, tint.g * 0.42f, tint.b * 0.42f, tint.a }.ToU32(1.0f), radius);

            const int cells = 5;
            const float cellW = box.width / static_cast<float>(cells);
            const float cellH = box.height / static_cast<float>(cells);
            const FyGUI::Color bright = { std::min(1.0f, tint.r + 0.26f), std::min(1.0f, tint.g + 0.26f), std::min(1.0f, tint.b + 0.26f), tint.a };
            for (int y = 0; y < cells; ++y)
            {
                for (int x = 0; x < cells; ++x)
                {
                    const FyGUI::Color tile = ((x + y) & 1) ? bright : tint;
                    drawList.AddRectFilled({ box.x + x * cellW, box.y + y * cellH }, { box.x + (x + 1) * cellW + 0.5f, box.y + (y + 1) * cellH + 0.5f }, tile.ToU32(1.0f));
                }
            }
            drawList.AddRect({ box.x, box.y }, { box.Right(), box.Bottom() }, stroke.ToU32(1.0f), radius, ImDrawFlags_None, std::max(1.0f, minSide * 0.035f));
        }

        static void DrawCircleSlotVisual(ImDrawList& drawList, const FyGUI::Rect& rect, const FyGUI::InventorySlot& slot, FyGUI::VisualState)
        {
            const float minSide = std::min(rect.width, rect.height);
            const float inset = std::max(2.0f, minSide * 0.08f);
            const FyGUI::Rect box = { rect.x + inset, rect.y + inset, std::max(0.0f, rect.width - inset * 2.0f), std::max(0.0f, rect.height - inset * 2.0f) };
            FyGUI::Color tint = slot.HasItemTint ? slot.ItemTint : FyGUI::ColorFromBytes(56, 189, 248);
            const FyGUI::Color stroke = ReadableSlotStroke(tint);
            const ImVec2 center = { box.x + box.width * 0.5f, box.y + box.height * 0.5f };
            const float r = std::max(1.0f, std::min(box.width, box.height) * 0.46f);
            const FyGUI::Color inner = { std::min(1.0f, tint.r + 0.18f), std::min(1.0f, tint.g + 0.18f), std::min(1.0f, tint.b + 0.18f), tint.a };
            const FyGUI::Color ring = { tint.r * 0.72f, tint.g * 0.72f, tint.b * 0.72f, tint.a };
            drawList.AddCircleFilled({ center.x + 2.0f, center.y + 3.0f }, r, FyGUI::ColorFromBytes(0, 0, 0, 58), 40);
            drawList.AddCircleFilled(center, r, tint.ToU32(1.0f), 40);
            drawList.AddCircleFilled({ center.x - r * 0.22f, center.y - r * 0.22f }, r * 0.62f, inner.ToU32(0.78f), 32);
            drawList.AddCircle(center, r, stroke.ToU32(1.0f), 40, std::max(1.0f, minSide * 0.035f));
            drawList.AddCircle(center, r * 0.68f, ring.ToU32(0.74f), 36, std::max(1.0f, minSide * 0.025f));
        }

        static void DrawStar(ImDrawList& drawList, ImVec2 center, float outerRadius, float innerRadius, ImU32 color)
        {
            ImVec2 points[10] {};
            for (int i = 0; i < 10; ++i)
            {
                const float radius = (i & 1) ? innerRadius : outerRadius;
                const float angle = -1.57079632679f + static_cast<float>(i) * 0.62831853071f;
                points[i] = { center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius };
            }
            for (int i = 0; i < 10; ++i)
                drawList.AddTriangleFilled(center, points[i], points[(i + 1) % 10], color);
        }

        static void DrawShapeSlotVisual(ImDrawList& drawList, const FyGUI::Rect& rect, const FyGUI::InventorySlot& slot, FyGUI::VisualState, int shapeKind)
        {
            const float minSide = std::min(rect.width, rect.height);
            const float inset = std::max(4.0f, minSide * 0.12f);
            const FyGUI::Rect box = { rect.x + inset, rect.y + inset, std::max(0.0f, rect.width - inset * 2.0f), std::max(0.0f, rect.height - inset * 2.0f) };
            FyGUI::Color tint = slot.HasItemTint ? slot.ItemTint : FyGUI::ColorFromBytes(0, 120, 212);
            const FyGUI::Color bright = { std::min(1.0f, tint.r + 0.22f), std::min(1.0f, tint.g + 0.22f), std::min(1.0f, tint.b + 0.22f), tint.a };
            const FyGUI::Color dark = { tint.r * 0.58f, tint.g * 0.58f, tint.b * 0.58f, tint.a };
            const FyGUI::Color stroke = ReadableSlotStroke(tint);
            const ImVec2 center = { box.x + box.width * 0.5f, box.y + box.height * 0.5f };
            const float radius = std::min(box.width, box.height) * 0.46f;

            drawList.AddCircleFilled({ center.x + 2.0f, center.y + 3.0f }, radius, FyGUI::ColorFromBytes(0, 0, 0, 58), 32);

            switch (shapeKind % 6)
            {
            case 0:
                drawList.AddTriangleFilled({ center.x, box.y }, { box.Right(), box.Bottom() }, { box.x, box.Bottom() }, tint.ToU32(1.0f));
                drawList.AddTriangle({ center.x, box.y }, { box.Right(), box.Bottom() }, { box.x, box.Bottom() }, stroke.ToU32(1.0f), std::max(1.0f, minSide * 0.035f));
                break;
            case 1:
                drawList.PathClear();
                drawList.PathLineTo({ center.x, box.y });
                drawList.PathLineTo({ box.Right(), center.y });
                drawList.PathLineTo({ center.x, box.Bottom() });
                drawList.PathLineTo({ box.x, center.y });
                drawList.PathFillConvex(tint.ToU32(1.0f));
                drawList.PathClear();
                drawList.PathLineTo({ center.x, box.y });
                drawList.PathLineTo({ box.Right(), center.y });
                drawList.PathLineTo({ center.x, box.Bottom() });
                drawList.PathLineTo({ box.x, center.y });
                drawList.PathStroke(stroke.ToU32(1.0f), ImDrawFlags_Closed, std::max(1.0f, minSide * 0.035f));
                break;
            case 2:
                DrawStar(drawList, center, radius, radius * 0.46f, tint.ToU32(1.0f));
                DrawStar(drawList, center, radius * 0.62f, radius * 0.28f, bright.ToU32(0.85f));
                break;
            case 3:
                drawList.AddCircleFilled(center, radius, tint.ToU32(1.0f), 40);
                drawList.AddCircleFilled(center, radius * 0.58f, FyGUI::ColorFromBytes(255, 255, 255, 230), 32);
                drawList.AddCircle(center, radius, stroke.ToU32(1.0f), 40, std::max(1.0f, minSide * 0.035f));
                break;
            case 4:
                drawList.AddRectFilled({ box.x, box.y }, { box.Right(), box.Bottom() }, dark.ToU32(1.0f), 8.0f);
                drawList.AddRectFilled({ box.x + box.width * 0.18f, box.y + box.height * 0.16f }, { box.x + box.width * 0.38f, box.Bottom() - box.height * 0.12f }, bright.ToU32(1.0f), 4.0f);
                drawList.AddRectFilled({ box.x + box.width * 0.46f, box.y + box.height * 0.28f }, { box.x + box.width * 0.66f, box.Bottom() - box.height * 0.12f }, tint.ToU32(1.0f), 4.0f);
                drawList.AddRectFilled({ box.x + box.width * 0.74f, box.y + box.height * 0.08f }, { box.x + box.width * 0.94f, box.Bottom() - box.height * 0.12f }, bright.ToU32(0.8f), 4.0f);
                break;
            default:
                drawList.PathClear();
                for (int i = 0; i < 6; ++i)
                {
                    const float angle = 0.52359877559f + static_cast<float>(i) * 1.0471975512f;
                    drawList.PathLineTo({ center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius });
                }
                drawList.PathFillConvex(tint.ToU32(1.0f));
                drawList.PathClear();
                for (int i = 0; i < 6; ++i)
                {
                    const float angle = 0.52359877559f + static_cast<float>(i) * 1.0471975512f;
                    drawList.PathLineTo({ center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius });
                }
                drawList.PathStroke(stroke.ToU32(1.0f), ImDrawFlags_Closed, std::max(1.0f, minSide * 0.035f));
                break;
            }
        }

        static FyGUI::InventorySlot::ItemVisualRendererCallback MakeShapeSlotRenderer(int shapeKind)
        {
            return [shapeKind](ImDrawList& drawList, const FyGUI::Rect& rect, const FyGUI::InventorySlot& slot, FyGUI::VisualState state)
            {
                DrawShapeSlotVisual(drawList, rect, slot, state, shapeKind);
            };
        }

        static FyGUI::InventoryItem MakeCustomInventoryItem(std::string name, std::string quantity, std::string rarity, FyGUI::Color tint, FyGUI::InventorySlot::ItemVisualRendererCallback renderer)
        {
            FyGUI::InventoryItem item;
            item.Name = std::move(name);
            item.QuantityText = std::move(quantity);
            item.Rarity = std::move(rarity);
            item.Tint = tint;
            item.HasTint = true;
            item.IsEmpty = false;
            item.CustomVisualRenderer = std::move(renderer);
            return item;
        }

        std::shared_ptr<FyGUI::ControlExample> CreateCustomInventoryExample(std::string header, FyGUI::InventorySlot::ItemVisualRendererCallback renderer, bool circleVisuals)
        {
            auto example = std::make_shared<FyGUI::ControlExample>();
            example->HeaderText = std::move(header);
            example->SourceCodeText = "InventoryItem.CustomVisualRenderer draws user content inside each slot.";
            example->MinExampleHeight = 330.0f;

            auto row = std::make_shared<FyGUI::StackPanel>();
            row->Orientation = FyGUI::Orientation::Horizontal;
            row->Spacing = 28.0f;
            row->HorizontalAlignment = FyGUI::HorizontalAlignment::Center;
            row->VerticalAlignment = FyGUI::VerticalAlignment::Center;

            auto grid = std::make_shared<FyGUI::InventoryGrid>();
            grid->Width = 620.0f;
            grid->Height = 205.0f;
            grid->SlotWidth = 70.0f;
            grid->SlotHeight = 70.0f;
            grid->Spacing = 10.0f;
            grid->LineSpacing = 10.0f;
            grid->HorizontalContentAlignment = FyGUI::HorizontalAlignment::Center;
            grid->VerticalContentAlignment = FyGUI::VerticalAlignment::Center;
            grid->AllowSwap = true;
            grid->AllowMove = true;
            grid->Background = FyGUI::ColorFromBytes(255, 255, 255);
            grid->BorderBrush = FyGUI::ColorFromBytes(218, 218, 218);
            grid->BorderThickness = FyGUI::Thickness(1.0f);
            grid->CornerRadius = 6.0f;
            grid->Style.SlotStyle.PART_QuantityText.ForegroundColor = FyGUI::ColorFromBytes(245, 248, 255);

            std::vector<FyGUI::InventoryItem> items;
            if (circleVisuals)
            {
                items.push_back(MakeCustomInventoryItem("Blue Core", "x1", "Rare", FyGUI::ColorFromBytes(56, 189, 248), renderer));
                items.push_back(MakeCustomInventoryItem("Rose Core", "x2", "Epic", FyGUI::ColorFromBytes(251, 113, 133), renderer));
                items.push_back(MakeCustomInventoryItem("Lime Core", "x5", "Uncommon", FyGUI::ColorFromBytes(132, 204, 22), renderer));
                items.push_back(MakeCustomInventoryItem("Violet Core", "x1", "Legendary", FyGUI::ColorFromBytes(139, 92, 246), renderer));
                items.push_back(MakeCustomInventoryItem("Gold Core", "x7", "Common", FyGUI::ColorFromBytes(251, 191, 36), renderer));
                items.push_back(FyGUI::InventoryItem {});
                items.push_back(MakeCustomInventoryItem("Teal Core", "x4", "Rare", FyGUI::ColorFromBytes(20, 184, 166), renderer));
                items.push_back(FyGUI::InventoryItem {});
            }
            else
            {
                items.push_back(MakeCustomInventoryItem("Signal Grid", "x3", "Rare", FyGUI::ColorFromBytes(14, 165, 233), renderer));
                items.push_back(MakeCustomInventoryItem("Heat Grid", "x2", "Epic", FyGUI::ColorFromBytes(239, 68, 68), renderer));
                items.push_back(MakeCustomInventoryItem("Toxic Grid", "x4", "Uncommon", FyGUI::ColorFromBytes(34, 197, 94), renderer));
                items.push_back(MakeCustomInventoryItem("Void Grid", "x1", "Legendary", FyGUI::ColorFromBytes(168, 85, 247), renderer));
                items.push_back(MakeCustomInventoryItem("Amber Grid", "x6", "Common", FyGUI::ColorFromBytes(245, 158, 11), renderer));
                items.push_back(FyGUI::InventoryItem {});
                items.push_back(MakeCustomInventoryItem("Ocean Grid", "x5", "Rare", FyGUI::ColorFromBytes(37, 99, 235), renderer));
                items.push_back(FyGUI::InventoryItem {});
            }
            grid->SetItems(std::move(items));

            auto notes = std::make_shared<FyGUI::StackPanel>();
            notes->Width = 430.0f;
            notes->Spacing = 8.0f;
            notes->AddChild(CreateText(circleVisuals ? "External circular slot content" : "External checker slot content", FyGUI::ColorFromBytes(32, 31, 30), 420.0f, 18.0f));
            notes->AddChild(CreateText("This is not a native InventoryGrid visual. The sample passes a C++ callback into InventoryItem.CustomVisualRenderer, so a game can draw textures, effects, meshes, cooldown masks or any ImDrawList content.", FyGUI::ColorFromBytes(96, 94, 92), 420.0f));
            notes->AddChild(CreateText("Drag a slot over another one. The drag ghost uses the same external renderer, which proves the slot can carry custom visuals through drag/drop.", FyGUI::ColorFromBytes(96, 94, 92), 420.0f));

            row->AddChild(grid);
            row->AddChild(notes);
            example->SetExample(row);
            return example;
        }

        std::shared_ptr<FyGUI::ControlExample> CreateShapeInventoryExample()
        {
            auto example = std::make_shared<FyGUI::ControlExample>();
            example->HeaderText = "Drag/drop with external shape slot content";
            example->SourceCodeText = "Each InventoryItem passes its own shape renderer callback. No native slot visual was added to FriendlyGUI.";
            example->MinExampleHeight = 330.0f;

            auto row = std::make_shared<FyGUI::StackPanel>();
            row->Orientation = FyGUI::Orientation::Horizontal;
            row->Spacing = 28.0f;
            row->HorizontalAlignment = FyGUI::HorizontalAlignment::Center;
            row->VerticalAlignment = FyGUI::VerticalAlignment::Center;

            auto grid = std::make_shared<FyGUI::InventoryGrid>();
            grid->Width = 620.0f;
            grid->Height = 205.0f;
            grid->SlotWidth = 70.0f;
            grid->SlotHeight = 70.0f;
            grid->Spacing = 10.0f;
            grid->LineSpacing = 10.0f;
            grid->HorizontalContentAlignment = FyGUI::HorizontalAlignment::Center;
            grid->VerticalContentAlignment = FyGUI::VerticalAlignment::Center;
            grid->AllowSwap = true;
            grid->AllowMove = true;
            grid->Background = FyGUI::ColorFromBytes(255, 255, 255);
            grid->BorderBrush = FyGUI::ColorFromBytes(218, 218, 218);
            grid->BorderThickness = FyGUI::Thickness(1.0f);
            grid->CornerRadius = 6.0f;
            grid->Style.SlotStyle.PART_QuantityText.ForegroundColor = FyGUI::ColorFromBytes(245, 248, 255);

            std::vector<FyGUI::InventoryItem> items;
            items.push_back(MakeCustomInventoryItem("Triangle Token", "x3", "Rare", FyGUI::ColorFromBytes(0, 120, 212), MakeShapeSlotRenderer(0)));
            items.push_back(MakeCustomInventoryItem("Diamond Token", "x2", "Epic", FyGUI::ColorFromBytes(196, 90, 240), MakeShapeSlotRenderer(1)));
            items.push_back(MakeCustomInventoryItem("Star Token", "x1", "Legendary", FyGUI::ColorFromBytes(255, 185, 0), MakeShapeSlotRenderer(2)));
            items.push_back(MakeCustomInventoryItem("Ring Token", "x4", "Rare", FyGUI::ColorFromBytes(16, 124, 16), MakeShapeSlotRenderer(3)));
            items.push_back(MakeCustomInventoryItem("Bars Token", "x6", "Common", FyGUI::ColorFromBytes(0, 153, 188), MakeShapeSlotRenderer(4)));
            items.push_back(FyGUI::InventoryItem {});
            items.push_back(MakeCustomInventoryItem("Hex Token", "x5", "Epic", FyGUI::ColorFromBytes(232, 17, 35), MakeShapeSlotRenderer(5)));
            items.push_back(FyGUI::InventoryItem {});
            grid->SetItems(std::move(items));

            auto notes = std::make_shared<FyGUI::StackPanel>();
            notes->Width = 430.0f;
            notes->Spacing = 8.0f;
            notes->AddChild(CreateText("External shape slot content", FyGUI::ColorFromBytes(32, 31, 30), 420.0f, 18.0f));
            notes->AddChild(CreateText("This sample is intentionally outside the core. A game can assign a different C++ renderer per item and draw any ImDrawList geometry inside the same InventoryGrid slot.", FyGUI::ColorFromBytes(96, 94, 92), 420.0f));
            notes->AddChild(CreateText("Drag any shape to another slot. The drag ghost keeps the exact same renderer callback, so custom visuals travel with the item.", FyGUI::ColorFromBytes(96, 94, 92), 420.0f));

            row->AddChild(grid);
            row->AddChild(notes);
            example->SetExample(row);
            return example;
        }

        void AppendInventoryGridCustomExamples(const std::shared_ptr<FyGUI::UIElement>& root)
        {
            auto* stack = dynamic_cast<FyGUI::StackPanel*>(root.get());
            if (!stack)
                return;
            auto intro = CreateText("The drag/drop samples below keep every custom visual outside FriendlyGUI core. The slot only provides bounds and events; the sample renderer draws checker textures, procedural shapes or any game content with ImDrawList.", FyGUI::ColorFromBytes(96, 94, 92), 980.0f);
            intro->Margin = FyGUI::Thickness(0.0f, 4.0f, 0.0f, 0.0f);
            stack->AddChild(intro);
            stack->AddChild(CreateCustomInventoryExample("Drag/drop with external checker slot content", DrawCheckerSlotVisual, false));
            stack->AddChild(CreateShapeInventoryExample());
            stack->AddChild(CreateCustomInventoryExample("External circular slot content", DrawCircleSlotVisual, true));
        }

        float StageLeft() const { return 330.0f; }
        float StageWidth() const { return std::max(520.0f, m_width - StageLeft() - 28.0f); }
        float CenterStageX(float width) const { return StageLeft() + std::max(0.0f, (StageWidth() - width) * 0.5f); }
        float CenterWindowY(float height, float minTop = 42.0f) const { return std::max(minTop, (m_height - height) * 0.5f); }

        static void AddToCanvas(const std::shared_ptr<FyGUI::Canvas>& canvas, const std::shared_ptr<FyGUI::UIElement>& child, float left, float top)
        {
            canvas->AddChild(child, left, top);
        }

        FyGUI::TextureId ResolveIconTexture(const std::string& label) const
        {
            return m_iconResolver ? m_iconResolver(label) : 0;
        }

        FyGUI::TextureId ResolveXamlTexture(const std::string& name) const
        {
            return m_textureResolver ? m_textureResolver(name) : 0;
        }

        std::shared_ptr<FyGUI::Border> CreateNavItem(std::string label, bool selected)
        {
            auto item = std::make_shared<FyGUI::Border>();
            item->Width = 224.0f;
            item->Height = 34.0f;
            item->Padding = FyGUI::Thickness(10.0f, 7.0f, 10.0f, 7.0f);
            item->CornerRadius = 4.0f;
            item->Background = selected ? FyGUI::ColorFromBytes(239, 233, 231) : FyGUI::ColorFromBytes(0, 0, 0, 0);
            auto text = CreateText((selected ? "|  " : "   ") + std::move(label), FyGUI::ColorFromBytes(32, 31, 30), 200.0f);
            item->SetChild(text);
            return item;
        }

        std::shared_ptr<FyGUI::Button> CreateGalleryNavButton(const std::string& label, bool selected, int index)
        {
            auto button = std::make_shared<FyGUI::Button>(label);
            button->Width = 214.0f;
            button->Height = 32.0f;
            button->CornerRadius = 4.0f;
            button->Padding = FyGUI::Thickness(10.0f, 6.0f, 10.0f, 6.0f);
            button->Foreground = FyGUI::ColorFromBytes(32, 31, 30);
            button->BackgroundNormal = selected ? FyGUI::ColorFromBytes(239, 233, 231) : FyGUI::ColorFromBytes(0, 0, 0, 0);
            button->BackgroundHover = FyGUI::ColorFromBytes(242, 237, 235);
            button->BackgroundPressed = FyGUI::ColorFromBytes(232, 226, 224);
            button->BorderBrush = FyGUI::ColorFromBytes(0, 0, 0, 0);
            button->HorizontalContentAlignment = FyGUI::HorizontalAlignment::Left;
            button->IconTexture = ResolveIconTexture(label);
            if (!button->IconTexture && selected)
                button->Content = "|  " + label;
            button->OnClick = [this, index]() { m_pendingExample = index; };
            return button;
        }

        std::shared_ptr<FyGUI::Expander> BuildGalleryNavSection(const std::string& category)
        {
            auto items = std::make_shared<FyGUI::StackPanel>();
            items->Spacing = 2.0f;
            bool containsActive = false;
            for (int i = 0; i < static_cast<int>(m_examples.size()); ++i)
            {
                const ExampleDefinition& example = m_examples[static_cast<size_t>(i)];
                if (example.Category != category)
                    continue;
                const bool selected = i == m_activeExample;
                containsActive = containsActive || selected;
                items->AddChild(CreateGalleryNavButton(example.Name, selected, i));
            }

            auto section = std::make_shared<FyGUI::Expander>();
            section->Width = 222.0f;
            section->Header = category;
            const bool defaultExpanded = containsActive || category == "Basic input" || category == "Layout";
            auto remembered = m_navSectionExpanded.find(category);
            bool expanded = remembered != m_navSectionExpanded.end() ? remembered->second : defaultExpanded;
            if (containsActive)
                expanded = true;
            m_navSectionExpanded[category] = expanded;
            section->IsExpanded = expanded;
            section->ExpansionProgress = section->IsExpanded ? 1.0f : 0.0f;
            section->OnExpanded = [this, category]() { m_navSectionExpanded[category] = true; };
            section->OnCollapsed = [this, category]() { m_navSectionExpanded[category] = false; };
            section->HeaderHeight = 34.0f;
            section->Padding = FyGUI::Thickness(6.0f, 4.0f, 6.0f, 6.0f);
            section->CornerRadius = 4.0f;
            section->Foreground = FyGUI::ColorFromBytes(50, 49, 48);
            section->Background = FyGUI::ColorFromBytes(0, 0, 0, 0);
            section->BorderBrush = FyGUI::ColorFromBytes(0, 0, 0, 0);
            section->BorderThickness = FyGUI::Thickness(0.0f);
            section->IsTabStop = false;
            section->SetChild(items);
            return section;
        }

        std::shared_ptr<FyGUI::Canvas> BuildGalleryRoot(std::shared_ptr<FyGUI::UIElement> page)
        {
            auto root = std::make_shared<FyGUI::Canvas>();
            root->Width = m_width;
            root->Height = m_height;
            root->Background = FyGUI::ColorFromBytes(250, 249, 248);

            auto top = std::make_shared<FyGUI::Border>();
            top->Width = root->Width;
            top->Height = 34.0f;
            top->Background = FyGUI::ColorFromBytes(253, 248, 245);
            top->BorderBrush = FyGUI::ColorFromBytes(237, 232, 229);
            top->BorderThickness = FyGUI::Thickness(0.0f, 0.0f, 0.0f, 1.0f);
            AddToCanvas(root, top, 0.0f, 0.0f);

            auto navToggleButton = std::make_shared<FyGUI::Button>("Menu");
            navToggleButton->Width = 58.0f;
            navToggleButton->Height = 26.0f;
            navToggleButton->CornerRadius = 4.0f;
            navToggleButton->Padding = FyGUI::Thickness(8.0f, 4.0f, 8.0f, 4.0f);
            navToggleButton->Foreground = FyGUI::ColorFromBytes(45, 45, 45);
            navToggleButton->BackgroundNormal = FyGUI::ColorFromBytes(0, 0, 0, 0);
            navToggleButton->BackgroundHover = FyGUI::ColorFromBytes(244, 240, 238);
            navToggleButton->BackgroundPressed = FyGUI::ColorFromBytes(234, 229, 227);
            navToggleButton->BorderBrush = FyGUI::ColorFromBytes(0, 0, 0, 0);
            navToggleButton->OnClick = [this]()
            {
                m_navigationPaneOpen = !m_navigationPaneOpen;
                m_needsRebuild = true;
            };
            AddToCanvas(root, navToggleButton, 8.0f, 4.0f);
            AddToCanvas(root, CreateText("FriendlyGUI Gallery", FyGUI::ColorFromBytes(45, 45, 45), 260.0f), 78.0f, 8.0f);

            auto search = std::make_shared<FyGUI::TextBox>();
            search->Width = 300.0f;
            search->Height = 26.0f;
            search->PlaceholderText = "Search controls and samples...";
            search->BackgroundNormal = FyGUI::ColorFromBytes(255, 255, 255);
            search->BackgroundHover = FyGUI::ColorFromBytes(255, 255, 255);
            search->BackgroundPressed = FyGUI::ColorFromBytes(255, 255, 255);
            search->BorderBrush = FyGUI::ColorFromBytes(218, 218, 218);
            search->Foreground = FyGUI::ColorFromBytes(32, 31, 30);
            AddToCanvas(root, search, std::max(360.0f, root->Width * 0.5f - 150.0f), 4.0f);

            auto diagnostics = std::make_shared<FyGUI::StackPanel>();
            diagnostics->Width = 190.0f;
            diagnostics->Spacing = 2.0f;
            diagnostics->HorizontalAlignment = FyGUI::HorizontalAlignment::Left;
            m_perfLabel = CreateText("FriendlyGUI 0.000 ms", FyGUI::ColorFromBytes(96, 94, 92), 190.0f);
            diagnostics->AddChild(m_perfLabel);
            AddToCanvas(root, diagnostics, std::max(520.0f, root->Width - 210.0f), 8.0f);

            const float navWidth = m_navigationPaneOpen ? 238.0f : 0.0f;
            if (m_navigationPaneOpen)
            {
                auto navStack = std::make_shared<FyGUI::StackPanel>();
                navStack->Padding = FyGUI::Thickness(10.0f, 10.0f, 4.0f, 10.0f);
                navStack->Spacing = 4.0f;
                navStack->AddChild(CreateNavItem("Home", false));
                navStack->AddChild(CreateNavItem("Fundamentals", false));
                navStack->AddChild(CreateNavItem("Design", false));
                navStack->AddChild(CreateText("Controls", FyGUI::ColorFromBytes(96, 94, 92), 210.0f));

                const std::vector<std::string> categories = { "Basic input", "Collections", "Dialogs & flyouts", "Design", "Layout", "Text", "Game UI" };
                for (const std::string& category : categories)
                    navStack->AddChild(BuildGalleryNavSection(category));

                auto navScroll = std::make_shared<FyGUI::ScrollViewer>();
                navScroll->Width = navWidth;
                navScroll->Height = std::max(0.0f, root->Height - 34.0f);
                navScroll->Background = FyGUI::ColorFromBytes(250, 244, 241);
                navScroll->BorderBrush = FyGUI::ColorFromBytes(224, 218, 216);
                navScroll->BorderThickness = FyGUI::Thickness(0.0f, 0.0f, 1.0f, 0.0f);
                navScroll->SetChild(navStack);
                AddToCanvas(root, navScroll, 0.0f, 34.0f);
            }

            if (!page)
                page = CreateText("This sample did not produce a UI root.", FyGUI::ColorFromBytes(160, 32, 32), 700.0f);

            auto pageScroll = std::make_shared<FyGUI::ScrollViewer>();
            const float pageLeft = navWidth + 26.0f;
            pageScroll->Width = std::max(320.0f, root->Width - pageLeft - 20.0f);
            pageScroll->Height = std::max(120.0f, root->Height - 58.0f);
            pageScroll->HorizontalScrollBarVisibility = FyGUI::ScrollBarVisibility::Disabled;
            pageScroll->Background = FyGUI::ColorFromBytes(0, 0, 0, 0);
            pageScroll->BorderBrush = FyGUI::ColorFromBytes(0, 0, 0, 0);
            pageScroll->BorderThickness = FyGUI::Thickness(0.0f);
            pageScroll->SetChild(page);
            AddToCanvas(root, pageScroll, pageLeft, 58.0f);

            m_validationLabel = nullptr;
            FyGUI::SetFocusVisualsEnabled(m_showFocusVisuals);
            m_context.ShowDragGhost = m_showDragGhost;
            return root;
        }

        std::string ResolveXamlPath(const std::string& fileName) const
        {
            const std::vector<std::string> prefixes = {
                "../../Docs/Xaml/",
                "Docs/Xaml/",
                "../Docs/Xaml/",
                "../../../Docs/Xaml/",
                "../../../../Docs/Xaml/"
            };

            for (const std::string& prefix : prefixes)
            {
                const std::string path = prefix + fileName;
                std::ifstream probe(path, std::ios::binary);
                if (probe.good())
                    return path;
            }
            return "../../Docs/Xaml/" + fileName;
        }

        std::shared_ptr<FyGUI::UIElement> BuildXamlFileExample(const std::string& fileName, const std::string& title)
        {
            if (auto cached = m_xamlPageCache.find(fileName); cached != m_xamlPageCache.end())
            {
                if (auto* canvas = dynamic_cast<FyGUI::Canvas*>(cached->second.get()))
                {
                    canvas->Width = m_width;
                    canvas->Height = m_height;
                }
                return cached->second;
            }

            FyGUI::XamlActionRegistry actions;
            actions.RegisterAction("ResumeMission", [this]() { m_sampleValidationMessage = "FriendlyXAML action: ResumeMission."; });
            actions.RegisterAction("LoadCheckpoint", [this]() { m_sampleValidationMessage = "FriendlyXAML action: LoadCheckpoint."; });
            actions.RegisterAction("OpenSettings", [this]() { m_sampleValidationMessage = "FriendlyXAML action: OpenSettings."; });
            actions.RegisterAction("ExitToTitle", [this]() { m_sampleValidationMessage = "FriendlyXAML action: ExitToTitle."; });
            actions.RegisterAction("EquipSelectedCell", [this]() { m_sampleValidationMessage = "FriendlyXAML action: EquipSelectedCell."; });
            actions.RegisterAction("ApplySettings", [this]() { m_sampleValidationMessage = "FriendlyXAML action: ApplySettings."; });
            actions.RegisterAction("ToggleFlyout", [this]() { TogglePopup(FindElementByName("SampleFlyout")); });
            actions.RegisterElementAction("ColorComboBox_SelectionChanged", [this](FyGUI::UIElement* source) { SetElementFill(FindElementByName("Control1Output"), GalleryColorFromName(SelectedTextFrom(source))); });
            actions.RegisterElementAction("FontComboBox_SelectionChanged", [this](FyGUI::UIElement* source)
            {
                const std::string selected = SelectedTextFrom(source);
                SetElementText(FindElementByName("Control2Output"), selected.empty() ? "Pick a font from the ComboBox." : "Selected font: " + selected);
            });
            actions.RegisterElementAction("FontSizeComboBox_SelectionChanged", [this](FyGUI::UIElement* source)
            {
                const std::string selected = SelectedTextFrom(source);
                if (!selected.empty())
                {
                    SetElementText(FindElementByName("Control3Output"), "You can set the font size used for this text.");
                    SetElementFontSize(FindElementByName("Control3Output"), std::max(10.0f, static_cast<float>(std::atof(selected.c_str()))));
                }
            });
            actions.RegisterElementAction("ColorListBox_SelectionChanged", [this](FyGUI::UIElement* source) { SetElementFill(FindElementByName("Control1Output"), GalleryColorFromName(SelectedTextFrom(source))); });
            actions.RegisterElementAction("OptionsRadioButtons_SelectionChanged", [this](FyGUI::UIElement* source)
            {
                const std::string selected = SelectedTextFrom(source);
                FyGUI::UIElement* output = FindElementByName("Control1Output");
                if (!output)
                    output = FindElementByName("Control1TextOutput");
                SetElementText(output, selected.empty() ? "Select an option." : selected + " selected.");
            });
            actions.RegisterElementAction("BackgroundColor_SelectionChanged", [this](FyGUI::UIElement* source) { SetElementFill(FindElementByName("ControlOutput"), GalleryColorFromName(SelectedTextFrom(source))); });
            actions.RegisterElementAction("BorderBrush_SelectionChanged", [this](FyGUI::UIElement* source) { SetElementStroke(FindElementByName("ControlOutput"), GalleryBorderColorFromName(SelectedTextFrom(source))); });
            actions.RegisterElementAction("ProgressState_SelectionChanged", [this](FyGUI::UIElement* source) { SetProgressState(FindElementByName("IndeterminateProgressBar"), SelectedTextFrom(source)); });
            actions.RegisterElementAction("ExpanderDirection_SelectionChanged", [this](FyGUI::UIElement* source) { SetExpanderDirection(FindElementByName("DirectionExpander"), SelectedTextFrom(source)); });
            actions.RegisterElementAction("CanvasLeftSlider_ValueChanged", [this](FyGUI::UIElement* source)
            {
                if (auto* slider = dynamic_cast<FyGUI::Slider*>(source))
                    FyGUI::Canvas::SetLeft(FindElementByName("CanvasRedRectangle"), static_cast<float>(slider->Value));
            });
            actions.RegisterElementAction("CanvasTopSlider_ValueChanged", [this](FyGUI::UIElement* source)
            {
                if (auto* slider = dynamic_cast<FyGUI::Slider*>(source))
                    FyGUI::Canvas::SetTop(FindElementByName("CanvasRedRectangle"), static_cast<float>(slider->Value));
            });
            actions.RegisterElementAction("CanvasZIndexSlider_ValueChanged", [this](FyGUI::UIElement* source)
            {
                if (auto* slider = dynamic_cast<FyGUI::Slider*>(source))
                    FyGUI::Canvas::SetZIndex(FindElementByName("CanvasRedRectangle"), static_cast<int32_t>(std::round(slider->Value)));
            });
            actions.RegisterElementAction("Slider1_ValueChanged", [this](FyGUI::UIElement* source)
            {
                if (auto* slider = dynamic_cast<FyGUI::Slider*>(source))
                    SetElementText(FindElementByName("Slider1Output"), "Value: " + std::to_string(static_cast<int>(std::round(slider->Value))));
            });
            actions.RegisterElementAction("Slider2_ValueChanged", [this](FyGUI::UIElement* source)
            {
                if (auto* slider = dynamic_cast<FyGUI::Slider*>(source))
                    SetElementText(FindElementByName("Slider2Output"), "Value: " + std::to_string(static_cast<int>(std::round(slider->Value))));
            });
            actions.RegisterElementAction("Slider3_ValueChanged", [this](FyGUI::UIElement* source)
            {
                if (auto* slider = dynamic_cast<FyGUI::Slider*>(source))
                    SetElementText(FindElementByName("Slider3Output"), "Value: " + std::to_string(static_cast<int>(std::round(slider->Value))));
            });
            actions.RegisterElementAction("RadialMenu_SelectionChanged", [this](FyGUI::UIElement* source)
            {
                if (auto* radial = dynamic_cast<FyGUI::RadialMenu*>(source))
                {
                    const bool valid = radial->SelectedIndex >= 0 && radial->SelectedIndex < static_cast<int32_t>(radial->Items.size());
                    SetElementText(FindElementByName("RadialMenuOutput"), valid ? "Selected: " + radial->Items[static_cast<size_t>(radial->SelectedIndex)].Label : "Selected: none");
                }
            });
            actions.RegisterElementAction("RadialMenu_ItemActivated", [this](FyGUI::UIElement* source)
            {
                if (auto* radial = dynamic_cast<FyGUI::RadialMenu*>(source))
                {
                    const bool valid = radial->SelectedIndex >= 0 && radial->SelectedIndex < static_cast<int32_t>(radial->Items.size());
                    SetElementText(FindElementByName("RadialMenuOutput"), valid ? "Activated: " + radial->Items[static_cast<size_t>(radial->SelectedIndex)].Label : "Activated: none");
                }
            });

            if (!m_xamlStylesLoaded)
            {
                m_xamlStyleResult = FyGUI::LoadXamlStylesFromFile(ResolveXamlPath("Styles.xaml"), m_xamlStyles);
                m_xamlStylesLoaded = true;
            }

            FyGUI::XamlLoadOptions options;
            options.Actions = &actions;
            options.Styles = &m_xamlStyles;
            options.ResolveTexture = [this](const std::string& textureName) { return ResolveXamlTexture(textureName); };

            FyGUI::XamlLoadResult result = FyGUI::LoadXamlFromFile(ResolveXamlPath(fileName), options);
            if (result.Success() && m_xamlStyleResult.Success())
            {
                if (auto* canvas = dynamic_cast<FyGUI::Canvas*>(result.Root.get()))
                {
                    canvas->Width = m_width;
                    canvas->Height = m_height;
                }
                if (fileName == "Gallery/InventoryGridPage.xaml")
                    AppendInventoryGridCustomExamples(result.Root);
                m_xamlPageCache[fileName] = result.Root;
                return result.Root;
            }

            auto fallback = std::make_shared<FyGUI::Canvas>();
            fallback->Width = m_width;
            fallback->Height = m_height;
            auto panel = CreatePanel(680.0f, 210.0f, UiColor(0.02f, 0.04f, 0.07f, 0.94f));
            auto stack = std::make_shared<FyGUI::StackPanel>();
            stack->Spacing = 10.0f;
            stack->AddChild(CreateLabel("FRIENDLYXAML ERROR / " + title, UiColor(1.0f, 0.48f, 0.56f), 620.0f));
            const std::string styleReason = m_xamlStyleResult.Errors.empty() ? std::string{} : "Styles: " + m_xamlStyleResult.Errors.front().Message;
            const std::string loadReason = result.Errors.empty() ? std::string{} : "XAML: " + result.Errors.front().Message;
            stack->AddChild(CreateLabel(styleReason.empty() ? loadReason : styleReason + "\n" + loadReason, UiColor(0.86f, 0.90f, 0.96f), 620.0f));
            panel->SetChild(stack);
            AddToCanvas(fallback, panel, CenterStageX(680.0f), CenterWindowY(210.0f));
            return fallback;
        }

        void EnsureExamples()
        {
            if (!m_examples.empty())
                return;

            m_examples.push_back({ "Button", "Basic input", [this]() { return BuildXamlFileExample("Gallery/ButtonPage.xaml", "Button"); } });
            m_examples.push_back({ "CheckBox", "Basic input", [this]() { return BuildXamlFileExample("Gallery/CheckBoxPage.xaml", "CheckBox"); } });
            m_examples.push_back({ "ComboBox", "Basic input", [this]() { return BuildXamlFileExample("Gallery/ComboBoxPage.xaml", "ComboBox"); } });
            m_examples.push_back({ "RadioButton", "Basic input", [this]() { return BuildXamlFileExample("Gallery/RadioButtonPage.xaml", "RadioButton"); } });
            m_examples.push_back({ "ToggleSwitch", "Basic input", [this]() { return BuildXamlFileExample("Gallery/ToggleSwitchPage.xaml", "ToggleSwitch"); } });
            m_examples.push_back({ "Slider", "Basic input", [this]() { return BuildXamlFileExample("Gallery/SliderPage.xaml", "Slider"); } });
            m_examples.push_back({ "TextBox", "Basic input", [this]() { return BuildXamlFileExample("Gallery/TextBoxPage.xaml", "TextBox"); } });
            m_examples.push_back({ "ProgressBar", "Basic input", [this]() { return BuildXamlFileExample("Gallery/ProgressBarPage.xaml", "ProgressBar"); } });
            m_examples.push_back({ "ListBox", "Collections", [this]() { return BuildXamlFileExample("Gallery/ListBoxPage.xaml", "ListBox"); } });
            m_examples.push_back({ "TabView", "Collections", [this]() { return BuildXamlFileExample("Gallery/TabViewPage.xaml", "TabView"); } });
            m_examples.push_back({ "TreeView", "Collections", [this]() { return BuildXamlFileExample("Gallery/TreeViewPage.xaml", "TreeView"); } });
            m_examples.push_back({ "ContentDialog", "Dialogs & flyouts", [this]() { return BuildXamlFileExample("Gallery/ContentDialogPage.xaml", "ContentDialog"); } });
            m_examples.push_back({ "Flyout", "Dialogs & flyouts", [this]() { return BuildXamlFileExample("Gallery/FlyoutPage.xaml", "Flyout"); } });
            m_examples.push_back({ "InfoBar", "Dialogs & flyouts", [this]() { return BuildXamlFileExample("Gallery/InfoBarPage.xaml", "InfoBar"); } });
            m_examples.push_back({ "TeachingTip", "Dialogs & flyouts", [this]() { return BuildXamlFileExample("Gallery/TeachingTipPage.xaml", "TeachingTip"); } });
            m_examples.push_back({ "NavigationViewItem", "Layout", [this]() { return BuildXamlFileExample("Gallery/NavigationViewPage.xaml", "NavigationViewItem"); } });
            m_examples.push_back({ "CommandBar", "Layout", [this]() { return BuildXamlFileExample("Gallery/CommandBarPage.xaml", "CommandBar"); } });
            m_examples.push_back({ "Color", "Design", [this]() { return BuildXamlFileExample("Gallery/ColorPage.xaml", "Color"); } });
            m_examples.push_back({ "Shapes", "Design", [this]() { return BuildXamlFileExample("Gallery/ShapesPage.xaml", "Shapes"); } });
            m_examples.push_back({ "Border", "Layout", [this]() { return BuildXamlFileExample("Gallery/BorderPage.xaml", "Border"); } });
            m_examples.push_back({ "Canvas", "Layout", [this]() { return BuildXamlFileExample("Gallery/CanvasPage.xaml", "Canvas"); } });
            m_examples.push_back({ "Expander", "Layout", [this]() { return BuildXamlFileExample("Gallery/ExpanderPage.xaml", "Expander"); } });
            m_examples.push_back({ "ScrollViewer", "Layout", [this]() { return BuildXamlFileExample("Gallery/ScrollViewerPage.xaml", "ScrollViewer"); } });
            m_examples.push_back({ "TextBlock", "Text", [this]() { return BuildXamlFileExample("Gallery/TextBlockPage.xaml", "TextBlock"); } });
            m_examples.push_back({ "RadialMenu", "Game UI", [this]() { return BuildXamlFileExample("Gallery/RadialMenuPage.xaml", "RadialMenu"); } });
            m_examples.push_back({ "InventoryGrid", "Game UI", [this]() { return BuildXamlFileExample("Gallery/InventoryGridPage.xaml", "InventoryGrid"); } });
        }

        void RebuildExample()
        {
            EnsureExamples();
            if (m_examples.empty())
                return;

            m_activeExample = std::clamp(m_activeExample, 0, static_cast<int>(m_examples.size()) - 1);
            std::shared_ptr<FyGUI::UIElement> sample = m_examples[static_cast<size_t>(m_activeExample)].Build();
            m_context.SetRoot(BuildGalleryRoot(sample));
            m_builtExample = m_activeExample;
            m_lastWidth = m_width;
            m_lastHeight = m_height;
            m_needsRebuild = false;
        }

        void ValidateSamples()
        {
            EnsureExamples();
            const FyGUI::Vec2 sampleSize { m_width, m_height };
            int validated = 0;

            for (const ExampleDefinition& example : m_examples)
            {
                try
                {
                    std::shared_ptr<FyGUI::UIElement> sample = example.Build();
                    if (!sample)
                    {
                        m_sampleValidationMessage = "Sample validation failed: '" + example.Name + "' returned a null root.";
                        return;
                    }
                    sample->Update(1.0f / 60.0f);
                    sample->Measure(sampleSize);
                    sample->Arrange({ 0.0f, 0.0f, sampleSize.x, sampleSize.y });
                    ++validated;
                }
                catch (const std::exception& ex)
                {
                    m_sampleValidationMessage = "Sample validation failed in '" + example.Name + "': " + ex.what();
                    return;
                }
                catch (...)
                {
                    m_sampleValidationMessage = "Sample validation failed in '" + example.Name + "' with an unknown exception.";
                    return;
                }
            }

            const FyGUI::SmokeTestResult smoke = FyGUI::RunSmokeTest();
            if (!smoke.Passed)
            {
                m_sampleValidationMessage = "Control smoke test failed: " + smoke.Message;
                return;
            }

            const FyGUI::XamlLoadResult xaml = FyGUI::RunXamlSmokeTest();
            if (!xaml.Success())
            {
                const std::string reason = xaml.Errors.empty() ? "unknown XAML loader error" : xaml.Errors.front().Message;
                m_sampleValidationMessage = "FriendlyXAML smoke test failed: " + reason;
                return;
            }

            m_sampleValidationMessage = "Sample validation passed: " + std::to_string(validated) + " examples + " + std::to_string(smoke.ControlsCreated) + " smoke-test controls + FriendlyXAML.";
        }

        void UpdatePerfText(const FrameDiagnostics& diagnostics)
        {
            if (!m_perfLabel)
                return;

            m_perfTextBuffer.clear();
            if (m_perfTextBuffer.capacity() < 256)
                m_perfTextBuffer.reserve(256);
            m_perfTextBuffer += "FriendlyGUI ";
            m_perfTextBuffer += FormatMs(m_lastFriendlyGuiMs);
            m_perfTextBuffer += " ms / avg ";
            m_perfTextBuffer += FormatMs(m_smoothedFriendlyGuiMs);
            m_perfTextBuffer += " ms";
            m_perfLabel->Text = m_perfTextBuffer;
        }

        TextureResolver m_textureResolver;
        TextureResolver m_iconResolver;
        FyGUI::Context m_context;
        std::vector<ExampleDefinition> m_examples;
        std::unordered_map<std::string, std::shared_ptr<FyGUI::UIElement>> m_xamlPageCache;
        std::unordered_map<std::string, bool> m_navSectionExpanded;
        FyGUI::XamlStyleRegistry m_xamlStyles;
        FyGUI::XamlStyleLoadResult m_xamlStyleResult;
        bool m_xamlStylesLoaded = false;
        int m_activeExample = 0;
        int m_builtExample = -1;
        int m_pendingExample = -1;
        bool m_needsRebuild = true;
        bool m_showFocusVisuals = true;
        bool m_showDragGhost = true;
        bool m_navigationPaneOpen = true;
        float m_width = 1280.0f;
        float m_height = 720.0f;
        float m_lastWidth = 0.0f;
        float m_lastHeight = 0.0f;
        float m_lastInputMs = 0.0f;
        float m_lastUpdateMs = 0.0f;
        float m_lastDrawListMs = 0.0f;
        float m_lastBackendMs = 0.0f;
        float m_lastFriendlyGuiMs = 0.0f;
        float m_smoothedFriendlyGuiMs = 0.0f;
        uint32_t m_lastDrawCommands = 0;
        uint32_t m_lastDrawVertices = 0;
        std::string m_sampleValidationMessage = "Samples not validated yet.";
        std::string m_perfTextBuffer;
        std::shared_ptr<FyGUI::TextBlock> m_perfLabel;
        std::shared_ptr<FyGUI::TextBlock> m_validationLabel;
    };
}
