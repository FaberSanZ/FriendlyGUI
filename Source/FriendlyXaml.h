#pragma once

#include "FriendlyControls.h"

#include <cctype>
#include <fstream>
#include <initializer_list>
#include <sstream>

namespace FyGUI
{
    struct XamlError
    {
        std::string Message;
        int Line = 1;
        int Column = 1;
        bool IsWarning = false;
    };

    struct XamlLoadResult
    {
        std::shared_ptr<UIElement> Root;
        std::vector<XamlError> Errors;

        bool Success() const
        {
            if (!Root)
                return false;
            for (const auto& error : Errors)
            {
                if (!error.IsWarning)
                    return false;
            }
            return true;
        }
    };

    class XamlActionRegistry
    {
    public:
        void RegisterAction(const std::string& name, std::function<void()> action)
        {
            m_actions[name] = std::move(action);
        }

        void RegisterElementAction(const std::string& name, std::function<void(UIElement*)> action)
        {
            m_elementActions[name] = std::move(action);
        }

        std::function<void()> FindAction(const std::string& name) const
        {
            auto it = m_actions.find(name);
            return it == m_actions.end() ? std::function<void()>{} : it->second;
        }

        std::function<void(UIElement*)> FindElementAction(const std::string& name) const
        {
            auto it = m_elementActions.find(name);
            return it == m_elementActions.end() ? std::function<void(UIElement*)>{} : it->second;
        }

    private:
        std::unordered_map<std::string, std::function<void()>> m_actions;
        std::unordered_map<std::string, std::function<void(UIElement*)>> m_elementActions;
    };

    struct XamlStyleSetter
    {
        std::string Property;
        std::string Value;
    };

    struct XamlStyleDefinition
    {
        std::string Name;
        std::string TargetType;
        std::vector<XamlStyleSetter> Setters;
    };

    class XamlStyleRegistry
    {
    public:
        void RegisterStyle(XamlStyleDefinition style)
        {
            if (!style.Name.empty())
                m_styles[style.Name] = std::move(style);
        }

        const XamlStyleDefinition* FindStyle(const std::string& name) const
        {
            auto it = m_styles.find(name);
            return it == m_styles.end() ? nullptr : &it->second;
        }

    private:
        std::unordered_map<std::string, XamlStyleDefinition> m_styles;
    };

    class XamlControlRegistry
    {
    public:
        void RegisterControl(std::string name, std::function<std::shared_ptr<UIElement>()> factory)
        {
            if (!name.empty())
                m_factories[NormalizeName(name)] = std::move(factory);
        }

        std::shared_ptr<UIElement> Create(std::string_view name) const
        {
            auto it = m_factories.find(NormalizeName(name));
            return it == m_factories.end() || !it->second ? nullptr : it->second();
        }

    private:
        static std::string NormalizeName(std::string_view name)
        {
            std::string value(name);
            const size_t colon = value.find(':');
            if (colon != std::string::npos)
                value = value.substr(colon + 1);
            for (char& c : value)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return value;
        }

        std::unordered_map<std::string, std::function<std::shared_ptr<UIElement>()>> m_factories;
    };

    using XamlPropertySetter = std::function<bool(UIElement&, const std::string&)>;

    class XamlPropertyRegistry
    {
    public:
        void RegisterProperty(std::string controlType, std::string property, XamlPropertySetter setter)
        {
            if (property.empty())
                return;
            m_setters[MakeKey(controlType, property)] = std::move(setter);
        }

        bool TrySet(UIElement& element, std::string_view controlType, std::string_view property, const std::string& value) const
        {
            if (const XamlPropertySetter* setter = FindSetter(controlType, property))
                return (*setter) ? (*setter)(element, value) : false;
            if (const XamlPropertySetter* setter = FindSetter("*", property))
                return (*setter) ? (*setter)(element, value) : false;
            return false;
        }

    private:
        static std::string NormalizeName(std::string_view name)
        {
            std::string value(name);
            const size_t colon = value.find(':');
            if (colon != std::string::npos)
                value = value.substr(colon + 1);
            for (char& c : value)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return value;
        }

        static std::string MakeKey(std::string_view controlType, std::string_view property)
        {
            return NormalizeName(controlType) + "." + NormalizeName(property);
        }

        const XamlPropertySetter* FindSetter(std::string_view controlType, std::string_view property) const
        {
            auto it = m_setters.find(MakeKey(controlType, property));
            return it == m_setters.end() ? nullptr : &it->second;
        }

        std::unordered_map<std::string, XamlPropertySetter> m_setters;
    };

    struct XamlStyleLoadResult
    {
        std::vector<XamlError> Errors;

        bool Success() const
        {
            for (const auto& error : Errors)
            {
                if (!error.IsWarning)
                    return false;
            }
            return true;
        }
    };

    struct XamlLoadOptions
    {
        XamlActionRegistry* Actions = nullptr;
        XamlStyleRegistry* Styles = nullptr;
        XamlControlRegistry* Controls = nullptr;
        XamlPropertyRegistry* Properties = nullptr;
        std::function<ImageSource(const std::string&)> ResolveImage;
        std::function<TextureId(const std::string&)> ResolveTexture;
        bool Strict = false;
    };

    namespace XamlDetail
    {
        struct XmlAttribute
        {
            std::string Name;
            std::string Value;
            int Line = 1;
            int Column = 1;
        };

        struct XmlNode
        {
            std::string Name;
            std::string Text;
            std::vector<XmlAttribute> Attributes;
            std::vector<XmlNode> Children;
            int Line = 1;
            int Column = 1;
        };

        inline std::string Trim(std::string value)
        {
            auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
            while (!value.empty() && isSpace(static_cast<unsigned char>(value.front())))
                value.erase(value.begin());
            while (!value.empty() && isSpace(static_cast<unsigned char>(value.back())))
                value.pop_back();
            return value;
        }

        inline std::string ToLower(std::string value)
        {
            for (char& c : value)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return value;
        }

        inline std::string LocalName(std::string value)
        {
            const size_t colon = value.find(':');
            if (colon != std::string::npos)
                value = value.substr(colon + 1);
            return value;
        }

        inline std::string LowerLocalName(const std::string& value)
        {
            return ToLower(LocalName(value));
        }

        inline bool EqualsIgnoreCase(std::string_view a, std::string_view b)
        {
            if (a.size() != b.size())
                return false;
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
                    return false;
            }
            return true;
        }

        inline std::vector<std::string> SplitList(const std::string& text)
        {
            std::vector<std::string> values;
            std::string current;
            for (char c : text)
            {
                if (c == ',' || std::isspace(static_cast<unsigned char>(c)))
                {
                    if (!current.empty())
                    {
                        values.push_back(current);
                        current.clear();
                    }
                }
                else
                {
                    current.push_back(c);
                }
            }
            if (!current.empty())
                values.push_back(current);
            return values;
        }

        inline void AddIssue(XamlLoadResult& result, std::string message, int line, int column, bool warning)
        {
            XamlError error;
            error.Message = std::move(message);
            error.Line = line;
            error.Column = column;
            error.IsWarning = warning;
            result.Errors.push_back(std::move(error));
        }

        class XmlParser
        {
        public:
            XmlParser(const std::string& text, XamlLoadResult& result)
                : m_text(text), m_result(result)
            {
            }

            bool Parse(XmlNode& root)
            {
                SkipMisc();
                if (End())
                {
                    Error("Empty XAML document.");
                    return false;
                }

                if (!ParseElement(root))
                    return false;

                SkipMisc();
                if (!End())
                    Warning("Unexpected content after the root element.");
                return true;
            }

        private:
            bool End() const { return m_pos >= m_text.size(); }
            char Peek(size_t offset = 0) const { return m_pos + offset < m_text.size() ? m_text[m_pos + offset] : '\0'; }

            bool StartsWith(std::string_view value) const
            {
                return m_text.compare(m_pos, value.size(), value.data(), value.size()) == 0;
            }

            char Advance()
            {
                const char c = End() ? '\0' : m_text[m_pos++];
                if (c == '\n')
                {
                    ++m_line;
                    m_column = 1;
                }
                else
                {
                    ++m_column;
                }
                return c;
            }

            void Error(const std::string& message)
            {
                AddIssue(m_result, message, m_line, m_column, false);
            }

            void Warning(const std::string& message)
            {
                AddIssue(m_result, message, m_line, m_column, true);
            }

            void SkipWhitespace()
            {
                while (!End() && std::isspace(static_cast<unsigned char>(Peek())))
                    Advance();
            }

            void SkipMisc()
            {
                bool advanced = true;
                while (advanced)
                {
                    advanced = false;
                    SkipWhitespace();
                    if (StartsWith("<!--"))
                    {
                        SkipComment();
                        advanced = true;
                    }
                    else if (StartsWith("<?"))
                    {
                        SkipDeclaration();
                        advanced = true;
                    }
                }
            }

            bool SkipUntil(std::string_view marker)
            {
                while (!End())
                {
                    if (StartsWith(marker))
                    {
                        for (size_t i = 0; i < marker.size(); ++i)
                            Advance();
                        return true;
                    }
                    Advance();
                }
                return false;
            }

            void SkipComment()
            {
                Advance(); Advance(); Advance(); Advance();
                if (!SkipUntil("-->"))
                    Error("Unterminated XML comment.");
            }

            void SkipDeclaration()
            {
                Advance(); Advance();
                if (!SkipUntil("?>"))
                    Error("Unterminated XML declaration.");
            }

            std::string ReadName()
            {
                std::string name;
                while (!End())
                {
                    const char c = Peek();
                    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.' || c == ':')
                        name.push_back(Advance());
                    else
                        break;
                }
                return name;
            }

            std::string DecodeEntities(const std::string& value)
            {
                std::string output;
                output.reserve(value.size());
                for (size_t i = 0; i < value.size(); ++i)
                {
                    if (value[i] != '&')
                    {
                        output.push_back(value[i]);
                        continue;
                    }

                    if (value.compare(i, 5, "&amp;") == 0) { output.push_back('&'); i += 4; }
                    else if (value.compare(i, 4, "&lt;") == 0) { output.push_back('<'); i += 3; }
                    else if (value.compare(i, 4, "&gt;") == 0) { output.push_back('>'); i += 3; }
                    else if (value.compare(i, 6, "&quot;") == 0) { output.push_back('"'); i += 5; }
                    else if (value.compare(i, 6, "&apos;") == 0) { output.push_back('\''); i += 5; }
                    else output.push_back('&');
                }
                return output;
            }

            bool ParseAttribute(XmlAttribute& attribute)
            {
                attribute.Line = m_line;
                attribute.Column = m_column;
                attribute.Name = ReadName();
                if (attribute.Name.empty())
                {
                    Error("Expected attribute name.");
                    return false;
                }

                SkipWhitespace();
                if (Advance() != '=')
                {
                    Error("Expected '=' after attribute name.");
                    return false;
                }

                SkipWhitespace();
                const char quote = Advance();
                if (quote != '"' && quote != '\'')
                {
                    Error("Expected quoted attribute value.");
                    return false;
                }

                std::string value;
                while (!End() && Peek() != quote)
                    value.push_back(Advance());
                if (End())
                {
                    Error("Unterminated attribute value.");
                    return false;
                }
                Advance();
                attribute.Value = DecodeEntities(value);
                return true;
            }

            bool ParseElement(XmlNode& node)
            {
                if (Advance() != '<')
                {
                    Error("Expected '<'.");
                    return false;
                }

                node.Line = m_line;
                node.Column = std::max(1, m_column - 1);
                node.Name = ReadName();
                if (node.Name.empty())
                {
                    Error("Expected element name.");
                    return false;
                }

                while (!End())
                {
                    SkipWhitespace();
                    if (StartsWith("/>"))
                    {
                        Advance(); Advance();
                        return true;
                    }
                    if (Peek() == '>')
                    {
                        Advance();
                        return ParseContent(node);
                    }

                    XmlAttribute attribute;
                    if (!ParseAttribute(attribute))
                        return false;
                    node.Attributes.push_back(std::move(attribute));
                }

                Error("Unterminated element.");
                return false;
            }

            bool ParseContent(XmlNode& node)
            {
                while (!End())
                {
                    if (StartsWith("</"))
                    {
                        Advance(); Advance();
                        const std::string closeName = ReadName();
                        SkipWhitespace();
                        if (Advance() != '>')
                        {
                            Error("Expected '>' after closing tag.");
                            return false;
                        }
                        if (!EqualsIgnoreCase(closeName, node.Name))
                        {
                            Error("Mismatched closing tag. Expected </" + node.Name + ">.");
                            return false;
                        }
                        return true;
                    }

                    if (StartsWith("<!--"))
                    {
                        SkipComment();
                        continue;
                    }

                    if (StartsWith("<?"))
                    {
                        SkipDeclaration();
                        continue;
                    }

                    if (StartsWith("<!"))
                    {
                        Warning("Unsupported XML markup skipped.");
                        SkipUntil(">");
                        continue;
                    }

                    if (Peek() == '<')
                    {
                        XmlNode child;
                        if (!ParseElement(child))
                            return false;
                        node.Children.push_back(std::move(child));
                        continue;
                    }

                    std::string text;
                    while (!End() && Peek() != '<')
                        text.push_back(Advance());
                    text = Trim(DecodeEntities(text));
                    if (!text.empty())
                    {
                        if (!node.Text.empty())
                            node.Text += " ";
                        node.Text += text;
                    }
                }

                Error("Missing closing tag for <" + node.Name + ">.");
                return false;
            }

