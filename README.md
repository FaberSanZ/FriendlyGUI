# FriendlyGUI


<p align="center">
  <img src="Screenshots/sample.gif" width="600" alt="Demostración centrada">
</p>

**Retained game UI for Dear ImGui.**

FriendlyGUI is a lightweight, game-first retained UI layer built on top of the **Dear ImGui drawing backend** most custom engines already have.

It is **not** a wrapper around ImGui widgets.

FriendlyGUI gives you retained controls, layouts, styles, events, drag/drop, gamepad-friendly navigation, inventory UI, optional XAML-style authoring, Event System: Bubble, Tunnel and Direct Events, and optional advanced input while still using the ImGui renderer your engine already ships with.

> If Dear ImGui works in your engine, FriendlyGUI can work there too.

---


## Why FriendlyGUI?

Dear ImGui is excellent for tools, debug panels, editors, and immediate-mode UI.

Games often need something different:

- retained screens
- reusable controls
- styled menus
- controller/gamepad navigation
- inventory grids
- hotbars
- radial menus
- settings pages
- XAML-like authoring
- UI events
- skins/themes
- designer-friendly layout files

FriendlyGUI keeps the practical power of ImGui's rendering path while adding a retained game UI model on top.

```txt
Dear ImGui:
  backend, draw list, font atlas, renderer integration

FriendlyGUI:
  retained controls, layout, events, styles, XAML, gamepad, game UI behavior
```

---

## Core Philosophy

FriendlyGUI is built around a few strong decisions.

### 1. ImGui-native drawing

FriendlyGUI uses `ImDrawList`, `ImTextureID`, ImGui fonts, clipping, and the existing ImGui backend.

This keeps integration simple.

You do not need to add a new renderer, font system, GPU pipeline, or platform layer.

### 2. Retained game controls

FriendlyGUI owns the control tree.

Controls are persistent objects, with layout, state, styles, events, focus, drag/drop, and game-specific behavior.

It does **not** call `ImGui::Button`, `ImGui::Slider`, `ImGui::Checkbox`, or other ImGui widgets internally.

### 3. Optional systems

Use only what you need.

```txt
Only controls:
  FriendlyControls.h

Controls + XAML:
  FriendlyControls.h
  FriendlyXaml.h

Controls + advanced input:
  FriendlyControls.h
  FriendlyInput.h

Everything:
  FriendlyControls.h
  FriendlyInput.h
  FriendlyXaml.h
```

### 4. Game-first

FriendlyGUI focuses on game UI, not desktop app UI.

That means InventoryGrid, Hotbar, RadialMenu, controller focus, prompt bars, animated states, item rarity visuals, drag/drop, and fast render cost matter more.

### 5. Lightweight and portable

The goal is to stay easy to integrate:

```txt
- header-only core
- MIT license
- Dear ImGui dependency
- no heavy text dependency
- no renderer replacement
- no mandatory XAML
- no mandatory custom input backend
```

---

## Features

### Current Core Features

- Retained UI tree
- ImDrawList rendering
- Mouse input
- Keyboard input
- Gamepad navigation support
- Focus management
- Pointer events
- Keyboard events
- Action events
- Drag/drop support
- Layout system
- Styling system
- Theme presets
- Optional XAML-style loader
- Optional ResourceDictionary styles
- Optional FriendlyInput builder
- Inventory UI controls
- Game-oriented controls
- ImGui input fallback
- Win32/XInput example backend
- Gallery/demo app support

---

## Header Overview

### `FriendlyControls.h`

The main header.

Contains:

- `FyGUI::Context`
- `FyGUI::UIElement`
- base layout and rendering
- events
- focus
- retained controls
- styles
- game controls
- ImGui input helper
- ImDrawList rendering

This is the only required FriendlyGUI header.

### `FriendlyXaml.h`

Optional XAML-style authoring.

Contains:

- `XamlActionRegistry`
- `XamlStyleRegistry`
- `XamlLoadOptions`
- `LoadXamlFromString`
- `LoadXamlFromFile`
- `LoadXamlStylesFromString`
- `LoadXamlStylesFromFile`
- ResourceDictionary-style styles
- StaticResource-style references
- error/warning reporting

