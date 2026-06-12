#include "Rendering.h"

#include <memory>
#include <string>

namespace GamePadSample
{
    inline void SetText(const std::shared_ptr<FyGUI::TextBlock>& text, const std::string& value)
    {
        if (text)
            text->Text = value;
    }

    inline std::shared_ptr<FyGUI::Button> MakeButton(const std::string& content, float width)
    {
        auto button = std::make_shared<FyGUI::Button>(content);
        button->Width = width;
        button->Height = 40.0f;
        return button;
    }

    inline std::shared_ptr<FyGUI::TextBlock> MakeLabel(const std::string& text, float fontSize = 15.0f)
    {
        auto label = std::make_shared<FyGUI::TextBlock>(text);
        label->FontSize = fontSize;
        label->Foreground = FyGUI::ColorFromBytes(50, 49, 48);
        label->TextWrapping = FyGUI::TextWrappingMode::Wrap;
        return label;
    }

    inline std::shared_ptr<FyGUI::Border> MakeCard(float width, float height)
    {
        auto card = std::make_shared<FyGUI::Border>();
        card->Width = width;
        card->Height = height;
        card->Padding = FyGUI::Thickness(18.0f);
        card->CornerRadius = 8.0f;
        card->BorderThickness = FyGUI::Thickness(1.0f);
        card->Background = FyGUI::ColorFromBytes(243, 243, 243);
        card->BorderBrush = FyGUI::ColorFromBytes(218, 218, 218);
        return card;
    }

    inline std::shared_ptr<FyGUI::Border> MakePromptPill(FyGUI::UIAction action, const char* text)
    {
        const FyGUI::GamepadMapping mapping;
        const FyGUI::InputGlyph glyph = FyGUI::GetGamepadGlyph(action, FyGUI::GamepadGlyphStyle::Xbox, mapping);

        auto pill = std::make_shared<FyGUI::Border>();
        pill->Height = 34.0f;
        pill->Padding = FyGUI::Thickness(8.0f, 5.0f, 10.0f, 5.0f);
        pill->CornerRadius = 17.0f;
        pill->Background = FyGUI::ColorFromBytes(255, 255, 255);
        pill->BorderBrush = FyGUI::ColorFromBytes(218, 218, 218);
        pill->BorderThickness = FyGUI::Thickness(1.0f);

        auto row = std::make_shared<FyGUI::StackPanel>();
        row->Orientation = FyGUI::Orientation::Horizontal;
        row->Spacing = 8.0f;

        auto badge = std::make_shared<FyGUI::InfoBadge>(glyph.Text);
        badge->Foreground = FyGUI::ColorFromBytes(255, 255, 255);
        badge->BackgroundNormal = FyGUI::ColorFromBytes(0, 120, 212);
        badge->BackgroundHover = badge->BackgroundNormal;
        badge->BackgroundPressed = badge->BackgroundNormal;
        badge->BorderBrush = FyGUI::ColorFromBytes(0, 95, 184);
        row->AddChild(badge);
        row->AddChild(MakeLabel(text, 14.0f));
        pill->SetChild(row);
        return pill;
    }