            const std::string& m_text;
            XamlLoadResult& m_result;
            size_t m_pos = 0;
            int m_line = 1;
            int m_column = 1;
        };

        inline const XmlAttribute* FindAttribute(const XmlNode& node, std::string_view name)
        {
            for (const auto& attribute : node.Attributes)
            {
                if (EqualsIgnoreCase(attribute.Name, name))
                    return &attribute;
            }
            return nullptr;
        }

        inline const std::string* AttributeValue(const XmlNode& node, std::string_view name, std::vector<std::string>& consumed)
        {
            const XmlAttribute* attribute = FindAttribute(node, name);
            if (!attribute)
                return nullptr;
            consumed.push_back(ToLower(attribute->Name));
            return &attribute->Value;
        }

        inline const std::string* AttributeValueAny(const XmlNode& node, std::initializer_list<std::string_view> names, std::vector<std::string>& consumed)
        {
            for (std::string_view name : names)
            {
                if (const std::string* value = AttributeValue(node, name, consumed))
                    return value;
            }
            return nullptr;
        }

        inline std::string ParseStaticResourceReference(const std::string& text)
        {
            std::string value = Trim(text);
            const std::string lower = ToLower(value);
            constexpr std::string_view prefix = "{staticresource ";
            if (lower.rfind(std::string(prefix), 0) == 0 && !value.empty() && value.back() == '}')
                return Trim(value.substr(prefix.size(), value.size() - prefix.size() - 1));
            return {};
        }

        inline bool WasConsumed(const std::vector<std::string>& consumed, const std::string& name)
        {
            const std::string lower = ToLower(name);
            return std::find(consumed.begin(), consumed.end(), lower) != consumed.end();
        }

        inline bool ParseFloatValue(const std::string& text, float& value)
        {
            const std::string trimmed = Trim(text);
            if (trimmed.empty())
                return false;
            char* end = nullptr;
            value = std::strtof(trimmed.c_str(), &end);
            return end && *end == '\0';
        }

        inline bool ParseDoubleValue(const std::string& text, double& value)
        {
            float parsed = 0.0f;
            if (!ParseFloatValue(text, parsed))
                return false;
            value = static_cast<double>(parsed);
            return true;
        }

        inline bool ParseIntValue(const std::string& text, int32_t& value)
        {
            const std::string trimmed = Trim(text);
            if (trimmed.empty())
                return false;
            char* end = nullptr;
            const long parsed = std::strtol(trimmed.c_str(), &end, 10);
            if (!end || *end != '\0')
                return false;
            value = static_cast<int32_t>(parsed);
            return true;
        }

        inline bool ParseBoolValue(const std::string& text, bool& value)
        {
            const std::string lower = ToLower(Trim(text));
            if (lower == "true" || lower == "1" || lower == "yes")
            {
                value = true;
                return true;
            }
            if (lower == "false" || lower == "0" || lower == "no")
            {
                value = false;
                return true;
            }
            return false;
        }

        inline bool HexByte(std::string_view text, uint8_t& value)
        {
            if (text.size() != 2)
                return false;
            auto hex = [](char c) -> int
            {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            const int hi = hex(text[0]);
            const int lo = hex(text[1]);
            if (hi < 0 || lo < 0)
                return false;
            value = static_cast<uint8_t>((hi << 4) | lo);
            return true;
        }

        inline bool ParseColorValue(const std::string& text, Color& color)
        {
            const std::string value = Trim(text);
            if (value.empty())
                return false;

            if (value[0] == '#')
            {
                if (value.size() != 7 && value.size() != 9)
                    return false;
                uint8_t r = 0, g = 0, b = 0, a = 255;
                if (!HexByte(std::string_view(value).substr(1, 2), r) ||
                    !HexByte(std::string_view(value).substr(3, 2), g) ||
                    !HexByte(std::string_view(value).substr(5, 2), b))
                    return false;
                if (value.size() == 9 && !HexByte(std::string_view(value).substr(7, 2), a))
                    return false;
                color = { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
                return true;
            }

            const std::string lower = ToLower(value);
            if (lower.rfind("rgba(", 0) == 0 && lower.back() == ')')
            {
                const std::string inner = value.substr(5, value.size() - 6);
                const std::vector<std::string> parts = SplitList(inner);
                if (parts.size() != 4)
                    return false;
                float channels[4] {};
                for (int i = 0; i < 4; ++i)
                {
                    if (!ParseFloatValue(parts[static_cast<size_t>(i)], channels[i]))
                        return false;
                }
                color = {
                    channels[0] > 1.0f ? channels[0] / 255.0f : channels[0],
                    channels[1] > 1.0f ? channels[1] / 255.0f : channels[1],
                    channels[2] > 1.0f ? channels[2] / 255.0f : channels[2],
                    channels[3] > 1.0f ? channels[3] / 255.0f : channels[3]
                };
                return true;
            }

            if (lower == "transparent") { color = { 0.0f, 0.0f, 0.0f, 0.0f }; return true; }
            if (lower == "white") { color = { 1.0f, 1.0f, 1.0f, 1.0f }; return true; }
            if (lower == "black") { color = { 0.0f, 0.0f, 0.0f, 1.0f }; return true; }
            return false;
        }

        inline bool ParseThicknessValue(const std::string& text, Thickness& thickness)
        {
            const std::vector<std::string> parts = SplitList(text);
            float values[4] {};
            if (parts.empty() || (parts.size() != 1 && parts.size() != 2 && parts.size() != 4))
                return false;
            for (size_t i = 0; i < parts.size(); ++i)
            {
                if (!ParseFloatValue(parts[i], values[i]))
                    return false;
            }
            if (parts.size() == 1)
                thickness = Thickness(values[0]);
            else if (parts.size() == 2)
                thickness = Thickness(values[0], values[1], values[0], values[1]);
            else
                thickness = Thickness(values[0], values[1], values[2], values[3]);
            return true;
        }

        inline bool ParseOrientationValue(const std::string& text, Orientation& value)
        {
            const std::string lower = ToLower(Trim(text));
            if (lower == "horizontal") { value = Orientation::Horizontal; return true; }
            if (lower == "vertical") { value = Orientation::Vertical; return true; }
            return false;
        }

        inline bool ParseHorizontalAlignmentValue(const std::string& text, HorizontalAlignment& value)
        {
            const std::string lower = ToLower(Trim(text));
            if (lower == "left") { value = HorizontalAlignment::Left; return true; }
            if (lower == "center" || lower == "centre") { value = HorizontalAlignment::Center; return true; }
            if (lower == "right") { value = HorizontalAlignment::Right; return true; }
            if (lower == "stretch") { value = HorizontalAlignment::Stretch; return true; }
            return false;
        }

        inline bool ParseVerticalAlignmentValue(const std::string& text, VerticalAlignment& value)
        {
            const std::string lower = ToLower(Trim(text));
            if (lower == "top") { value = VerticalAlignment::Top; return true; }
            if (lower == "center" || lower == "centre") { value = VerticalAlignment::Center; return true; }
            if (lower == "bottom") { value = VerticalAlignment::Bottom; return true; }
            if (lower == "stretch") { value = VerticalAlignment::Stretch; return true; }
            return false;
        }

        inline bool ParseVisibilityValue(const std::string& text, Visibility& value)
        {
            const std::string lower = ToLower(Trim(text));
            if (lower == "visible") { value = Visibility::Visible; return true; }
            if (lower == "collapsed") { value = Visibility::Collapsed; return true; }
            return false;
        }

        inline bool ParseFlowDirectionValue(const std::string& text, FlowDirection& value)
        {
            const std::string lower = ToLower(Trim(text));
            if (lower == "lefttoright") { value = FlowDirection::LeftToRight; return true; }
            if (lower == "righttoleft") { value = FlowDirection::RightToLeft; return true; }
            return false;
        }

        inline bool ParseContentDialogButtonValue(const std::string& text, ContentDialogButton& value)
        {
            const std::string lower = ToLower(Trim(text));
            if (lower == "none") { value = ContentDialogButton::None; return true; }
            if (lower == "primary") { value = ContentDialogButton::Primary; return true; }
            if (lower == "secondary") { value = ContentDialogButton::Secondary; return true; }
            if (lower == "close") { value = ContentDialogButton::Close; return true; }
            return false;
        }

        inline bool ParseScrollBarVisibilityValue(const std::string& text, ScrollBarVisibility& value)
        {
            const std::string lower = ToLower(Trim(text));
            if (lower == "disabled") { value = ScrollBarVisibility::Disabled; return true; }
            if (lower == "auto") { value = ScrollBarVisibility::Auto; return true; }
            if (lower == "hidden") { value = ScrollBarVisibility::Hidden; return true; }
            if (lower == "visible") { value = ScrollBarVisibility::Visible; return true; }
            return false;
        }

        inline bool ParseThemePresetValue(const std::string& text, ThemePreset& value)
        {
            const std::string lower = ToLower(Trim(text));
            if (lower == "scifi" || lower == "sci-fi") { value = ThemePreset::SciFi; return true; }
            if (lower == "fantasy") { value = ThemePreset::Fantasy; return true; }
            if (lower == "tactical") { value = ThemePreset::Tactical; return true; }
            if (lower == "minimal") { value = ThemePreset::Minimal; return true; }
            return false;
        }

        inline void ApplyPresetStyle(UIElement& element, ThemePreset preset)
        {
            if (auto* combo = dynamic_cast<ComboBox*>(&element))
            {
                combo->Style = MakeListBoxStyle(preset);
                combo->ComboStyle = MakeComboBoxStyle(preset);
                return;
            }
            if (auto* list = dynamic_cast<ListBox*>(&element))
            {
                list->Style = MakeListBoxStyle(preset);
                return;
            }
            if (auto* slider = dynamic_cast<Slider*>(&element))
            {
                slider->Style = MakeSliderStyle(preset);
                return;
            }
            if (auto* progress = dynamic_cast<ProgressBar*>(&element))
            {
                progress->Style = MakeProgressBarStyle(preset);
                return;
            }
            if (auto* textBox = dynamic_cast<TextBox*>(&element))
            {
                textBox->Style = MakeTextBoxStyle(preset);
                return;
            }
            if (auto* tab = dynamic_cast<TabView*>(&element))
            {
                tab->Style = MakeTabViewStyle(preset);
                return;
            }
            if (auto* expander = dynamic_cast<Expander*>(&element))
            {
                expander->ExpanderParts = MakeExpanderStyle(preset);
                return;
            }
            if (auto* toggle = dynamic_cast<ToggleSwitch*>(&element))
            {
                toggle->ToggleStyle = MakeToggleSwitchStyle(preset);
                return;
            }
            if (auto* infoBar = dynamic_cast<InfoBar*>(&element))
            {
                infoBar->Style = MakeInfoBarStyle(preset);
                return;
            }
            if (auto* dialog = dynamic_cast<ContentDialog*>(&element))
            {
                dialog->Style = MakeContentDialogStyle(preset);
                return;
            }
            if (auto* commandBar = dynamic_cast<CommandBar*>(&element))
            {
                commandBar->Style = MakeCommandBarStyle(preset);
                return;
            }
            if (auto* appBarButton = dynamic_cast<AppBarButton*>(&element))
            {
                appBarButton->AppBarStyle = MakeAppBarButtonStyle(preset);
                return;
            }
            if (auto* separator = dynamic_cast<Separator*>(&element))
            {
                separator->Style = MakeSeparatorStyle(preset);
                return;
            }
            if (auto* inventory = dynamic_cast<InventoryGrid*>(&element))
            {
                const StylePalette palette = MakeStylePalette(preset);
                inventory->Style.PART_Background = SolidPart(Color{ palette.Surface.r, palette.Surface.g, palette.Surface.b, 0.66f }, palette.Border, 1.0f, palette.Radius);
                inventory->Style.PART_ItemsPresenter = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f });
                inventory->Style.SlotStyle = MakeInventorySlotStyle(preset);
                return;
            }
            if (auto* checkBox = dynamic_cast<CheckBox*>(&element))
            {
                const StylePalette palette = MakeStylePalette(preset);
                checkBox->CheckStyle.PART_Box = SolidPart(palette.SurfaceAlt, palette.Border, 1.0f, 4.0f);
                checkBox->CheckStyle.PART_CheckMark = SolidPart(palette.Accent, Color{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, 3.0f);
                checkBox->CheckStyle.PART_Text.ForegroundColor = palette.Text;
                checkBox->CheckStyle.PART_FocusVisual = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, palette.AccentAlt, 2.0f, palette.Radius);
                return;
            }
            if (auto* button = dynamic_cast<Button*>(&element))
            {
                button->Style = MakeButtonStyle(preset);
                return;
            }
            if (auto* border = dynamic_cast<Border*>(&element))
            {
                const StylePalette palette = MakeStylePalette(preset);
                border->Style.PART_Background = SolidPart(Color{ palette.Surface.r, palette.Surface.g, palette.Surface.b, 0.92f }, palette.Border, 1.0f, palette.Radius);
                border->Style.PART_Border = SolidPart(Color{ 0.0f, 0.0f, 0.0f, 0.0f }, palette.Border, 1.0f, palette.Radius);
            }
        }