FriendlyXAML is inspired by XAML, but it is not a full WPF/WinUI parser.

It is intentionally a small game UI loader for FriendlyGUI.

### `FriendlyInput.h`

Optional input builder for games.

Contains:

- `InputBuilder`
- mouse state
- keyboard state
- text input
- gamepad buttons
- gamepad axes
- gamepad mappings
- navigation repeat
- glyph helpers
- Xbox/PlayStation/Switch/Generic glyph naming

The core still works without this header by using ImGui input.

---

## Requirements

- C++17 or newer
- Dear ImGui
- An existing ImGui backend for your renderer/platform

FriendlyGUI itself does not require DirectX, Vulkan, OpenGL, Metal, SDL, GLFW, Win32, or XInput in the core.

Your engine provides the existing ImGui integration.

---

## Installation

Copy the headers into your project:

```txt
FriendlyControls.h
FriendlyXaml.h      optional
FriendlyInput.h     optional
```

Make sure `imgui.h` is visible to your include path.

Example layout:


Then include:

```cpp
#include "FriendlyControls.h"
```

Optional:

```cpp
#include "FriendlyXaml.h"
#include "FriendlyInput.h"
```

---

## Quick Start: Controls Only

```cpp
#include "FriendlyControls.h"

FyGUI::Context gui;

auto root = std::make_shared<FyGUI::Canvas>();
root->Width = 1280.0f;
root->Height = 720.0f;

auto panel = std::make_shared<FyGUI::Border>();
panel->Width = 420.0f;
panel->Height = 260.0f;
panel->Padding = FyGUI::Thickness(18.0f);
panel->Background = FyGUI::Color{ 0.04f, 0.06f, 0.10f, 0.95f };
panel->BorderBrush = FyGUI::Color{ 0.35f, 0.70f, 1.0f, 0.85f };
panel->BorderThickness = FyGUI::Thickness(1.0f);
panel->CornerRadius = 12.0f;

auto stack = std::make_shared<FyGUI::StackPanel>();
stack->Orientation = FyGUI::Orientation::Vertical;
stack->Spacing = 12.0f;

auto title = std::make_shared<FyGUI::TextBlock>();
title->Text = "FriendlyGUI";
title->FontSize = 24.0f;

auto play = std::make_shared<FyGUI::Button>();
play->Content = "Start Game";
play->Width = 220.0f;
play->Height = 42.0f;
play->OnClick = []()
{
    // Start the game.
};

stack->AddChild(title);
stack->AddChild(play);
panel->SetChild(stack);

root->AddChild(panel, 80.0f, 80.0f);
gui.SetRoot(root);
```

In your frame:

```cpp
ImDrawList* drawList = ImGui::GetBackgroundDrawList();
gui.UpdateAndRenderFromImGui(drawList, { width, height }, deltaTime);
```

---

## Quick Start: FriendlyInput

Use this when you want more control over input or when your game already owns the input layer.

```cpp
#include "FriendlyControls.h"
#include "FriendlyInput.h"

FyGUI::Context gui;
FyGUI::InputBuilder input;

void Frame(float deltaTime)
{
    input.BeginFrame(deltaTime);

    input.SetPointerPosition({ mouseX, mouseY });
    input.SetMouseButton(FyGUI::MouseButton::Left, mouseLeftDown);
    input.AddWheelDelta(mouseWheelDelta);

    input.SetKey(FyGUI::Key::Enter, enterDown);
    input.SetKey(FyGUI::Key::Escape, escapeDown);
    input.AddTextInputUtf8(typedTextUtf8);

    input.SetGamepadConnected(gamepadConnected);
    input.SetGamepadButton(FyGUI::GamepadButton::FaceDown, gamepadA);
    input.SetGamepadButton(FyGUI::GamepadButton::FaceRight, gamepadB);
    input.SetGamepadButton(FyGUI::GamepadButton::DPadDown, dpadDown);
    input.SetGamepadAxis(FyGUI::GamepadAxis::LeftX, leftStickX);
    input.SetGamepadAxis(FyGUI::GamepadAxis::LeftY, leftStickY);

    FyGUI::InputSnapshot snapshot = input.EndFrame();

    gui.Update(snapshot, { width, height }, deltaTime);
    gui.Render(*ImGui::GetBackgroundDrawList());
}
```

