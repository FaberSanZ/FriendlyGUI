#pragma once

#include "FriendlyControls.h"
#include "FriendlyXaml.h"

#include <array>
#include <fstream>
#include <memory>
#include <string>

namespace HelloWorld
{
    struct Ui
    {
        std::shared_ptr<FyGUI::UIElement> Root;
        FyGUI::TextBlock* HelloText = nullptr;
    };

    inline std::string ResolveXamlPath()
    {
        const std::array<std::string, 5> candidates {
            "Samples/HelloWorld/HelloWorld.xaml",
            "../Samples/HelloWorld/HelloWorld.xaml",
            "../../Samples/HelloWorld/HelloWorld.xaml",
            "../../../Samples/HelloWorld/HelloWorld.xaml",
            "HelloWorld.xaml"
        };
        for (const std::string& path : candidates)
        {
            std::ifstream file(path);
            if (file)
                return path;
        }
        return candidates.front();
    }

    inline FyGUI::UIElement* FindElementByName(FyGUI::UIElement* root, std::string_view name)
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
        return nullptr;
    }

    inline Ui CreateUi()
    {
        Ui ui {};
        FyGUI::XamlLoadResult loaded = FyGUI::LoadXamlFromFile(ResolveXamlPath());
        if (!loaded.Success() || !loaded.Root)
            return ui;

        ui.Root = loaded.Root;
        ui.HelloText = dynamic_cast<FyGUI::TextBlock*>(FindElementByName(ui.Root.get(), "HelloText"));
        auto* canvas = dynamic_cast<FyGUI::Canvas*>(ui.Root.get());
        if (!ui.HelloText || !canvas)
        {
            ui.Root.reset();
            ui.HelloText = nullptr;
            return ui;
        }

        auto button = std::make_shared<FyGUI::Button>("Change text color");
        button->Width = 220.0f;
        button->Height = 44.0f;
        button->Background = FyGUI::ColorFromBytes(0, 120, 212);
        button->Foreground = FyGUI::ColorFromBytes(255, 255, 255);
        button->CornerRadius = 4.0f;
        button->OnClick = [text = ui.HelloText]() {
            static int index = 0;
            const FyGUI::Color colors[] = {
                FyGUI::ColorFromBytes(0, 120, 212),
                FyGUI::ColorFromBytes(16, 124, 16),
                FyGUI::ColorFromBytes(232, 17, 35),
                FyGUI::ColorFromBytes(136, 23, 152)
            };
            index = (index + 1) % 4;
            text->Foreground = colors[index];
        };
        canvas->AddChild(button, 76.0f, 220.0f);
        return ui;
    }
}