        inline bool ResolveTextureAttribute(const XmlNode& node, std::string_view name, TextureId& target, std::vector<std::string>& consumed, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            if (const std::string* value = AttributeValue(node, name, consumed))
            {
                if (options.ResolveTexture)
                    target = options.ResolveTexture(*value);
                else
                    AddIssue(result, "Texture property '" + std::string(name) + "' requires XamlLoadOptions::ResolveTexture.", node.Line, node.Column, !options.Strict);
                return true;
            }
            return false;
        }

        inline void ApplyImagePartAttribute(const XmlNode& node, std::string_view name, ControlPartStyle& part, TextureId ControlPartStyle::*member, std::vector<std::string>& consumed, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            TextureId texture = 0;
            if (ResolveTextureAttribute(node, name, texture, consumed, result, options) && texture)
            {
                part.*member = texture;
                part.UseImage = true;
            }
        }

        inline void AttributeWarning(XamlLoadResult& result, const XmlNode& node, const std::string& message, bool strict)
        {
            AddIssue(result, message, node.Line, node.Column, !strict);
        }

        inline bool TryFloatAttribute(const XmlNode& node, std::string_view name, float& target, std::vector<std::string>& consumed, XamlLoadResult& result, bool strict)
        {
            if (const std::string* value = AttributeValue(node, name, consumed))
            {
                if (!ParseFloatValue(*value, target))
                    AttributeWarning(result, node, "Invalid float value for '" + std::string(name) + "'.", strict);
                return true;
            }
            return false;
        }

        inline bool TryDoubleAttribute(const XmlNode& node, std::string_view name, double& target, std::vector<std::string>& consumed, XamlLoadResult& result, bool strict)
        {
            if (const std::string* value = AttributeValue(node, name, consumed))
            {
                if (!ParseDoubleValue(*value, target))
                    AttributeWarning(result, node, "Invalid numeric value for '" + std::string(name) + "'.", strict);
                return true;
            }
            return false;
        }

        inline bool TryIntAttribute(const XmlNode& node, std::string_view name, int32_t& target, std::vector<std::string>& consumed, XamlLoadResult& result, bool strict)
        {
            if (const std::string* value = AttributeValue(node, name, consumed))
            {
                if (!ParseIntValue(*value, target))
                    AttributeWarning(result, node, "Invalid integer value for '" + std::string(name) + "'.", strict);
                return true;
            }
            return false;
        }

        inline bool TryBoolAttribute(const XmlNode& node, std::string_view name, bool& target, std::vector<std::string>& consumed, XamlLoadResult& result, bool strict)
        {
            if (const std::string* value = AttributeValue(node, name, consumed))
            {
                if (!ParseBoolValue(*value, target))
                    AttributeWarning(result, node, "Invalid bool value for '" + std::string(name) + "'.", strict);
                return true;
            }
            return false;
        }

        inline bool TryColorAttribute(const XmlNode& node, std::string_view name, Color& target, std::vector<std::string>& consumed, XamlLoadResult& result, bool strict)
        {
            if (const std::string* value = AttributeValue(node, name, consumed))
            {
                if (!ParseColorValue(*value, target))
                    AttributeWarning(result, node, "Invalid color value for '" + std::string(name) + "'.", strict);
                return true;
            }
            return false;
        }

        inline bool TryThicknessAttribute(const XmlNode& node, std::string_view name, Thickness& target, std::vector<std::string>& consumed, XamlLoadResult& result, bool strict)
        {
            if (const std::string* value = AttributeValue(node, name, consumed))
            {
                if (!ParseThicknessValue(*value, target))
                    AttributeWarning(result, node, "Invalid thickness value for '" + std::string(name) + "'.", strict);
                return true;
            }
            return false;
        }

        inline void ApplyCommonProperties(UIElement& element, const XmlNode& node, std::vector<std::string>& consumed, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            if (const std::string* value = AttributeValue(node, "x:Name", consumed))
                element.Name = *value;
            TryFloatAttribute(node, "Width", element.Width, consumed, result, options.Strict);
            TryFloatAttribute(node, "Height", element.Height, consumed, result, options.Strict);
            TryFloatAttribute(node, "MinWidth", element.MinWidth, consumed, result, options.Strict);
            TryFloatAttribute(node, "MinHeight", element.MinHeight, consumed, result, options.Strict);
            TryFloatAttribute(node, "MaxWidth", element.MaxWidth, consumed, result, options.Strict);
            TryFloatAttribute(node, "MaxHeight", element.MaxHeight, consumed, result, options.Strict);
            TryThicknessAttribute(node, "Margin", element.Margin, consumed, result, options.Strict);
            TryThicknessAttribute(node, "Padding", element.Padding, consumed, result, options.Strict);
            TryColorAttribute(node, "Foreground", element.Foreground, consumed, result, options.Strict);
            TryColorAttribute(node, "BorderBrush", element.BorderBrush, consumed, result, options.Strict);
            TryThicknessAttribute(node, "BorderThickness", element.BorderThickness, consumed, result, options.Strict);
            TryFloatAttribute(node, "CornerRadius", element.CornerRadius, consumed, result, options.Strict);
            TryFloatAttribute(node, "Opacity", element.Opacity, consumed, result, options.Strict);
            TryBoolAttribute(node, "IsEnabled", element.IsEnabled, consumed, result, options.Strict);
            TryBoolAttribute(node, "IsHitTestVisible", element.IsHitTestVisible, consumed, result, options.Strict);
            TryBoolAttribute(node, "IsTabStop", element.IsTabStop, consumed, result, options.Strict);
            TryBoolAttribute(node, "ClipToBounds", element.ClipToBounds, consumed, result, options.Strict);
            TryBoolAttribute(node, "UseLayoutRounding", element.UseLayoutRounding, consumed, result, options.Strict);
            AttributeValue(node, "Tag", consumed);
            AttributeValue(node, "DataContext", consumed);
            if (const std::string* value = AttributeValue(node, "ToolTipService.ToolTip", consumed))
                element.ToolTip = *value;

            if (const std::string* value = AttributeValue(node, "Background", consumed))
            {
                Color background;
                if (ParseColorValue(*value, background))
                {
                    element.Background = background;
                    if (auto* control = dynamic_cast<Control*>(&element))
                    {
                        control->BackgroundNormal = background;
                        control->BackgroundHover = BrightenColor(background, 0.10f);
                        control->BackgroundPressed = BrightenColor(background, -0.10f);
                    }
                }
                else
                {
                    AttributeWarning(result, node, "Invalid color value for 'Background'.", options.Strict);
                }
            }

            if (const std::string* value = AttributeValue(node, "HorizontalAlignment", consumed))
            {
                HorizontalAlignment alignment;
                if (ParseHorizontalAlignmentValue(*value, alignment))
                    element.HorizontalAlignment = alignment;
                else
                    AttributeWarning(result, node, "Invalid HorizontalAlignment value.", options.Strict);
            }

            if (const std::string* value = AttributeValue(node, "VerticalAlignment", consumed))
            {
                VerticalAlignment alignment;
                if (ParseVerticalAlignmentValue(*value, alignment))
                    element.VerticalAlignment = alignment;
                else
                    AttributeWarning(result, node, "Invalid VerticalAlignment value.", options.Strict);
            }

            if (const std::string* value = AttributeValue(node, "Visibility", consumed))
            {
                Visibility visibility;
                if (ParseVisibilityValue(*value, visibility))
                    element.Visibility = visibility;
                else
                    AttributeWarning(result, node, "Invalid Visibility value.", options.Strict);
            }

            if (const std::string* value = AttributeValue(node, "FlowDirection", consumed))
            {
                FlowDirection flowDirection;
                if (ParseFlowDirectionValue(*value, flowDirection))
                    element.FlowDirection = flowDirection;
                else
                    AttributeWarning(result, node, "Invalid FlowDirection value.", options.Strict);
            }

            if (auto* control = dynamic_cast<Control*>(&element))
                control->BorderBrush = element.BorderBrush;

            AttributeValue(node, "Canvas.Left", consumed);
            AttributeValue(node, "Canvas.Top", consumed);
            AttributeValue(node, "Canvas.Right", consumed);
            AttributeValue(node, "Canvas.Bottom", consumed);
            AttributeValue(node, "Canvas.ZIndex", consumed);
        }

        inline std::shared_ptr<UIElement> CreateElement(const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            const std::string name = LowerLocalName(node.Name);
            if (name == "canvas") return std::make_shared<Canvas>();
            if (name == "border") return std::make_shared<Border>();
            if (name == "stackpanel") return std::make_shared<StackPanel>();
            if (name == "wrappanel") return std::make_shared<WrapPanel>();
            if (name == "controlexample") return std::make_shared<ControlExample>();
            if (name == "textblock") return std::make_shared<TextBlock>();
            if (name == "button") return std::make_shared<Button>();
            if (name == "image") return std::make_shared<Image>();
            if (name == "slider") return std::make_shared<Slider>();
            if (name == "progressbar") return std::make_shared<ProgressBar>();
            if (name == "checkbox") return std::make_shared<CheckBox>();
            if (name == "radiobutton") return std::make_shared<RadioButton>();
            if (name == "radiobuttons") return std::make_shared<RadioButtons>();
            if (name == "toggleswitch") return std::make_shared<ToggleSwitch>();
            if (name == "textbox") return std::make_shared<TextBox>();
            if (name == "listbox") return std::make_shared<ListBox>();
            if (name == "combobox") return std::make_shared<ComboBox>();
            if (name == "scrollviewer") return std::make_shared<ScrollViewer>();
            if (name == "tabview") return std::make_shared<TabView>();
            if (name == "treeview") return std::make_shared<TreeView>();
            if (name == "inventorygrid") return std::make_shared<InventoryGrid>();
            if (name == "separator") return std::make_shared<Separator>();
            if (name == "infobadge") return std::make_shared<InfoBadge>();
            if (name == "commandbar") return std::make_shared<CommandBar>();
            if (name == "appbarbutton") return std::make_shared<AppBarButton>();
            if (name == "appbarseparator") return std::make_shared<AppBarSeparator>();
            if (name == "menu") return std::make_shared<Menu>();
            if (name == "menuitem") return std::make_shared<MenuItem>();
            if (name == "flyout" || name == "popup") return std::make_shared<Popup>();
            if (name == "contentdialog") return std::make_shared<ContentDialog>();
            if (name == "expander") return std::make_shared<Expander>();
            if (name == "infobar") return std::make_shared<InfoBar>();
            if (name == "teachingtip") return std::make_shared<TeachingTip>();
            if (name == "navigationviewitem") return std::make_shared<NavigationViewItem>();
            if (name == "radialmenu") return std::make_shared<RadialMenu>();
            if (name == "commandwheel") return std::make_shared<CommandWheel>();
            if (name == "rectangle") return std::make_shared<Rectangle>();
            if (name == "ellipse") return std::make_shared<Ellipse>();
            if (name == "line") return std::make_shared<Line>();

            if (options.Controls)
            {
                std::shared_ptr<UIElement> custom = options.Controls->Create(node.Name);
                if (custom)
                    return custom;
            }

            AttributeWarning(result, node, "Unknown control '" + node.Name + "'. Register it with XamlControlRegistry.", options.Strict);
            return nullptr;
        }

        inline void ApplyRegisteredProperties(UIElement& element, const XmlNode& node, std::vector<std::string>& consumed, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            if (!options.Properties)
                return;

            for (const XmlAttribute& attribute : node.Attributes)
            {
                if (WasConsumed(consumed, attribute.Name))
                    continue;
                if (options.Properties->TrySet(element, node.Name, attribute.Name, attribute.Value))
                    consumed.push_back(attribute.Name);
            }
        }