---

## Quick Start: XAML-style UI

```cpp
#include "FriendlyControls.h"
#include "FriendlyXaml.h"

FyGUI::Context gui;
FyGUI::XamlActionRegistry actions;
FyGUI::XamlStyleRegistry styles;

actions.RegisterAction("StartGame", []()
{
    // Start the game.
});

actions.RegisterAction("OpenSettings", []()
{
    // Open settings.
});

FyGUI::LoadXamlStylesFromFile("Assets/UI/Styles.xaml", styles);

FyGUI::XamlLoadOptions options;
options.Actions = &actions;
options.Styles = &styles;
options.ResolveTexture = [](const std::string& key) -> FyGUI::TextureId
{
    return ResolveTextureFromEngine(key);
};

FyGUI::XamlLoadResult result = FyGUI::LoadXamlFromFile("Assets/UI/MainMenu.xaml", options);

if (!result.Success())
{
    for (const FyGUI::XamlError& error : result.Errors)
    {
        // Print error.Message, error.Line, error.Column.
    }
}

gui.SetRoot(result.Root);
```

---

## XAML Example

```xml
<Canvas Width="1280" Height="720" Background="#00000000">
    <Border x:Name="MainPanel"
            Canvas.Left="80"
            Canvas.Top="60"
            Width="520"
            Height="420"
            Style="{StaticResource PanelCard}">

        <StackPanel Orientation="Vertical" Spacing="12">
            <TextBlock Text="FRIENDLY GUI"
                       FontSize="24"
                       FontWeight="SemiBold"
                       Foreground="#B0F2FF" />

            <Button Content="Start Game"
                    Width="260"
                    Height="44"
                    Style="{StaticResource PrimaryButton}"
                    OnClick="StartGame" />

            <Button Content="Settings"
                    Width="260"
                    Height="44"
                    Style="{StaticResource SecondaryButton}"
                    OnClick="OpenSettings" />
        </StackPanel>
    </Border>
</Canvas>
```

---

## Styles Example

```xml
<ResourceDictionary>
    <Style x:Key="PanelCard" TargetType="Border">
        <Setter Property="Background" Value="#050A10E8" />
        <Setter Property="BorderBrush" Value="#5CAAE0AA" />
        <Setter Property="BorderThickness" Value="1" />
        <Setter Property="CornerRadius" Value="12" />
        <Setter Property="Padding" Value="18" />
    </Style>

    <Style x:Key="PrimaryButton" TargetType="Button">
        <Setter Property="Background" Value="#182230" />
        <Setter Property="Foreground" Value="#F0F7FF" />
        <Setter Property="BorderBrush" Value="#6BA0CC" />
        <Setter Property="BorderThickness" Value="1" />
        <Setter Property="CornerRadius" Value="8" />
        <Setter Property="Padding" Value="12,6" />
    </Style>

    <Style x:Key="SecondaryButton" TargetType="Button">
        <Setter Property="Background" Value="#101820" />
        <Setter Property="Foreground" Value="#D8F2FF" />
        <Setter Property="BorderBrush" Value="#34526A" />
        <Setter Property="BorderThickness" Value="1" />
        <Setter Property="CornerRadius" Value="8" />
        <Setter Property="Padding" Value="12,6" />
    </Style>
</ResourceDictionary>
```

---

## Controls

FriendlyGUI is focused on game UI controls.

### Layout

- `Canvas`
- `Border`
- `StackPanel`
- `WrapPanel`
- `Grid`
- `ScrollViewer`
- `ScrollBar`
- `Separator`

### Basic Controls