    inline std::shared_ptr<FyGUI::UIElement> CreateRoot()
    {
        auto root = std::make_shared<FyGUI::Canvas>();
        root->Background = FyGUI::ColorFromBytes(250, 249, 248);

        auto title = std::make_shared<FyGUI::TextBlock>("GamePad");
        title->FontSize = 28.0f;
        title->Foreground = FyGUI::ColorFromBytes(32, 31, 30);
        root->AddChild(title, 264.0f, 92.0f);

        auto subtitle = MakeLabel("Use an Xbox controller through FriendlyInput/XInput. D-pad or left stick moves focus, A activates, B cancels, X/Y trigger secondary actions, LT/RT cycle focus, LB/RB page controls.", 15.0f);
        subtitle->Width = 860.0f;
        root->AddChild(subtitle, 264.0f, 138.0f);

        auto status = MakeLabel("Waiting for input. Move focus with D-pad or left stick.", 16.0f);
        status->Foreground = FyGUI::ColorFromBytes(0, 95, 184);
        root->AddChild(status, 264.0f, 178.0f);

        auto promptRow = std::make_shared<FyGUI::WrapPanel>();
        promptRow->Spacing = 10.0f;
        promptRow->LineSpacing = 8.0f;
        promptRow->AddChild(MakePromptPill(FyGUI::UIAction::Accept, "Accept"));
        promptRow->AddChild(MakePromptPill(FyGUI::UIAction::Cancel, "Cancel"));
        promptRow->AddChild(MakePromptPill(FyGUI::UIAction::Secondary, "Secondary"));
        promptRow->AddChild(MakePromptPill(FyGUI::UIAction::Details, "Details"));
        promptRow->AddChild(MakePromptPill(FyGUI::UIAction::PageLeft, "Previous page"));
        promptRow->AddChild(MakePromptPill(FyGUI::UIAction::PageRight, "Next page"));
        root->AddChild(promptRow, 264.0f, 214.0f);

        auto actionsCard = MakeCard(900.0f, 142.0f);
        auto actions = std::make_shared<FyGUI::StackPanel>();
        actions->Orientation = FyGUI::Orientation::Vertical;
        actions->Spacing = 12.0f;
        actionsCard->SetChild(actions);

        auto actionsTitle = MakeLabel("Controller actions", 16.0f);
        actionsTitle->Foreground = FyGUI::ColorFromBytes(32, 31, 30);
        actions->AddChild(actionsTitle);

        auto actionRow = std::make_shared<FyGUI::WrapPanel>();
        actionRow->Spacing = 12.0f;
        actionRow->LineSpacing = 8.0f;
        actions->AddChild(actionRow);

        auto accept = MakeButton("A / Accept", 160.0f);
        accept->OnClick = [status]() { SetText(status, "A pressed: primary action accepted."); };
        actionRow->AddChild(accept);

        auto secondary = MakeButton("X / Secondary", 170.0f);
        secondary->OnClick = [status]() { SetText(status, "X pressed: secondary action opened."); };
        actionRow->AddChild(secondary);

        auto details = MakeButton("Y / Details", 150.0f);
        details->OnClick = [status]() { SetText(status, "Y pressed: details panel requested."); };
        actionRow->AddChild(details);

        auto cancel = MakeButton("B / Cancel", 150.0f);
        cancel->OnClick = [status]() { SetText(status, "B pressed: cancel/back action."); };
        actionRow->AddChild(cancel);

        root->AddChild(actionsCard, 264.0f, 264.0f);

        auto controlsCard = MakeCard(430.0f, 330.0f);
        auto controls = std::make_shared<FyGUI::StackPanel>();
        controls->Orientation = FyGUI::Orientation::Vertical;
        controls->Spacing = 14.0f;
        controlsCard->SetChild(controls);

        controls->AddChild(MakeLabel("Tuning controls", 16.0f));

        auto slider = std::make_shared<FyGUI::Slider>();
        slider->Header = "Aim sensitivity";
        slider->Width = 360.0f;
        slider->Minimum = 0.0;
        slider->Maximum = 100.0;
        slider->Value = 62.0;
        slider->AnimatedValue = 62.0;
        slider->StepFrequency = 1.0;
        slider->OnValueChanged = [status](double value) {
            SetText(status, "Aim sensitivity changed to " + std::to_string(static_cast<int>(value)) + "%.");
        };
        controls->AddChild(slider);

        auto progress = std::make_shared<FyGUI::ProgressBar>();
        progress->Width = 360.0f;
        progress->Height = 6.0f;
        progress->Minimum = 0.0;
        progress->Maximum = 100.0;
        progress->Value = 74.0;
        progress->AnimatedValue = 74.0;
        controls->AddChild(progress);

        auto toggle = std::make_shared<FyGUI::ToggleSwitch>();
        toggle->Header = "Controller vibration";
        toggle->OnContent = "Enabled";
        toggle->OffContent = "Off";
        toggle->SetState(FyGUI::CheckedState::Checked);
        toggle->OnCheckedChanged = [status](FyGUI::CheckedState state) {
            SetText(status, state == FyGUI::CheckedState::Checked ? "Vibration enabled." : "Vibration disabled.");
        };
        controls->AddChild(toggle);

        auto check = std::make_shared<FyGUI::CheckBox>("Invert Y axis");
        check->OnCheckedChanged = [status](FyGUI::CheckedState state) {
            SetText(status, state == FyGUI::CheckedState::Checked ? "Invert Y axis enabled." : "Invert Y axis disabled.");
        };
        controls->AddChild(check);

        auto list = std::make_shared<FyGUI::ListBox>();
        list->Width = 360.0f;
        list->VisibleItems = 4;
        list->Items = { "Default profile", "Shooter profile", "Racing profile", "Accessibility profile", "Custom profile" };
        list->SelectedIndex = 0;
        list->OnSelectionChanged = [status, list](int32_t) {
            SetText(status, "Selected profile: " + list->GetSelectedText());
        };
        controls->AddChild(list);

        root->AddChild(controlsCard, 264.0f, 430.0f);

        auto gameCard = MakeCard(430.0f, 330.0f);
        auto game = std::make_shared<FyGUI::StackPanel>();
        game->Orientation = FyGUI::Orientation::Vertical;
        game->Spacing = 12.0f;
        gameCard->SetChild(game);

        game->AddChild(MakeLabel("Game controls", 16.0f));

        auto hotbar = std::make_shared<FyGUI::Hotbar>();
        hotbar->Width = 390.0f;
        hotbar->Height = 86.0f;
        hotbar->SlotWidth = 58.0f;
        hotbar->SlotHeight = 58.0f;
        hotbar->Items = {
            { "Boost", "A", 0, 0.0f, 1.0f, true },
            { "Scan", "X", 0, 0.35f, 1.0f, true },
            { "Map", "Y", 0, 0.0f, 1.0f, true },
            { "Empty", "B", 0, 0.0f, 1.0f, false }
        };
        hotbar->OnUseSlot = [status, hotbar](int32_t index) {
            if (index >= 0 && index < static_cast<int32_t>(hotbar->Items.size()))
                SetText(status, "Hotbar activated: " + hotbar->Items[static_cast<size_t>(index)].Label);
        };
        hotbar->RebuildSlots();
        game->AddChild(hotbar);

        auto radial = std::make_shared<FyGUI::RadialMenu>();
        radial->Width = 230.0f;
        radial->Height = 230.0f;
        radial->Radius = 106.0f;
        radial->InnerRadius = 34.0f;
        radial->Items = {
            { "Map", 0, true },
            { "Quest", 0, true },
            { "Inventory", 0, true },
            { "Skills", 0, true },
            { "Exit", 0, true }
        };
        radial->OnSelectionChanged = [status, radial](int32_t index) {
            if (index >= 0 && index < static_cast<int32_t>(radial->Items.size()))
                SetText(status, "Command wheel selected: " + radial->Items[static_cast<size_t>(index)].Label);
        };
        radial->OnConfirm = [status, radial](int32_t index) {
            if (index >= 0 && index < static_cast<int32_t>(radial->Items.size()))
                SetText(status, "Command wheel activated: " + radial->Items[static_cast<size_t>(index)].Label);
        };
        game->AddChild(radial);

        root->AddChild(gameCard, 734.0f, 430.0f);
        return root;
    }

}

int main()
{
    const float clear[] = { 0.98f, 0.976f, 0.972f, 1.0f };
    return Rendering::RunRoot(L"GamePad", 1280, 720, [] { return GamePadSample::CreateRoot(); }, clear);
}