        inline void ApplySpecificProperties(UIElement& element, const XmlNode& node, std::vector<std::string>& consumed, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            if (auto* border = dynamic_cast<Border*>(&element))
            {
                ApplyImagePartAttribute(node, "BackgroundImage", border->Style.PART_Background, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "HoverImage", border->Style.PART_Background, &ControlPartStyle::HoverImage, consumed, result, options);
                ApplyImagePartAttribute(node, "PressedImage", border->Style.PART_Background, &ControlPartStyle::PressedImage, consumed, result, options);
            }

            if (auto* controlExample = dynamic_cast<ControlExample*>(&element))
            {
                if (const std::string* value = AttributeValueAny(node, { "HeaderText", "Header" }, consumed))
                    controlExample->HeaderText = *value;
                if (const std::string* value = AttributeValue(node, "SourceCodeText", consumed))
                    controlExample->SourceCodeText = *value;
                TryBoolAttribute(node, "ShowSourceCode", controlExample->ShowSourceCode, consumed, result, options.Strict);
                TryFloatAttribute(node, "MinExampleHeight", controlExample->MinExampleHeight, consumed, result, options.Strict);
            }

            if (auto* shape = dynamic_cast<Shape*>(&element))
            {
                TryColorAttribute(node, "Fill", shape->Fill, consumed, result, options.Strict);
                TryColorAttribute(node, "Stroke", shape->Stroke, consumed, result, options.Strict);
                TryFloatAttribute(node, "StrokeThickness", shape->StrokeThickness, consumed, result, options.Strict);
            }

            if (auto* rectangle = dynamic_cast<Rectangle*>(&element))
            {
                TryFloatAttribute(node, "RadiusX", rectangle->RadiusX, consumed, result, options.Strict);
                TryFloatAttribute(node, "RadiusY", rectangle->RadiusY, consumed, result, options.Strict);
            }

            if (auto* line = dynamic_cast<Line*>(&element))
            {
                TryFloatAttribute(node, "X1", line->X1, consumed, result, options.Strict);
                TryFloatAttribute(node, "Y1", line->Y1, consumed, result, options.Strict);
                TryFloatAttribute(node, "X2", line->X2, consumed, result, options.Strict);
                TryFloatAttribute(node, "Y2", line->Y2, consumed, result, options.Strict);
            }

            if (auto* text = dynamic_cast<TextBlock*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Text", consumed))
                    text->Text = *value;
                else if (!node.Text.empty())
                    text->Text = node.Text;
                if (const std::string* value = AttributeValue(node, "TextWrapping", consumed))
                {
                    const std::string wrapping = ToLower(Trim(*value));
                    if (wrapping == "nowrap")
                        text->TextWrapping = TextWrappingMode::NoWrap;
                    else if (wrapping == "wrap" || wrapping == "wrapwholewords")
                        text->TextWrapping = wrapping == "wrapwholewords" ? TextWrappingMode::WrapWholeWords : TextWrappingMode::Wrap;
                    else
                        AttributeWarning(result, node, "Invalid TextWrapping value.", options.Strict);
                }
                TryFloatAttribute(node, "FontSize", text->FontSize, consumed, result, options.Strict);
                AttributeValue(node, "FontFamily", consumed);
                AttributeValue(node, "FontWeight", consumed);
                AttributeValue(node, "FontStyle", consumed);
                AttributeValue(node, "MaxLines", consumed);
                AttributeValue(node, "TextAlignment", consumed);
                AttributeValue(node, "TextTrimming", consumed);
                AttributeValue(node, "IsTextSelectionEnabled", consumed);
            }

            if (auto* button = dynamic_cast<Button*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Content", consumed))
                    button->Content = *value;
                else if (!node.Text.empty())
                    button->Content = node.Text;

                if (const std::string* value = AttributeValue(node, "HorizontalContentAlignment", consumed))
                {
                    HorizontalAlignment alignment;
                    if (ParseHorizontalAlignmentValue(*value, alignment))
                        button->HorizontalContentAlignment = alignment;
                    else
                        AttributeWarning(result, node, "Invalid Button HorizontalContentAlignment value.", options.Strict);
                }
                if (const std::string* value = AttributeValue(node, "VerticalContentAlignment", consumed))
                {
                    VerticalAlignment alignment;
                    if (ParseVerticalAlignmentValue(*value, alignment))
                        button->VerticalContentAlignment = alignment;
                    else
                        AttributeWarning(result, node, "Invalid Button VerticalContentAlignment value.", options.Strict);
                }

                if (const std::string* value = AttributeValue(node, "OnClick", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        button->OnClick = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }

                if (const std::string* value = AttributeValueAny(node, { "Icon", "IconSource" }, consumed))
                {
                    if (options.ResolveTexture)
                        button->IconTexture = options.ResolveTexture(*value);
                    else
                        AttributeWarning(result, node, "Icon requires XamlLoadOptions::ResolveTexture.", options.Strict);
                }
                ApplyImagePartAttribute(node, "BackgroundImage", button->Style.PART_Background, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "HoverImage", button->Style.PART_Background, &ControlPartStyle::HoverImage, consumed, result, options);
                ApplyImagePartAttribute(node, "PressedImage", button->Style.PART_Background, &ControlPartStyle::PressedImage, consumed, result, options);
                ApplyImagePartAttribute(node, "DisabledImage", button->Style.PART_Background, &ControlPartStyle::DisabledImage, consumed, result, options);
                ApplyImagePartAttribute(node, "FocusedImage", button->Style.PART_Background, &ControlPartStyle::FocusedImage, consumed, result, options);
            }

            if (auto* image = dynamic_cast<Image*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Source", consumed))
                {
                    if (options.ResolveImage)
                        image->Source = options.ResolveImage(*value);
                    else if (options.ResolveTexture)
                        image->Source.texture = options.ResolveTexture(*value);
                    else
                        AttributeWarning(result, node, "Image Source requires ResolveImage or ResolveTexture.", options.Strict);
                }
                TryColorAttribute(node, "Tint", image->Tint, consumed, result, options.Strict);
            }

            if (auto* stack = dynamic_cast<StackPanel*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Orientation", consumed))
                {
                    Orientation orientation;
                    if (ParseOrientationValue(*value, orientation))
                        stack->Orientation = orientation;
                    else
                        AttributeWarning(result, node, "Invalid Orientation value.", options.Strict);
                }
                TryFloatAttribute(node, "Spacing", stack->Spacing, consumed, result, options.Strict);
            }

            if (auto* wrap = dynamic_cast<WrapPanel*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Orientation", consumed))
                {
                    Orientation orientation;
                    if (ParseOrientationValue(*value, orientation))
                        wrap->Orientation = orientation;
                    else
                        AttributeWarning(result, node, "Invalid Orientation value.", options.Strict);
                }
                TryFloatAttribute(node, "Spacing", wrap->Spacing, consumed, result, options.Strict);
                TryFloatAttribute(node, "LineSpacing", wrap->LineSpacing, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "HorizontalContentAlignment", consumed))
                {
                    HorizontalAlignment alignment;
                    if (ParseHorizontalAlignmentValue(*value, alignment))
                        wrap->HorizontalContentAlignment = alignment;
                    else
                        AttributeWarning(result, node, "Invalid WrapPanel HorizontalContentAlignment value.", options.Strict);
                }
                if (const std::string* value = AttributeValue(node, "VerticalContentAlignment", consumed))
                {
                    VerticalAlignment alignment;
                    if (ParseVerticalAlignmentValue(*value, alignment))
                        wrap->VerticalContentAlignment = alignment;
                    else
                        AttributeWarning(result, node, "Invalid WrapPanel VerticalContentAlignment value.", options.Strict);
                }
            }

            if (auto* slider = dynamic_cast<Slider*>(&element))
            {
                TryDoubleAttribute(node, "Minimum", slider->Minimum, consumed, result, options.Strict);
                TryDoubleAttribute(node, "Maximum", slider->Maximum, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "Orientation", consumed))
                {
                    Orientation orientation;
                    if (ParseOrientationValue(*value, orientation))
                        slider->Orientation = orientation;
                    else
                        AttributeWarning(result, node, "Invalid Slider Orientation.", options.Strict);
                }
                if (const std::string* value = AttributeValue(node, "Value", consumed))
                {
                    double parsed = slider->Value;
                    if (ParseDoubleValue(*value, parsed))
                    {
                        slider->SetValue(parsed);
                        slider->AnimatedValue = slider->Value;
                    }
                    else
                    {
                        AttributeWarning(result, node, "Invalid Slider Value.", options.Strict);
                    }
                }
                if (const std::string* value = AttributeValue(node, "OnValueChanged", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        slider->OnValueChanged = [elementAction = std::move(elementAction), slider](double) { elementAction(slider); };
                    else if (action)
                        slider->OnValueChanged = [action = std::move(action)](double) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnDragStarted", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        slider->OnDragStarted = [elementAction = std::move(elementAction), slider] { elementAction(slider); };
                    else if (action)
                        slider->OnDragStarted = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnDragCompleted", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        slider->OnDragCompleted = [elementAction = std::move(elementAction), slider] { elementAction(slider); };
                    else if (action)
                        slider->OnDragCompleted = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                TryDoubleAttribute(node, "StepFrequency", slider->StepFrequency, consumed, result, options.Strict);
                TryDoubleAttribute(node, "SmallChange", slider->SmallChange, consumed, result, options.Strict);
                TryDoubleAttribute(node, "TickFrequency", slider->TickFrequency, consumed, result, options.Strict);
                TryBoolAttribute(node, "IsDirectionReversed", slider->IsDirectionReversed, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "TickPlacement", consumed))
                    slider->TickPlacement = *value;
                if (const std::string* value = AttributeValue(node, "Header", consumed))
                    slider->Header = *value;
                AttributeValue(node, "IsThumbToolTipEnabled", consumed);
                ApplyImagePartAttribute(node, "TrackImage", slider->Style.PART_Track, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "FillImage", slider->Style.PART_FillTrack, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "ThumbImage", slider->Style.PART_Thumb, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "ThumbHoverImage", slider->Style.PART_Thumb, &ControlPartStyle::HoverImage, consumed, result, options);
                ApplyImagePartAttribute(node, "ThumbPressedImage", slider->Style.PART_Thumb, &ControlPartStyle::PressedImage, consumed, result, options);
            }

            if (auto* progress = dynamic_cast<ProgressBar*>(&element))
            {
                TryDoubleAttribute(node, "Minimum", progress->Minimum, consumed, result, options.Strict);
                TryDoubleAttribute(node, "Maximum", progress->Maximum, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "Value", consumed))
                {
                    double parsed = progress->Value;
                    if (ParseDoubleValue(*value, parsed))
                    {
                        progress->SetValue(parsed);
                        progress->AnimatedValue = progress->Value;
                    }
                    else
                    {
                        AttributeWarning(result, node, "Invalid ProgressBar Value.", options.Strict);
                    }
                }
                TryColorAttribute(node, "Foreground", progress->Fill, consumed, result, options.Strict);
                TryColorAttribute(node, "Fill", progress->Fill, consumed, result, options.Strict);
                TryBoolAttribute(node, "IsIndeterminate", progress->IsIndeterminate, consumed, result, options.Strict);
                TryBoolAttribute(node, "ShowError", progress->ShowError, consumed, result, options.Strict);
                TryBoolAttribute(node, "ShowPaused", progress->ShowPaused, consumed, result, options.Strict);
                ApplyImagePartAttribute(node, "BackgroundImage", progress->Style.PART_Background, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "FillImage", progress->Style.PART_Fill, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "OverlayImage", progress->Style.PART_Overlay, &ControlPartStyle::Image, consumed, result, options);
            }