- `TextBlock`
- `Button`
- `Image`
- `Slider`
- `ProgressBar`
- `CheckBox`
- `RadioButton`
- `ToggleSwitch`
- `TextBox`
- `ListBox`
- `ComboBox`
- `TabView`
- `TabViewItem`
- `Expander`

### Information and Dialogs

- `InfoBadge`
- `InfoBar`
- `ContentDialog`
- `ToolTip` / tooltip-style helpers where applicable

### Game Controls

- `InventoryGrid`
- `InventoryItem`
- `Hotbar`
- `RadialMenu`
- `CooldownButton`
- `PromptBar` planned
- `Viewport3D` planned/prototype direction

---

## Game UI Example: InventoryGrid

Inventory UI is one of the main use cases for FriendlyGUI.

```xml
<InventoryGrid x:Name="CargoGrid"
               Width="420"
               SlotWidth="58"
               SlotHeight="58"
               Spacing="8"
               LineSpacing="8"
               AllowSwap="true"
               AllowMove="true"
               OnSelectionChanged="SelectItem"
               OnDrop="MoveItem">

    <InventoryItem Name="Aegis Cell MK III"
                   QuantityText="x3"
                   IsEmpty="false"
                   StackCount="3"
                   Rarity="Rare" />

    <InventoryItem Name="Empty"
                   IsEmpty="true" />
</InventoryGrid>
```

FriendlyGUI is designed so controls like `InventoryGrid` can support:

- selection
- hover/focus
- drag/drop
- item movement
- item swapping
- rarity visuals
- stack counts
- cooldown overlays
- gamepad navigation
- custom events

---

## Input Model

FriendlyGUI can use ImGui input by default:

```cpp
gui.UpdateAndRenderFromImGui(ImGui::GetBackgroundDrawList(), viewportSize, deltaTime);
```

Or you can use `FriendlyInput.h` for game input:

```cpp
FyGUI::InputBuilder builder;
builder.BeginFrame(deltaTime);
builder.SetGamepadButton(FyGUI::GamepadButton::FaceDown, acceptDown);
FyGUI::InputSnapshot input = builder.EndFrame();

gui.Update(input, viewportSize, deltaTime);
gui.Render(*ImGui::GetBackgroundDrawList());
```

### InputBuilder supports

- pointer position
- pointer delta
- mouse buttons
- wheel
- keyboard down/pressed/released
- text input codepoints
- UTF-8 text input helper
- gamepad connection
- gamepad buttons
- gamepad axes
- navigation repeat
- remappable gamepad actions
- glyph helpers for Xbox/PlayStation/Switch/Generic

---

## Gamepad Mapping

FriendlyGUI uses action-style mapping rather than hardcoding one controller layout.

Default mapping direction:

```txt
Accept       -> FaceDown
Cancel       -> FaceRight
Secondary    -> FaceLeft
Details      -> FaceUp
Back         -> Back/View/Create
Menu         -> Start/Menu/Options
Navigate     -> D-pad + left stick
PageLeft     -> LeftShoulder
PageRight    -> RightShoulder
```

This is useful for UI prompts:

```txt
A Select
B Back
X Details
Y Compare
LB Previous
RB Next
```

---

## Events

FriendlyGUI uses event names with an `On` prefix because it fits game code well.

Examples:

```cpp
button->OnClick = []() {};
slider->OnValueChanged = [](double value) {};
list->OnSelectionChanged = [](int32_t index) {};
dialog->OnPrimaryButtonClick = []() {};
```

XAML:

```xml
<Button Content="Start Game" OnClick="StartGame" />
<Slider Value="75" OnValueChanged="SetVolume" />
<ListBox OnSelectionChanged="SelectSaveSlot" />
```

Common events include:

- `OnClick`
- `OnTextChanged`
- `OnValueChanged`
- `OnSelectionChanged`
- `OnChecked`
- `OnUnchecked`
- `OnOpened`
- `OnClosed`
- `OnExpanded`
- `OnCollapsed`
- `OnPointerPressed`
- `OnPointerReleased`
- `OnPointerMoved`
- `OnPointerWheelChanged`
- `OnDrop`
- `OnDragStarted`
- `OnDragCompleted`