            if (auto* checkBox = dynamic_cast<CheckBox*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "IsChecked", consumed))
                {
                    bool checked = false;
                    if (ParseBoolValue(*value, checked))
                        checkBox->SetState(checked ? CheckedState::Checked : CheckedState::Unchecked);
                    else
                        AttributeWarning(result, node, "Invalid CheckBox IsChecked value.", options.Strict);
                }
                std::function<void()> checkedAction;
                std::function<void()> uncheckedAction;
                if (const std::string* value = AttributeValue(node, "OnChecked", consumed))
                {
                    checkedAction = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (!checkedAction)
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnUnchecked", consumed))
                {
                    uncheckedAction = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (!uncheckedAction)
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (checkedAction || uncheckedAction)
                {
                    checkBox->OnCheckedChanged = [checkedAction = std::move(checkedAction), uncheckedAction = std::move(uncheckedAction)](CheckedState state)
                    {
                        if (state == CheckedState::Checked && checkedAction)
                            checkedAction();
                        else if (state == CheckedState::Unchecked && uncheckedAction)
                            uncheckedAction();
                    };
                }
                if (const std::string* value = AttributeValue(node, "OnIndeterminate", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (!action)
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                    else
                    {
                        auto previous = std::move(checkBox->OnCheckedChanged);
                        checkBox->OnCheckedChanged = [previous = std::move(previous), action = std::move(action)](CheckedState state)
                        {
                            if (previous)
                                previous(state);
                            if (state == CheckedState::Indeterminate)
                                action();
                        };
                    }
                }
            }

            if (auto* radioButton = dynamic_cast<RadioButton*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "GroupName", consumed))
                    radioButton->GroupName = *value;
                if (const std::string* value = AttributeValue(node, "IsChecked", consumed))
                {
                    bool checked = false;
                    if (ParseBoolValue(*value, checked))
                        radioButton->SetState(checked ? CheckedState::Checked : CheckedState::Unchecked);
                    else
                        AttributeWarning(result, node, "Invalid RadioButton IsChecked value.", options.Strict);
                }
                std::function<void()> checkedAction;
                std::function<void()> uncheckedAction;
                if (const std::string* value = AttributeValue(node, "OnChecked", consumed))
                {
                    checkedAction = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (!checkedAction)
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnUnchecked", consumed))
                {
                    uncheckedAction = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (!uncheckedAction)
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (checkedAction || uncheckedAction)
                {
                    radioButton->OnCheckedChanged = [checkedAction = std::move(checkedAction), uncheckedAction = std::move(uncheckedAction)](CheckedState state)
                    {
                        if (state == CheckedState::Checked && checkedAction)
                            checkedAction();
                        else if (state == CheckedState::Unchecked && uncheckedAction)
                            uncheckedAction();
                    };
                }
            }

            if (auto* radioButtons = dynamic_cast<RadioButtons*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Header", consumed))
                    radioButtons->Header = *value;
                TryIntAttribute(node, "SelectedIndex", radioButtons->SelectedIndex, consumed, result, options.Strict);
                TryIntAttribute(node, "MaxColumns", radioButtons->MaxColumns, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "OnSelectionChanged", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        radioButtons->OnSelectionChanged = [elementAction = std::move(elementAction), radioButtons](int32_t) { elementAction(radioButtons); };
                    else if (action)
                        radioButtons->OnSelectionChanged = [action = std::move(action)](int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
            }

            if (auto* toggleSwitch = dynamic_cast<ToggleSwitch*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Header", consumed))
                    toggleSwitch->Header = *value;
                if (const std::string* value = AttributeValue(node, "OnContent", consumed))
                    toggleSwitch->OnContent = *value;
                if (const std::string* value = AttributeValue(node, "OffContent", consumed))
                    toggleSwitch->OffContent = *value;
                if (const std::string* value = AttributeValue(node, "IsOn", consumed))
                {
                    bool isOn = false;
                    if (ParseBoolValue(*value, isOn))
                        toggleSwitch->SetState(isOn ? CheckedState::Checked : CheckedState::Unchecked);
                    else
                        AttributeWarning(result, node, "Invalid ToggleSwitch IsOn value.", options.Strict);
                }
                if (const std::string* value = AttributeValue(node, "OnToggled", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        toggleSwitch->OnCheckedChanged = [action = std::move(action)](CheckedState) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
            }

            if (auto* textBox = dynamic_cast<TextBox*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Text", consumed))
                    textBox->Text = *value;
                if (const std::string* value = AttributeValue(node, "PlaceholderText", consumed))
                    textBox->PlaceholderText = *value;
                if (const std::string* value = AttributeValue(node, "Header", consumed))
                    textBox->Header = *value;
                if (const std::string* value = AttributeValue(node, "OnTextChanged", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        textBox->OnTextChanged = [action = std::move(action)](const std::string&) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnSelectionChanged", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        textBox->OnSelectionChanged = [action = std::move(action)](int32_t, int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnSubmit", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        textBox->OnSubmit = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnCancel", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        textBox->OnCancel = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnGotFocus", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        textBox->OnGotFocus = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnLostFocus", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        textBox->OnLostFocus = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                TryBoolAttribute(node, "IsReadOnly", textBox->IsReadOnly, consumed, result, options.Strict);
                TryBoolAttribute(node, "AcceptsReturn", textBox->AcceptsReturn, consumed, result, options.Strict);
                TryIntAttribute(node, "MaxLength", textBox->MaxLength, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "TextWrapping", consumed))
                {
                    const std::string wrapping = ToLower(Trim(*value));
                    if (wrapping == "nowrap")
                        textBox->TextWrapping = false;
                    else if (wrapping == "wrap" || wrapping == "wrapwholewords")
                        textBox->TextWrapping = true;
                    else
                        AttributeWarning(result, node, "Invalid TextBox TextWrapping value.", options.Strict);
                }
                TryFloatAttribute(node, "FontSize", textBox->FontSize, consumed, result, options.Strict);
                AttributeValue(node, "FontFamily", consumed);
                AttributeValue(node, "FontStyle", consumed);
                AttributeValue(node, "CharacterSpacing", consumed);
                AttributeValue(node, "SelectionHighlightColor", consumed);
                AttributeValue(node, "IsSpellCheckEnabled", consumed);
                int32_t selectionStart = textBox->SelectionStart;
                int32_t selectionLength = textBox->SelectionLength;
                const bool hasSelectionStart = TryIntAttribute(node, "SelectionStart", selectionStart, consumed, result, options.Strict);
                const bool hasSelectionLength = TryIntAttribute(node, "SelectionLength", selectionLength, consumed, result, options.Strict);
                if (hasSelectionStart || hasSelectionLength)
                    textBox->SetSelection(selectionStart, selectionLength);
            }

            if (auto* list = dynamic_cast<ListBox*>(&element))
            {
                TryIntAttribute(node, "SelectedIndex", list->SelectedIndex, consumed, result, options.Strict);
                TryIntAttribute(node, "VisibleItems", list->VisibleItems, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "OnSelectionChanged", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        list->OnSelectionChanged = [elementAction = std::move(elementAction), list](int32_t) { elementAction(list); };
                    else if (action)
                        list->OnSelectionChanged = [action = std::move(action)](int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                AttributeValue(node, "SelectionMode", consumed);
                AttributeValue(node, "SelectedItem", consumed);
                AttributeValue(node, "ItemContainerStyle", consumed);
                if (const std::string* value = AttributeValue(node, "ItemTemplate", consumed))
                    list->ItemTemplateName = *value;
                ApplyImagePartAttribute(node, "BackgroundImage", list->Style.PART_Background, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "ItemBackgroundImage", list->Style.PART_ItemContainer, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "SelectionImage", list->Style.PART_SelectionVisual, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "HoverImage", list->Style.PART_HoverVisual, &ControlPartStyle::Image, consumed, result, options);
            }

            if (auto* tree = dynamic_cast<TreeView*>(&element))
            {
                TryIntAttribute(node, "SelectedIndex", tree->SelectedIndex, consumed, result, options.Strict);
                TryFloatAttribute(node, "ItemHeight", tree->ItemHeight, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "OnSelectionChanged", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        tree->OnSelectionChanged = [elementAction = std::move(elementAction), tree](int32_t) { elementAction(tree); };
                    else if (action)
                        tree->OnSelectionChanged = [action = std::move(action)](int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
            }

            if (auto* combo = dynamic_cast<ComboBox*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Header", consumed))
                    combo->Header = *value;
                if (const std::string* value = AttributeValue(node, "Text", consumed))
                    combo->Text = *value;
                if (const std::string* value = AttributeValue(node, "PlaceholderText", consumed))
                    combo->PlaceholderText = *value;
                TryBoolAttribute(node, "IsEditable", combo->IsEditable, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "IsDropDownOpen", consumed))
                {
                    bool isOpen = false;
                    if (ParseBoolValue(*value, isOpen))
                        combo->SetIsOpen(isOpen);
                    else
                        AttributeWarning(result, node, "Invalid ComboBox IsDropDownOpen value.", options.Strict);
                }
                TryFloatAttribute(node, "MaxDropDownHeight", combo->MaxDropDownHeight, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "OnDropDownOpened", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        combo->OnDropDownOpened = [elementAction = std::move(elementAction), combo] { elementAction(combo); };
                    else if (action)
                        combo->OnDropDownOpened = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnDropDownClosed", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        combo->OnDropDownClosed = [elementAction = std::move(elementAction), combo] { elementAction(combo); };
                    else if (action)
                        combo->OnDropDownClosed = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                AttributeValue(node, "TextSubmitted", consumed);
            }

            if (auto* scrollViewer = dynamic_cast<ScrollViewer*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "VerticalScrollBarVisibility", consumed))
                {
                    ScrollBarVisibility visibility;
                    if (ParseScrollBarVisibilityValue(*value, visibility))
                        scrollViewer->VerticalScrollBarVisibility = visibility;
                    else
                        AttributeWarning(result, node, "Invalid VerticalScrollBarVisibility value.", options.Strict);
                }
                if (const std::string* value = AttributeValue(node, "HorizontalScrollBarVisibility", consumed))
                {
                    ScrollBarVisibility visibility;
                    if (ParseScrollBarVisibilityValue(*value, visibility))
                        scrollViewer->HorizontalScrollBarVisibility = visibility;
                    else
                        AttributeWarning(result, node, "Invalid HorizontalScrollBarVisibility value.", options.Strict);
                }
                TryBoolAttribute(node, "ShowScrollBarButtons", scrollViewer->ShowScrollBarButtons, consumed, result, options.Strict);
                TryFloatAttribute(node, "LineScrollAmount", scrollViewer->LineScrollAmount, consumed, result, options.Strict);
                TryFloatAttribute(node, "PageScrollFactor", scrollViewer->PageScrollFactor, consumed, result, options.Strict);
                AttributeValue(node, "HorizontalScrollMode", consumed);
                AttributeValue(node, "VerticalScrollMode", consumed);
                if (const std::string* value = AttributeValue(node, "OnViewChanged", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        scrollViewer->OnViewChanged = [elementAction = std::move(elementAction), scrollViewer](Vec2) { elementAction(scrollViewer); };
                    else if (action)
                        scrollViewer->OnViewChanged = [action = std::move(action)](Vec2) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
            }

            if (auto* tab = dynamic_cast<TabView*>(&element))
            {
                TryIntAttribute(node, "SelectedIndex", tab->SelectedIndex, consumed, result, options.Strict);
                TryFloatAttribute(node, "HeaderHeight", tab->HeaderHeight, consumed, result, options.Strict);
                TryBoolAttribute(node, "IsAddTabButtonVisible", tab->IsAddTabButtonVisible, consumed, result, options.Strict);
                TryBoolAttribute(node, "CanCloseTabs", tab->CanCloseTabs, consumed, result, options.Strict);
                TryFloatAttribute(node, "AddButtonWidth", tab->AddButtonWidth, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "OnAddTabButtonClick", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        tab->OnAddTabClick = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnTabCloseRequested", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        tab->OnTabCloseRequested = [action = std::move(action)](int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnSelectionChanged", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        tab->OnSelectionChanged = [elementAction = std::move(elementAction), tab](int32_t) { elementAction(tab); };
                    else if (action)
                        tab->OnSelectionChanged = [action = std::move(action)](int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
            }

            if (auto* inventory = dynamic_cast<InventoryGrid*>(&element))
            {
                TryFloatAttribute(node, "SlotWidth", inventory->SlotWidth, consumed, result, options.Strict);
                TryFloatAttribute(node, "SlotHeight", inventory->SlotHeight, consumed, result, options.Strict);
                TryFloatAttribute(node, "Spacing", inventory->Spacing, consumed, result, options.Strict);
                TryFloatAttribute(node, "LineSpacing", inventory->LineSpacing, consumed, result, options.Strict);
                TryBoolAttribute(node, "AllowSwap", inventory->AllowSwap, consumed, result, options.Strict);
                TryBoolAttribute(node, "AllowMove", inventory->AllowMove, consumed, result, options.Strict);
                TryBoolAttribute(node, "AllowCopy", inventory->AllowCopy, consumed, result, options.Strict);
                auto bindInventoryPairAction = [&](std::string_view attribute, std::function<void(int32_t, int32_t)>& target)
                {
                    if (const std::string* value = AttributeValue(node, attribute, consumed))
                    {
                        std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                        std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                        if (elementAction)
                            target = [elementAction = std::move(elementAction), inventory](int32_t, int32_t) { elementAction(inventory); };
                        else if (action)
                            target = [action = std::move(action)](int32_t, int32_t) { action(); };
                        else
                            AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                    }
                };
                auto bindInventoryIndexAction = [&](std::string_view attribute, std::function<void(int32_t)>& target)
                {
                    if (const std::string* value = AttributeValue(node, attribute, consumed))
                    {
                        std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                        std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                        if (elementAction)
                            target = [elementAction = std::move(elementAction), inventory](int32_t) { elementAction(inventory); };
                        else if (action)
                            target = [action = std::move(action)](int32_t) { action(); };
                        else
                            AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                    }
                };
                if (const std::string* value = AttributeValue(node, "OnSelectionChanged", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        inventory->OnSelectionChanged = [elementAction = std::move(elementAction), inventory](int32_t) { elementAction(inventory); };
                    else if (action)
                        inventory->OnSelectionChanged = [action = std::move(action)](int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                bindInventoryPairAction("OnDrop", inventory->OnDrop);
                bindInventoryPairAction("OnMove", inventory->OnMove);
                bindInventoryPairAction("OnSwap", inventory->OnSwap);
                bindInventoryPairAction("OnCopy", inventory->OnCopy);
                bindInventoryIndexAction("OnUse", inventory->OnUse);
                bindInventoryIndexAction("OnInspect", inventory->OnInspect);
                bindInventoryIndexAction("OnDragStarted", inventory->OnDragStarted);
                bindInventoryIndexAction("OnDragCompleted", inventory->OnDragCompleted);
                if (const std::string* value = AttributeValue(node, "OnSplitStack", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        inventory->OnSplitStack = [elementAction = std::move(elementAction), inventory](int32_t, int32_t, int32_t) { elementAction(inventory); };
                    else if (action)
                        inventory->OnSplitStack = [action = std::move(action)](int32_t, int32_t, int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "SlotTemplate", consumed))
                    inventory->SlotTemplateName = *value;
                ApplyImagePartAttribute(node, "BackgroundImage", inventory->Style.PART_Background, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "SlotImage", inventory->Style.SlotStyle.PART_Background, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "SlotHoverImage", inventory->Style.SlotStyle.PART_HoverGlow, &ControlPartStyle::Image, consumed, result, options);
                ApplyImagePartAttribute(node, "SlotSelectedImage", inventory->Style.SlotStyle.PART_SelectionGlow, &ControlPartStyle::Image, consumed, result, options);
            }

            if (auto* radial = dynamic_cast<RadialMenu*>(&element))
            {
                TryFloatAttribute(node, "Radius", radial->Radius, consumed, result, options.Strict);
                TryFloatAttribute(node, "InnerRadius", radial->InnerRadius, consumed, result, options.Strict);
                TryIntAttribute(node, "SelectedIndex", radial->SelectedIndex, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "IsOpen", consumed))
                {
                    bool isOpen = true;
                    if (ParseBoolValue(*value, isOpen))
                    {
                        if (isOpen)
                            radial->Open();
                        else
                            radial->Close();
                    }
                    else
                    {
                        AttributeWarning(result, node, "Invalid RadialMenu IsOpen value.", options.Strict);
                    }
                }
                if (const std::string* value = AttributeValue(node, "OnSelectionChanged", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        radial->OnSelectionChanged = [elementAction = std::move(elementAction), radial](int32_t) { elementAction(radial); };
                    else if (action)
                        radial->OnSelectionChanged = [action = std::move(action)](int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnConfirm", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        radial->OnConfirm = [elementAction = std::move(elementAction), radial](int32_t) { elementAction(radial); };
                    else if (action)
                        radial->OnConfirm = [action = std::move(action)](int32_t) { action(); };
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnOpened", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        radial->OnOpened = [elementAction = std::move(elementAction), radial] { elementAction(radial); };
                    else if (action)
                        radial->OnOpened = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnClosed", consumed))
                {
                    std::function<void(UIElement*)> elementAction = options.Actions ? options.Actions->FindElementAction(*value) : std::function<void(UIElement*)>{};
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (elementAction)
                        radial->OnClosed = [elementAction = std::move(elementAction), radial] { elementAction(radial); };
                    else if (action)
                        radial->OnClosed = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
            }

            if (auto* wheel = dynamic_cast<CommandWheel*>(&element))
            {
                TryBoolAttribute(node, "IsOpen", wheel->IsOpen, consumed, result, options.Strict);
                TryBoolAttribute(node, "CloseOnActivate", wheel->CloseOnActivate, consumed, result, options.Strict);
                TryFloatAttribute(node, "OpenProgress", wheel->OpenProgress, consumed, result, options.Strict);
                TryFloatAttribute(node, "OpenAnimationSpeed", wheel->OpenAnimationSpeed, consumed, result, options.Strict);
                if (const std::string* value = AttributeValueAny(node, { "CenterLabel", "Header" }, consumed))
                    wheel->CenterLabel = *value;
            }

            if (auto* separator = dynamic_cast<Separator*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Orientation", consumed))
                {
                    Orientation orientation;
                    if (ParseOrientationValue(*value, orientation))
                        separator->Orientation = orientation;
                    else
                        AttributeWarning(result, node, "Invalid Separator Orientation.", options.Strict);
                }
                TryFloatAttribute(node, "ThicknessValue", separator->ThicknessValue, consumed, result, options.Strict);
                TryColorAttribute(node, "LineColor", separator->LineColor, consumed, result, options.Strict);
            }

            if (auto* badge = dynamic_cast<InfoBadge*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Value", consumed))
                    badge->Value = *value;
                else if (!node.Text.empty())
                    badge->Value = node.Text;
            }

            if (auto* expander = dynamic_cast<Expander*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Header", consumed))
                    expander->Header = *value;
                if (const std::string* value = AttributeValue(node, "Content", consumed))
                {
                    auto content = std::make_shared<TextBlock>();
                    content->Text = *value;
                    content->Foreground = expander->Foreground;
                    content->TextWrapping = TextWrappingMode::Wrap;
                    expander->SetChild(content);
                }
                if (const std::string* value = AttributeValue(node, "IsExpanded", consumed))
                {
                    bool expanded = false;
                    if (ParseBoolValue(*value, expanded))
                    {
                        expander->SetIsExpanded(expanded);
                        expander->ExpansionProgress = expanded ? 1.0f : 0.0f;
                    }
                    else
                    {
                        AttributeWarning(result, node, "Invalid Expander IsExpanded value.", options.Strict);
                    }
                }
                if (const std::string* value = AttributeValue(node, "ExpandDirection", consumed))
                {
                    const std::string direction = ToLower(Trim(*value));
                    if (direction == "down")
                        expander->SetExpandDirection(ExpandDirection::Down);
                    else if (direction == "up")
                        expander->SetExpandDirection(ExpandDirection::Up);
                    else if (direction == "left")
                        expander->SetExpandDirection(ExpandDirection::Left);
                    else if (direction == "right")
                        expander->SetExpandDirection(ExpandDirection::Right);
                    else
                        AttributeWarning(result, node, "Invalid Expander ExpandDirection value.", options.Strict);
                }
                if (const std::string* value = AttributeValue(node, "HorizontalContentAlignment", consumed))
                {
                    HorizontalAlignment alignment;
                    if (ParseHorizontalAlignmentValue(*value, alignment))
                        expander->HorizontalContentAlignment = alignment;
                    else
                        AttributeWarning(result, node, "Invalid HorizontalContentAlignment value.", options.Strict);
                }
                if (const std::string* value = AttributeValue(node, "VerticalContentAlignment", consumed))
                {
                    VerticalAlignment alignment;
                    if (ParseVerticalAlignmentValue(*value, alignment))
                        expander->VerticalContentAlignment = alignment;
                    else
                        AttributeWarning(result, node, "Invalid VerticalContentAlignment value.", options.Strict);
                }
                if (const std::string* value = AttributeValue(node, "OnExpanded", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        expander->OnExpanded = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnCollapsed", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        expander->OnCollapsed = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
            }

            if (auto* infoBar = dynamic_cast<InfoBar*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Title", consumed))
                    infoBar->Title = *value;
                if (const std::string* value = AttributeValue(node, "Message", consumed))
                    infoBar->Message = *value;
                if (const std::string* value = AttributeValue(node, "Severity", consumed))
                    infoBar->Severity = *value;
                TryBoolAttribute(node, "IsOpen", infoBar->IsOpen, consumed, result, options.Strict);
                TryBoolAttribute(node, "IsClosable", infoBar->IsClosable, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "OnCloseButtonClick", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        infoBar->OnCloseButtonClick = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
            }

            if (auto* tip = dynamic_cast<TeachingTip*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Title", consumed))
                    tip->Title = *value;
                if (const std::string* value = AttributeValueAny(node, { "Subtitle", "Message" }, consumed))
                    tip->Subtitle = *value;
                TryBoolAttribute(node, "IsOpen", tip->IsOpen, consumed, result, options.Strict);
            }

            if (auto* popup = dynamic_cast<Popup*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "IsOpen", consumed))
                {
                    bool isOpen = false;
                    if (ParseBoolValue(*value, isOpen))
                        popup->SetIsOpen(isOpen);
                    else
                        AttributeWarning(result, node, "Invalid Flyout/Popup IsOpen value.", options.Strict);
                }
                TryFloatAttribute(node, "PlacementX", popup->Placement.x, consumed, result, options.Strict);
                TryFloatAttribute(node, "PlacementY", popup->Placement.y, consumed, result, options.Strict);
            }

            if (auto* navItem = dynamic_cast<NavigationViewItem*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Icon", consumed))
                {
                    navItem->Icon = *value;
                    if (options.ResolveTexture)
                        navItem->IconTexture = options.ResolveTexture(*value);
                }
            }

            if (auto* dialog = dynamic_cast<ContentDialog*>(&element))
            {
                if (const std::string* value = AttributeValue(node, "Title", consumed))
                    dialog->Title = *value;
                if (const std::string* value = AttributeValue(node, "Content", consumed))
                    dialog->Content = *value;
                if (const std::string* value = AttributeValue(node, "IsOpen", consumed))
                {
                    bool isOpen = false;
                    if (ParseBoolValue(*value, isOpen))
                    {
                        if (isOpen)
                            dialog->Open();
                        else
                            dialog->Close();
                    }
                    else
                    {
                        AttributeWarning(result, node, "Invalid ContentDialog IsOpen value.", options.Strict);
                    }
                }
                if (const std::string* value = AttributeValue(node, "PrimaryButtonText", consumed))
                    dialog->PrimaryButtonText = *value;
                if (const std::string* value = AttributeValue(node, "SecondaryButtonText", consumed))
                    dialog->SecondaryButtonText = *value;
                if (const std::string* value = AttributeValue(node, "CloseButtonText", consumed))
                    dialog->CloseButtonText = *value;
                if (const std::string* value = AttributeValue(node, "OnPrimaryButtonClick", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        dialog->OnPrimaryButtonClick = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnSecondaryButtonClick", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        dialog->OnSecondaryButtonClick = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnCloseButtonClick", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        dialog->OnCloseButtonClick = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "DefaultButton", consumed))
                {
                    ContentDialogButton button = dialog->DefaultButton;
                    if (ParseContentDialogButtonValue(*value, button))
                        dialog->DefaultButton = button;
                    else
                        AttributeWarning(result, node, "Invalid ContentDialog DefaultButton value.", options.Strict);
                }
                TryBoolAttribute(node, "IsPrimaryButtonEnabled", dialog->IsPrimaryButtonEnabled, consumed, result, options.Strict);
                TryBoolAttribute(node, "IsSecondaryButtonEnabled", dialog->IsSecondaryButtonEnabled, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(node, "OnOpened", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        dialog->OnOpened = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
                if (const std::string* value = AttributeValue(node, "OnClosed", consumed))
                {
                    std::function<void()> action = options.Actions ? options.Actions->FindAction(*value) : std::function<void()>{};
                    if (action)
                        dialog->OnClosed = std::move(action);
                    else
                        AttributeWarning(result, node, "Action '" + *value + "' was not found.", false);
                }
            }
        }

        inline void ApplyStyleReference(UIElement& element, const XmlNode& node, std::vector<std::string>& consumed, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            const std::string* styleName = AttributeValue(node, "Style", consumed);
            if (!styleName || styleName->empty())
                return;
            const std::string resolvedStyleName = ParseStaticResourceReference(*styleName);
            if (resolvedStyleName.empty())
            {
                AttributeWarning(result, node, "Style must use {StaticResource Key}.", options.Strict);
                return;
            }

            const XamlStyleDefinition* style = options.Styles ? options.Styles->FindStyle(resolvedStyleName) : nullptr;
            if (!style)
            {
                AttributeWarning(result, node, "Style '" + resolvedStyleName + "' was not found.", options.Strict);
                return;
            }

            if (!style->TargetType.empty() && !EqualsIgnoreCase(style->TargetType, node.Name))
                AttributeWarning(result, node, "Style '" + resolvedStyleName + "' targets '" + style->TargetType + "' but is applied to '" + node.Name + "'.", options.Strict);

            XmlNode styleNode;
            styleNode.Name = node.Name;
            styleNode.Line = node.Line;
            styleNode.Column = node.Column;
            for (const XamlStyleSetter& setter : style->Setters)
            {
                XmlAttribute attribute;
                attribute.Name = setter.Property;
                attribute.Value = setter.Value;
                attribute.Line = node.Line;
                attribute.Column = node.Column;
                styleNode.Attributes.push_back(std::move(attribute));
            }

            std::vector<std::string> styleConsumed;
            ApplyCommonProperties(element, styleNode, styleConsumed, result, options);
            ApplySpecificProperties(element, styleNode, styleConsumed, result, options);
            ApplyRegisteredProperties(element, styleNode, styleConsumed, result, options);
        }

        inline InventoryItem ParseInventoryItem(const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            std::vector<std::string> consumed;
            InventoryItem item;
            if (const std::string* value = AttributeValue(node, "Content", consumed))
                item.Name = *value;
            if (const std::string* value = AttributeValue(node, "QuantityText", consumed))
                item.QuantityText = *value;
            if (const std::string* value = AttributeValue(node, "BadgeText", consumed))
                item.BadgeText = *value;
            if (const std::string* value = AttributeValue(node, "Rarity", consumed))
                item.Rarity = *value;
            if (const std::string* value = AttributeValue(node, "Tint", consumed))
            {
                if (ParseColorValue(*value, item.Tint))
                    item.HasTint = true;
                else
                    AttributeWarning(result, node, "Invalid InventoryItem Tint value.", options.Strict);
            }
            TryIntAttribute(node, "StackCount", item.StackCount, consumed, result, options.Strict);
            TryIntAttribute(node, "MaxStack", item.MaxStack, consumed, result, options.Strict);
            TryDoubleAttribute(node, "Cooldown", item.Cooldown, consumed, result, options.Strict);
            TryBoolAttribute(node, "IsEmpty", item.IsEmpty, consumed, result, options.Strict);
            if (const std::string* value = AttributeValue(node, "Icon", consumed))
            {
                if (options.ResolveImage)
                    item.Icon = options.ResolveImage(*value);
                else if (options.ResolveTexture)
                    item.Icon.texture = options.ResolveTexture(*value);
                else
                    AttributeWarning(result, node, "InventoryItem Icon requires ResolveImage or ResolveTexture.", options.Strict);
            }
            for (const auto& attribute : node.Attributes)
            {
                if (!WasConsumed(consumed, attribute.Name))
                    AttributeWarning(result, node, "Unknown InventoryItem property '" + attribute.Name + "'.", options.Strict);
            }
            return item;
        }

        inline std::shared_ptr<UIElement> BuildElement(const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options);

        inline void AddChildToParent(UIElement& parent, std::shared_ptr<UIElement> child, const XmlNode& childNode, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            if (!child)
                return;

            if (auto* canvas = dynamic_cast<Canvas*>(&parent))
            {
                float left = 0.0f;
                float top = 0.0f;
                float right = AutoSize;
                float bottom = AutoSize;
                int32_t zIndex = 0;
                if (const XmlAttribute* attr = FindAttribute(childNode, "Canvas.Left"))
                    ParseFloatValue(attr->Value, left);
                if (const XmlAttribute* attr = FindAttribute(childNode, "Canvas.Top"))
                    ParseFloatValue(attr->Value, top);
                if (const XmlAttribute* attr = FindAttribute(childNode, "Canvas.Right"))
                    ParseFloatValue(attr->Value, right);
                if (const XmlAttribute* attr = FindAttribute(childNode, "Canvas.Bottom"))
                    ParseFloatValue(attr->Value, bottom);
                if (const XmlAttribute* attr = FindAttribute(childNode, "Canvas.ZIndex"))
                    ParseIntValue(attr->Value, zIndex);
                canvas->AddChild(child, left, top, right, bottom, zIndex);
                return;
            }

            if (auto* border = dynamic_cast<Border*>(&parent))
            {
                if (border->Child)
                    AttributeWarning(result, childNode, "Border only accepts one child. Extra child skipped.", options.Strict);
                else
                    border->SetChild(child);
                return;
            }

            if (auto* panel = dynamic_cast<Panel*>(&parent))
            {
                panel->AddChild(child);
                return;
            }

            AttributeWarning(result, childNode, "Parent element does not accept visual children.", options.Strict);
        }

        inline void BuildListItems(ListBox& list, const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            std::vector<ListBoxItemData> items;
            for (const XmlNode& child : node.Children)
            {
                if (!EqualsIgnoreCase(child.Name, "ListBoxItem") && !EqualsIgnoreCase(child.Name, "ComboBoxItem") && !EqualsIgnoreCase(child.Name, "x:String"))
                {
                    AttributeWarning(result, child, "ListBox/ComboBox only accepts <ListBoxItem Content=\"...\" />, <ComboBoxItem Content=\"...\" /> or <x:String>...</x:String> children.", options.Strict);
                    continue;
                }

                std::vector<std::string> consumed;
                ListBoxItemData item;
                if (const std::string* value = AttributeValue(child, "Content", consumed))
                    item.Text = *value;
                else
                    item.Text = child.Text;
                if (const std::string* value = AttributeValue(child, "BadgeText", consumed))
                    item.BadgeText = *value;
                if (const std::string* value = AttributeValue(child, "Foreground", consumed))
                {
                    if (ParseColorValue(*value, item.Foreground))
                        item.HasForeground = true;
                    else
                        AttributeWarning(result, child, "Invalid Item Foreground value.", options.Strict);
                }
                TryBoolAttribute(child, "IsEnabled", item.IsEnabled, consumed, result, options.Strict);
                if (const std::string* value = AttributeValue(child, "Icon", consumed))
                {
                    if (options.ResolveImage)
                        item.Icon = options.ResolveImage(*value);
                    else if (options.ResolveTexture)
                        item.Icon.texture = options.ResolveTexture(*value);
                    else
                        AttributeWarning(result, child, "Item Icon requires ResolveImage or ResolveTexture.", options.Strict);
                }
                items.push_back(std::move(item));

                for (const auto& attribute : child.Attributes)
                {
                    if (!WasConsumed(consumed, attribute.Name))
                        AttributeWarning(result, child, "Unknown Item property '" + attribute.Name + "'.", options.Strict);
                }
            }
            if (!items.empty())
                list.SetItemData(std::move(items));
        }

        inline TreeViewItem ParseTreeViewItemNode(const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            std::vector<std::string> consumed;
            TreeViewItem item;
            if (const std::string* value = AttributeValue(node, "Content", consumed))
                item.Text = *value;
            else if (!node.Text.empty())
                item.Text = Trim(node.Text);
            if (item.Text.empty())
                item.Text = "Item";
            TryBoolAttribute(node, "IsExpanded", item.IsExpanded, consumed, result, options.Strict);

            for (const XmlNode& child : node.Children)
            {
                if (EqualsIgnoreCase(child.Name, "TreeViewItem") || EqualsIgnoreCase(child.Name, "TreeViewNode") || EqualsIgnoreCase(child.Name, "Item"))
                    item.Children.push_back(ParseTreeViewItemNode(child, result, options));
                else if (!child.Name.empty())
                    AttributeWarning(result, child, "TreeViewItem only accepts nested <TreeViewItem> children.", options.Strict);
            }

            for (const auto& attribute : node.Attributes)
            {
                if (!WasConsumed(consumed, attribute.Name))
                    AttributeWarning(result, node, "Unknown TreeViewItem property '" + attribute.Name + "'.", options.Strict);
            }
            return item;
        }

        inline void BuildTreeViewItems(TreeView& tree, const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            std::vector<TreeViewItem> items;
            for (const XmlNode& child : node.Children)
            {
                if (!EqualsIgnoreCase(child.Name, "TreeViewItem") && !EqualsIgnoreCase(child.Name, "TreeViewNode") && !EqualsIgnoreCase(child.Name, "Item"))
                {
                    AttributeWarning(result, child, "TreeView only accepts <TreeViewItem Content=\"...\" /> children.", options.Strict);
                    continue;
                }
                items.push_back(ParseTreeViewItemNode(child, result, options));
            }
            if (!items.empty())
                tree.Items = std::move(items);
        }

        inline void BuildRadioButtonsItems(RadioButtons& radioButtons, const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            if (node.Children.empty())
            {
                if (!radioButtons.Items.empty())
                    radioButtons.RefreshItems();
                return;
            }

            std::vector<std::string> items;
            int32_t checkedIndex = -1;
            for (const XmlNode& child : node.Children)
            {
                if (!EqualsIgnoreCase(child.Name, "RadioButton") && !EqualsIgnoreCase(child.Name, "Item") && !EqualsIgnoreCase(child.Name, "x:String"))
                {
                    AttributeWarning(result, child, "RadioButtons accepts <RadioButton Content=\"...\" />, <Item Content=\"...\" /> or <x:String>...</x:String> children.", options.Strict);
                    continue;
                }

                std::vector<std::string> consumed;
                std::string text;
                if (const std::string* value = AttributeValue(child, "Content", consumed))
                    text = *value;
                else
                    text = child.Text;
                if (text.empty())
                    text = "Option";

                bool isChecked = false;
                if (const std::string* value = AttributeValue(child, "IsChecked", consumed))
                {
                    if (ParseBoolValue(*value, isChecked) && isChecked)
                        checkedIndex = static_cast<int32_t>(items.size());
                }

                items.push_back(std::move(text));
                for (const auto& attribute : child.Attributes)
                {
                    if (!WasConsumed(consumed, attribute.Name))
                        AttributeWarning(result, child, "Unknown RadioButtons item property '" + attribute.Name + "'.", options.Strict);
                }
            }

            if (!items.empty())
            {
                radioButtons.Items = std::move(items);
                if (checkedIndex >= 0)
                    radioButtons.SelectedIndex = checkedIndex;
                radioButtons.RefreshItems();
            }
        }

        inline void BuildTabViewItems(TabView& tab, const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            for (const XmlNode& child : node.Children)
            {
                if (!EqualsIgnoreCase(child.Name, "TabViewItem"))
                {
                    AttributeWarning(result, child, "TabView only accepts <TabViewItem> children.", options.Strict);
                    continue;
                }

                std::vector<std::string> consumed;
                std::string header = "Tab";
                if (const std::string* value = AttributeValue(child, "Header", consumed))
                    header = *value;

                std::shared_ptr<UIElement> content;
                for (const XmlNode& visual : child.Children)
                {
                    if (content)
                    {
                        AttributeWarning(result, visual, "TabViewItem only accepts one visual child. Extra child skipped.", options.Strict);
                        continue;
                    }
                    content = BuildElement(visual, result, options);
                }

                for (const auto& attribute : child.Attributes)
                {
                    if (!WasConsumed(consumed, attribute.Name))
                        AttributeWarning(result, child, "Unknown TabViewItem property '" + attribute.Name + "'.", options.Strict);
                }

                tab.AddTab(header, content);
            }
            if (!tab.Items.empty())
                tab.SelectedIndex = std::clamp(tab.SelectedIndex, 0, static_cast<int32_t>(tab.Items.size()) - 1);
        }

        inline std::shared_ptr<UIElement> BuildVisualGroup(const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            std::vector<std::shared_ptr<UIElement>> visuals;
            for (const XmlNode& child : node.Children)
            {
                std::shared_ptr<UIElement> visual = BuildElement(child, result, options);
                if (visual)
                    visuals.push_back(std::move(visual));
            }

            if (visuals.empty())
                return nullptr;
            if (visuals.size() == 1)
                return std::move(visuals.front());

            auto stack = std::make_shared<StackPanel>();
            stack->Spacing = 8.0f;
            for (auto& visual : visuals)
                stack->AddChild(std::move(visual));
            return stack;
        }

        inline void BuildControlExampleSlots(ControlExample& controlExample, const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            for (const XmlNode& child : node.Children)
            {
                const std::string local = LowerLocalName(child.Name);
                const size_t dot = local.find('.');
                const std::string property = dot == std::string::npos ? std::string{} : local.substr(dot + 1);

                if (property == "example")
                {
                    controlExample.SetExample(BuildVisualGroup(child, result, options));
                    continue;
                }
                if (property == "options")
                {
                    controlExample.SetOptions(BuildVisualGroup(child, result, options));
                    continue;
                }
                if (property == "output")
                {
                    controlExample.SetOutput(BuildVisualGroup(child, result, options));
                    continue;
                }
                if (property == "xaml" || property == "csharp" || property == "substitutions")
                    continue;
                if (!property.empty())
                {
                    AttributeWarning(result, child, "Unsupported ControlExample section <" + child.Name + "> skipped.", options.Strict);
                    continue;
                }

                if (!controlExample.Example)
                    controlExample.SetExample(BuildElement(child, result, options));
                else
                    AttributeWarning(result, child, "ControlExample already has an example visual. Extra visual skipped.", options.Strict);
            }
        }

        inline void BuildInventoryItems(InventoryGrid& inventory, const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            std::vector<InventoryItem> items;
            for (const XmlNode& child : node.Children)
            {
                if (!EqualsIgnoreCase(child.Name, "InventoryItem"))
                {
                    AttributeWarning(result, child, "InventoryGrid only accepts <InventoryItem /> children.", options.Strict);
                    continue;
                }
                items.push_back(ParseInventoryItem(child, result, options));
            }
            inventory.SetItems(std::move(items));
        }

        inline void BuildRadialMenuItems(RadialMenu& radial, const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            std::vector<RadialMenuItem> items;
            for (const XmlNode& child : node.Children)
            {
                if (!EqualsIgnoreCase(child.Name, "RadialMenuItem") && !EqualsIgnoreCase(child.Name, "CommandWheelItem"))
                {
                    AttributeWarning(result, child, "RadialMenu only accepts <RadialMenuItem /> children.", options.Strict);
                    continue;
                }

                RadialMenuItem item {};
                std::vector<std::string> consumed;
                if (const std::string* value = AttributeValueAny(child, { "Label", "Content" }, consumed))
                    item.Label = *value;
                else if (!child.Text.empty())
                    item.Label = child.Text;
                TryBoolAttribute(child, "Enabled", item.Enabled, consumed, result, options.Strict);
                if (const std::string* value = AttributeValueAny(child, { "Icon", "IconSource" }, consumed))
                {
                    if (options.ResolveTexture)
                        item.Icon = options.ResolveTexture(*value);
                    if (!item.Icon)
                        AttributeWarning(result, child, "RadialMenuItem Icon requires ResolveTexture.", options.Strict);
                }
                if (item.Label.empty())
                    item.Label = "Item " + std::to_string(items.size() + 1);

                for (const XmlAttribute& attribute : child.Attributes)
                {
                    if (!WasConsumed(consumed, attribute.Name))
                        AttributeWarning(result, child, "Unknown RadialMenuItem property '" + attribute.Name + "'.", options.Strict);
                }
                items.push_back(std::move(item));
            }

            radial.Items = std::move(items);
            radial.SelectedIndex = radial.Items.empty() ? -1 : std::clamp(radial.SelectedIndex, 0, static_cast<int32_t>(radial.Items.size()) - 1);
        }

        inline void ReportUnknownAttributes(const XmlNode& node, const std::vector<std::string>& consumed, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            for (const auto& attribute : node.Attributes)
            {
                if (!WasConsumed(consumed, attribute.Name))
                    AttributeWarning(result, node, "Unknown property '" + attribute.Name + "' on <" + node.Name + ">.", options.Strict);
            }
        }

        inline std::shared_ptr<UIElement> BuildElement(const XmlNode& node, XamlLoadResult& result, const XamlLoadOptions& options)
        {
            const std::string lowered = LowerLocalName(node.Name);
            if (lowered == "item" || lowered == "listboxitem" || lowered == "comboboxitem" || lowered == "treeviewitem" || lowered == "treeviewnode" || lowered == "string" || lowered == "x:string" || lowered == "tabitem" || lowered == "tabviewitem" || lowered == "inventoryitem" || lowered == "radialmenuitem" || lowered == "commandwheelitem" || lowered.find('.') != std::string::npos)
            {
                AttributeWarning(result, node, "Element <" + node.Name + "> is only valid as data inside a supported parent.", options.Strict);
                return nullptr;
            }

            std::shared_ptr<UIElement> element = CreateElement(node, result, options);
            if (!element)
                return nullptr;

            std::vector<std::string> consumed;
            ApplyStyleReference(*element, node, consumed, result, options);
            ApplyCommonProperties(*element, node, consumed, result, options);
            ApplySpecificProperties(*element, node, consumed, result, options);
            ApplyRegisteredProperties(*element, node, consumed, result, options);

            if (auto* radioButtons = dynamic_cast<RadioButtons*>(element.get()))
                BuildRadioButtonsItems(*radioButtons, node, result, options);
            else if (auto* tree = dynamic_cast<TreeView*>(element.get()))
                BuildTreeViewItems(*tree, node, result, options);
            else if (auto* list = dynamic_cast<ListBox*>(element.get()))
                BuildListItems(*list, node, result, options);
            else if (auto* tab = dynamic_cast<TabView*>(element.get()))
                BuildTabViewItems(*tab, node, result, options);
            else if (auto* controlExample = dynamic_cast<ControlExample*>(element.get()))
                BuildControlExampleSlots(*controlExample, node, result, options);
            else if (auto* inventory = dynamic_cast<InventoryGrid*>(element.get()))
                BuildInventoryItems(*inventory, node, result, options);
            else if (auto* radial = dynamic_cast<RadialMenu*>(element.get()))
                BuildRadialMenuItems(*radial, node, result, options);
            else if (auto* dialog = dynamic_cast<ContentDialog*>(element.get()))
            {
                for (const XmlNode& child : node.Children)
                {
                    if (EqualsIgnoreCase(child.Name, "TextBlock"))
                    {
                        if (const XmlAttribute* text = FindAttribute(child, "Text"))
                            dialog->Content = text->Value;
                        else if (!child.Text.empty())
                            dialog->Content = child.Text;
                    }
                    else
                    {
                        AttributeWarning(result, child, "ContentDialog currently accepts a TextBlock child as Content.", options.Strict);
                    }
                }
            }
            else if (dynamic_cast<Button*>(element.get()) || dynamic_cast<TextBlock*>(element.get()))
            {
                for (const XmlNode& child : node.Children)
                    AttributeWarning(result, child, "<" + node.Name + "> does not accept visual children yet.", options.Strict);
            }
            else
            {
                for (const XmlNode& child : node.Children)
                {
                    std::shared_ptr<UIElement> visual = BuildElement(child, result, options);
                    AddChildToParent(*element, visual, child, result, options);
                }
            }

            ReportUnknownAttributes(node, consumed, result, options);
            return element;
        }
    }

    inline XamlLoadResult LoadXamlFromString(const std::string& text, const XamlLoadOptions& options = {})
    {
        XamlLoadResult result;
        XamlDetail::XmlNode rootNode;
        XamlDetail::XmlParser parser(text, result);
        if (!parser.Parse(rootNode))
            return result;

        result.Root = XamlDetail::BuildElement(rootNode, result, options);
        if (!result.Root)
            XamlDetail::AddIssue(result, "XAML did not produce a UIElement root.", rootNode.Line, rootNode.Column, false);
        return result;
    }

    inline XamlLoadResult LoadXamlFromFile(const std::string& path, const XamlLoadOptions& options = {})
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            XamlLoadResult result;
            XamlDetail::AddIssue(result, "Could not open XAML file '" + path + "'.", 1, 1, false);
            return result;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return LoadXamlFromString(buffer.str(), options);
    }

    inline void AddStyleIssue(XamlStyleLoadResult& result, std::string message, int line, int column, bool warning)
    {
        XamlError error;
        error.Message = std::move(message);
        error.Line = line;
        error.Column = column;
        error.IsWarning = warning;
        result.Errors.push_back(std::move(error));
    }

    inline void ParseXamlStyleNode(const XamlDetail::XmlNode& node, XamlStyleRegistry& registry, XamlStyleLoadResult& result, bool strict)
    {
        if (!XamlDetail::EqualsIgnoreCase(node.Name, "Style"))
        {
            AddStyleIssue(result, "Expected <Style> but found <" + node.Name + ">.", node.Line, node.Column, !strict);
            return;
        }

        XamlStyleDefinition style;
        if (const XamlDetail::XmlAttribute* name = XamlDetail::FindAttribute(node, "x:Key"))
            style.Name = name->Value;
        if (const XamlDetail::XmlAttribute* targetType = XamlDetail::FindAttribute(node, "TargetType"))
            style.TargetType = targetType->Value;

        if (style.Name.empty())
            AddStyleIssue(result, "Style is missing required x:Key attribute.", node.Line, node.Column, false);

        for (const auto& child : node.Children)
        {
            if (!XamlDetail::EqualsIgnoreCase(child.Name, "Setter"))
            {
                AddStyleIssue(result, "Style only accepts <Setter Property=\"...\" Value=\"...\" /> children.", child.Line, child.Column, !strict);
                continue;
            }

            XamlStyleSetter setter;
            if (const XamlDetail::XmlAttribute* property = XamlDetail::FindAttribute(child, "Property"))
                setter.Property = property->Value;
            if (const XamlDetail::XmlAttribute* value = XamlDetail::FindAttribute(child, "Value"))
                setter.Value = value->Value;

            if (setter.Property.empty())
            {
                AddStyleIssue(result, "Setter is missing required Property attribute.", child.Line, child.Column, false);
                continue;
            }

            style.Setters.push_back(std::move(setter));
        }

        if (!style.Name.empty())
            registry.RegisterStyle(std::move(style));
    }

    inline XamlStyleLoadResult LoadXamlStylesFromString(const std::string& text, XamlStyleRegistry& registry, bool strict = false)
    {
        XamlLoadResult parseResult;
        XamlDetail::XmlNode rootNode;
        XamlDetail::XmlParser parser(text, parseResult);
        XamlStyleLoadResult result;
        if (!parser.Parse(rootNode))
        {
            result.Errors = std::move(parseResult.Errors);
            return result;
        }

        for (const auto& error : parseResult.Errors)
            result.Errors.push_back(error);

        if (XamlDetail::EqualsIgnoreCase(rootNode.Name, "ResourceDictionary"))
        {
            for (const auto& child : rootNode.Children)
                ParseXamlStyleNode(child, registry, result, strict);
            return result;
        }

        AddStyleIssue(result, "Style file root must be <ResourceDictionary>.", rootNode.Line, rootNode.Column, false);
        return result;
    }

    inline XamlStyleLoadResult LoadXamlStylesFromFile(const std::string& path, XamlStyleRegistry& registry, bool strict = false)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            XamlStyleLoadResult result;
            AddStyleIssue(result, "Could not open XAML style file '" + path + "'.", 1, 1, false);
            return result;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return LoadXamlStylesFromString(buffer.str(), registry, strict);
    }

    inline XamlLoadResult RunXamlSmokeTest()
    {
        const std::string styles = R"(
<ResourceDictionary>
    <Style x:Key="PrimaryButton" TargetType="Button">
        <Setter Property="Background" Value="#182230" />
        <Setter Property="Foreground" Value="#F0F7FF" />
        <Setter Property="BorderBrush" Value="#6BA0CC" />
        <Setter Property="BorderThickness" Value="1" />
        <Setter Property="CornerRadius" Value="8" />
        <Setter Property="Padding" Value="12,6" />
    </Style>
</ResourceDictionary>
)";

        const std::string xaml = R"(
<Canvas Width="800" Height="600" Background="#00000000">
    <Border x:Name="InventoryPanel" Canvas.Left="24" Canvas.Top="20" Width="520" Height="420" Padding="18" Background="#050A10E8" BorderBrush="#5CAAE0AA" BorderThickness="1" CornerRadius="12">
        <StackPanel Orientation="Vertical" Spacing="12">
            <TextBlock Text="INVENTORY / CARGO HOLD" Foreground="#B0F2FF" TextWrapping="Wrap" />
            <InfoBadge Value="3" />
            <Slider Minimum="0" Maximum="100" Value="42" Width="240" />
            <ProgressBar Minimum="0" Maximum="1" Value="0.64" Width="240" Height="18" Foreground="#52C6F6" />
            <TabView SelectedIndex="1" Width="320" Height="120" OnSelectionChanged="SelectTab">
                <TabViewItem Header="Cargo">
                    <TextBlock Text="Cargo tab" />
                </TabViewItem>
                <TabViewItem Header="Skills">
                    <TextBlock Text="Skills tab" />
                </TabViewItem>
            </TabView>
            <InventoryGrid x:Name="CargoGrid" SlotWidth="58" SlotHeight="58" Spacing="8" LineSpacing="8" AllowSwap="true" AllowMove="true">
                <InventoryItem Content="Aegis Cell MK III" QuantityText="x3" IsEmpty="false" StackCount="3" />
                <InventoryItem Content="Empty" IsEmpty="true" />
            </InventoryGrid>
            <SkillTree x:Name="PlayerSkills" Power="7" />
            <Button Content="Equip Cell" Width="240" Height="38" Style="{StaticResource PrimaryButton}" OnClick="EquipSelectedCell" />
            <ContentDialog x:Name="ConfirmEquipDialog" Title="Equip item?" PrimaryButtonText="Equip" SecondaryButtonText="Cancel" CloseButtonText="Close" IsOpen="false" OnOpened="DialogOpened" OnClosed="DialogClosed">
                <TextBlock Text="This dialog is loaded from the XAML smoke test." TextWrapping="Wrap" />
            </ContentDialog>
        </StackPanel>
    </Border>
</Canvas>
)";
        XamlActionRegistry actions;
        XamlStyleRegistry styleRegistry;
        XamlControlRegistry controls;
        XamlPropertyRegistry properties;
        actions.RegisterAction("EquipSelectedCell", []() {});
        actions.RegisterAction("SelectTab", []() {});
        actions.RegisterAction("DialogOpened", []() {});
        actions.RegisterAction("DialogClosed", []() {});
        controls.RegisterControl("SkillTree", []()
        {
            auto text = std::make_shared<TextBlock>();
            text->Text = "Custom SkillTree";
            return text;
        });
        properties.RegisterProperty("SkillTree", "Power", [](UIElement& element, const std::string& value)
        {
            element.ToolTip = "Power " + value;
            return !value.empty();
        });
        XamlLoadOptions options;
        options.Actions = &actions;
        options.Styles = &styleRegistry;
        options.Controls = &controls;
        options.Properties = &properties;
        XamlStyleLoadResult styleResult = LoadXamlStylesFromString(styles, styleRegistry);
        XamlLoadResult result = LoadXamlFromString(xaml, options);
        result.Errors.insert(result.Errors.end(), styleResult.Errors.begin(), styleResult.Errors.end());
        if (result.Root && !result.Root->FindName("CargoGrid"))
            result.Errors.push_back({ "XAML smoke test could not find CargoGrid by x:Name.", 1, 1, false });
        if (result.Root && !result.Root->FindName("ConfirmEquipDialog"))
            result.Errors.push_back({ "XAML smoke test could not find ConfirmEquipDialog by x:Name.", 1, 1, false });
        return result;
    }
}