---

## Styling

FriendlyGUI supports C++ theme presets and XAML-style resource dictionaries.

### C++ styles

Useful for quick prototypes and examples.

```cpp
button->Style = FyGUI::MakeButtonStyle(FyGUI::ThemePreset::SciFi);
```

### ResourceDictionary styles

Recommended for real game screens.

```xml
<ResourceDictionary>
    <Style x:Key="SciFiButton" TargetType="Button">
        <Setter Property="Background" Value="#182230" />
        <Setter Property="Foreground" Value="#F0F7FF" />
        <Setter Property="BorderBrush" Value="#6BA0CC" />
        <Setter Property="CornerRadius" Value="8" />
    </Style>
</ResourceDictionary>
```

Then:

```xml
<Button Content="Play" Style="{StaticResource SciFiButton}" />
```

---

## Event System: Bubble, Tunnel and Direct Events

FriendlyGUI includes a game-ready event system inspired by modern retained UI frameworks.

Events can be routed through the visual tree using different strategies:

```cpp
enum class RoutingStrategy
{
    Direct,
    Bubble,
    Tunnel
};
```

### Event routing modes

| Mode | Description |
|---|---|
| `Direct` | The event is handled only by the target element. |
| `Bubble` | The event starts at the target and bubbles up through its parents. |
| `Tunnel` | The event starts at the root and travels down toward the target. |

This makes it possible to handle input at different levels of the UI tree.

```cpp
button->OnClick = []()
{
    // Local button action
};

panel->AddHandler(FyGUI::EventKind::PointerPressed, [](FyGUI::EventArgs& args)
{
    // Parent panel can react to child pointer events.
});
```

Useful for:

```txt
Menus
Modal dialogs
Inventory grids
Drag/drop
Tooltips
Global shortcuts
Gamepad navigation
Custom controls
```

---

## Root Controls

FriendlyGUI is built around a retained root element.

A `Context` owns and updates one root control:

```cpp
FyGUI::Context gui;

auto root = std::make_shared<FyGUI::Canvas>();
gui.SetRoot(root);
```

The root can be any `UIElement`, but common root choices are:

```txt
Canvas
StackPanel
Grid
Border
NavigationView
Custom root control
```

Example:

```cpp
auto root = std::make_shared<FyGUI::Canvas>();
root->Width = 1280.0f;
root->Height = 720.0f;

auto button = std::make_shared<FyGUI::Button>();
button->Content = "Play";
button->Width = 240.0f;
button->Height = 48.0f;

root->AddChild(button);
gui.SetRoot(root);
```

Frame flow:

```cpp
gui.Update(input, { width, height }, deltaTime);
gui.Render(*ImGui::GetBackgroundDrawList());
```

---

## Custom Controls

FriendlyGUI is designed so developers can create their own game controls.

Every control derives from `UIElement` or `Control`:

```cpp
class HealthBar : public FyGUI::Control
{
public:
    float Value = 1.0f;

    FyGUI::Vec2 Measure(FyGUI::Vec2 available) override
    {
        return { 240.0f, 24.0f };
    }

    void Arrange(FyGUI::Rect finalRect) override
    {
        VisualBounds = finalRect;
    }

    void Render(ImDrawList& drawList) override
    {
        const auto rect = VisualBounds;

        drawList.AddRectFilled(
            { rect.Left(), rect.Top() },
            { rect.Right(), rect.Bottom() },
            FyGUI::Color{ 0.08f, 0.08f, 0.10f, 1.0f }.ToU32(),
            6.0f
        );

        const float fillWidth = rect.width * std::clamp(Value, 0.0f, 1.0f);

        drawList.AddRectFilled(
            { rect.Left(), rect.Top() },
            { rect.Left() + fillWidth, rect.Bottom() },
            FyGUI::Color{ 0.9f, 0.12f, 0.18f, 1.0f }.ToU32(),
            6.0f
        );
    }
};
```

Custom controls can reuse:

```txt
Layout
Events
Focus
Hover/pressed state
Styles
Pointer input
Gamepad navigation
XAML registration
ImDrawList rendering
```

---

## Visual States

Controls can react to common retained UI states:

```cpp
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
```

These states allow controls and styles to respond consistently to user interaction.

Examples:

```txt
Button hover
Button pressed
Inventory slot selected
ListBox item focused
CheckBox checked
Disabled controls
Gamepad focus ring
```

---

## Game-Ready Focus System

FriendlyGUI supports keyboard and gamepad-friendly focus navigation.

Controls can opt into focus using:

```cpp
button->IsTabStop = true;
```

Focused controls can react visually through styles, focus rings, or custom rendering.

This is especially useful for:

```txt
Console menus
Inventory screens
Settings panels
Radial menus
Hotbars
Dialog boxes
Gamepad navigation
```

---

## Optional Input Layer

FriendlyGUI can use ImGui input by default, or a richer optional input layer.

### ImGui input mode

Best for fast integration:

```cpp
gui.UpdateAndRenderFromImGui(ImGui::GetBackgroundDrawList(), { width, height }, deltaTime);
```

### FriendlyInput mode

Best for games that need deeper control:

```cpp
FyGUI::InputBuilder inputBuilder;

inputBuilder.BeginFrame(deltaTime);
inputBuilder.SetPointerPosition({ mouseX, mouseY });
inputBuilder.SetMouseButton(FyGUI::MouseButton::Left, mouseDown);
inputBuilder.SetGamepadConnected(true);
inputBuilder.SetGamepadButton(FyGUI::GamepadButton::FaceDown, aPressed);

FyGUI::InputSnapshot input = inputBuilder.EndFrame();

gui.Update(input, { width, height }, deltaTime);
gui.Render(*ImGui::GetBackgroundDrawList());
```

FriendlyInput supports:

```txt
Mouse
Keyboard
Text input
Gamepad buttons
Gamepad axes
Navigation repeat
Xbox / PlayStation / Switch glyph mapping
Custom engine input
```

---

## XAML Is Optional

FriendlyGUI can be used fully from C++, or with XAML-style declarative UI.

### C++ only

```cpp
auto button = std::make_shared<FyGUI::Button>();
button->Content = "Start Game";
button->OnClick = []()
{
    StartGame();
};
```

### XAML

```xml
<Button Content="Start Game"
        Width="260"
        Height="48"
        Style="{StaticResource PrimaryButton}"
        OnClick="StartGame" />
```

This allows developers to choose their workflow:

```txt
C++ only for engine/game code
XAML for menus/screens/styles
Both together for production workflows
```

---

## Retained UI Over ImGui Drawing

FriendlyGUI is not a wrapper over ImGui widgets.

It does **not** use:

```txt
ImGui::Button
ImGui::Slider
ImGui::Checkbox
ImGui::Begin / End as UI layout
```

Instead, FriendlyGUI uses its own retained control tree and renders through `ImDrawList`.

```txt
FriendlyGUI:
  Controls
  Layout
  Events
  Styles
  XAML
  Gamepad navigation
  Inventory systems

Dear ImGui:
  ImDrawList
  Font atlas
  Renderer backend
```

That means:

> If your engine already renders ImGui, it can render FriendlyGUI.

---

## Why FriendlyGUI Exists

Dear ImGui is excellent for tools and debug UI.

FriendlyGUI adds a retained, game-focused layer on top:

```txt
Main menus
Pause menus
HUDs
Settings screens
Inventory screens
Hotbars
Radial menus
Dialog boxes
Gamepad-first UI
XAML-driven screens
```

The goal is simple:

> Keep ImGui’s portability and rendering ecosystem, but add a retained UI model designed for games.


---



## Text Rendering

FriendlyGUI intentionally keeps text practical and ImGui-native by default.

The UI should focus on controls, layout, styles, input, and game behavior.

Every engine can decide how far it wants to go with text rendering, localization, shaping, glyph fallback, SDF/MSDF, icon fonts, or custom text systems.

FriendlyGUI's default direction:

```txt
Good default text path:
  ImGui font atlas
  ImGui text measurement
  ImDrawList text rendering

Advanced text:
  engine-specific
  optional future backends
```

No heavy text dependency is required for the core.

---

## Performance

FriendlyGUI is designed to be very cheap because it reuses the ImGui drawing/render path.

Actual timing depends on:

- number of controls
- number of text draws
- number of images
- inventory size
- draw list complexity
- compiler settings
- CPU/GPU
- ImGui backend
- build type

When publishing numbers, always include your test scene, resolution, hardware, backend, and build configuration.

---

## Dear ImGui Integration

FriendlyGUI assumes your engine already does the usual ImGui frame setup.

Typical flow:

```cpp
ImGui_ImplXXXX_NewFrame();
ImGui::NewFrame();

// Your game/editor code.

FyGUI::InputSnapshot input = FyGUI::BuildInputFromImGui();
gui.Update(input, viewportSize, deltaTime);
gui.Render(*ImGui::GetBackgroundDrawList());

ImGui::Render();
ImGui_ImplXXXX_RenderDrawData(ImGui::GetDrawData());
```

Or the shortcut:

```cpp
gui.UpdateAndRenderFromImGui(ImGui::GetBackgroundDrawList(), viewportSize, deltaTime);
```

---

## Public API Naming

FriendlyGUI uses WinUI-inspired names where they make sense, but it is not trying to be WinUI.

Preferred public naming:

```txt
Button.Content
CheckBox.Content
RadioButton.Content
TextBlock.Text
InfoBadge.Value
TabView
TabViewItem
ContentDialog
IsTabStop
BorderBrush
Orientation
Visibility
HorizontalAlignment
VerticalAlignment
```

## XAML Is a Friendly Subset

FriendlyXAML supports a useful subset for game screens:

- element tree
- attributes
- attached properties such as `Canvas.Left`
- `x:Name`
- styles
- setters
- static resources
- action callbacks
- texture/image resolvers
- warnings/errors

It does not aim to support all desktop XAML features.

---

## Custom Controls

The intended direction is to make custom controls easy.

A custom control should be able to:

- inherit from `UIElement` or `Control`
- override measure/arrange/render
- receive input events
- expose properties
- be registered in XAML
- reuse common helpers
- use shared tween/animation state
- use shared focus and drag/drop behavior

Example direction:

```cpp
class SkillTree : public FyGUI::Control
{
public:
    std::vector<SkillNode> Nodes;

protected:
    FyGUI::Vec2 MeasureOverride(FyGUI::Vec2 availableSize) override;
    void ArrangeOverride(FyGUI::Rect finalRect) override;
    void RenderOverride(ImDrawList& drawList) override;
};
```

Future XAML registration direction:

```cpp
FyGUI::XamlControlRegistry registry;
registry.RegisterControl("SkillTree", []()
{
    return std::make_shared<SkillTree>();
});
```

---

## What FriendlyGUI Is Not

FriendlyGUI is not:

- a replacement for Dear ImGui
- a wrapper around ImGui widgets
- a full renderer

FriendlyGUI is:

> retained game UI on top of the ImGui drawing backend.


---


## License

FriendlyGUI is intended to be released under the MIT License.

Dear ImGui is also MIT licensed.

If you bundle Dear ImGui or copy/adapt any Dear ImGui source code directly, keep the appropriate Dear ImGui license notice and attribution.

---

## Credits

FriendlyGUI is built on top of Dear ImGui's proven drawing/rendering ecosystem.

Special thanks to the Dear ImGui project and community.

---

Most custom engines already ship with Dear ImGui for tools and debugging. FriendlyGUI builds on that existing foundation and adds a retained UI model designed for real game screens.

Use it to build menus, HUDs, settings pages, inventory screens, hotbars, radial menus, and controller-friendly interfaces with a small, portable, header-only core.

Start with `FriendlyControls.h`, add `FriendlyXaml.h` when you want declarative authoring, and add `FriendlyInput.h` when you want advanced gamepad input.

No new renderer.  
No heavy UI dependency.  
No expensive middleware.  
Just retained game UI where ImGui already works.


