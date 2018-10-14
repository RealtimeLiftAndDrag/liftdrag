#include "UI_Text.hpp"

#include <cctype>

#include "GLFW/glfw3.h"

#include "Util.hpp"



namespace UI {

    bool Text::setup(const std::string & resourceDir) {
        return ::Text::setup(resourceDir);
    }

    Text::Text(const std::string & string, const ivec2 & align, const vec4 & color) :
        Text(string, align, color, ::Text::detDimensions(string), ::Text::detDimensions(string))
    {}

    Text::Text(const std::string & string, const ivec2 & align, const vec4 & color, const ivec2 & minDimensions, const ivec2 & maxDimensions) :
        Single(::Text::detSize(minDimensions), ::Text::detSize(maxDimensions)),
        m_text(string, align, color)
    {}

    void Text::render() const {
        Component::render();
        m_text.renderWithin(m_position, m_size);
    }

    bool Text::string(const std::string & str) {
        string(std::string(str));
        return true;
    }

    bool Text::string(std::string && str) {
        m_text.string(move(str));
        return true;
    }

    void Text::align(const ivec2 & align) {
        m_text.align(align);
    }

    void Text::color(const vec4 & color) {
        m_text.color(color);
    }
    
    

    String::String(const std::string & initStr, int align, const vec4 & color) :
        String(initStr, align, color, ::Text::detDimensions(initStr).x, ::Text::detDimensions(initStr).x)
    {}

    String::String(const std::string & initStr, int align, const vec4 & color, int minWidth, int maxWidth) :
        Text("", ivec2(align, 0), color, ivec2(minWidth, 1), ivec2(maxWidth, 1))
    {
        string(initStr);
    }

    bool String::string(std::string && str) {
        for (char c : str) {
            if (!Util::isPrintable(c)) {
                return false;
            }
        }
        Text::string(move(str));
        return true;
    }
    


    double Number::detValue(const std::string & str) {
        double val;
        if (str.empty()) {
            val = 0.0f;
        }
        else {
            try {
                val = std::stod(str);
            } catch (const std::invalid_argument &) {
                val = std::numeric_limits<double>::quiet_NaN();
            }
        }
        return val;
    }

    Number::Number(double initVal, int align, const vec4 & color, int minWidth, int maxWidth, bool fixed, int precision) :
        String(Util::numberString(initVal, fixed, precision), align, color, minWidth, maxWidth),
        m_value(initVal),
        m_fixed(fixed),
        m_precision(precision)
    {}

    bool Number::string(std::string && str) {
        double val(detValue(str));
        if (std::isnan(val)) {
            return false;
        }
        value(val);
        return true;
    }

    void Number::value(double val) {
        m_value = val;
        Text::string(Util::numberString(val, m_fixed, m_precision));
    }

    

    Vector::Vector(const vec3 & initVal, const vec4 & color, int compWidth, bool fixed, int precision) :
        m_xNum(new Number(initVal.x, 0, color, compWidth, compWidth, fixed, precision)),
        m_yNum(new Number(initVal.y, 0, color, compWidth, compWidth, fixed, precision)),
        m_zNum(new Number(initVal.z, 0, color, compWidth, compWidth, fixed, precision))
    {
        add(shr<String>(new String("<", 0, color, 1, 1)));
        add(m_xNum);
        add(shr<String>(new String(" ", 0, color, 1, 1)));
        add(m_yNum);
        add(shr<String>(new String(" ", 0, color, 1, 1)));
        add(m_zNum);
        add(shr<String>(new String(">", 0, color, 1, 1)));
    }

    void Vector::value(const vec3 & val) {
        m_xNum->value(val.x);
        m_yNum->value(val.y);
        m_zNum->value(val.z);
    }

    vec3 Vector::value() const {
        return vec3(m_xNum->value(), m_yNum->value(), m_zNum->value());
    }

    

    TextField::TextField(const std::string & initStr, const ivec2 & align, const vec4 & color, const ivec2 & minDimensions, const ivec2 & maxDimensions) :
        Text("", align, color, minDimensions, maxDimensions),
        m_editing(false),
        m_savedString()
    {
        string(initStr);
    }

    bool TextField::string(std::string && string) {
        for (char c : string) {
            if (!valid(c)) {
                return false;
            }
        }

        if (m_editing) m_savedString = move(string);
        else Text::string(move(string));
        return true;
    }

    const std::string & TextField::string() const {
        return m_editing ? m_savedString : Text::string();
    }

    void TextField::keyEvent(int key, int action, int mods) {
        if (!m_editing) {
            return;
        }

        if (key == GLFW_KEY_ESCAPE) {
            if (action == GLFW_RELEASE && !mods) {
                cancelEditing();
                unfocus();
            }
        }
        else if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)) {
            if (action == GLFW_RELEASE && !mods) {
                doneEditing();
                unfocus();
            }
            else if ((action == GLFW_PRESS || action == GLFW_REPEAT) && (mods & GLFW_MOD_SHIFT)) {
                charEvent('\n');  
            }
        }
    }

    void TextField::charEvent(char c) {
        if (m_editing && valid(c)) Text::string(Text::string() + c);
    }
    
    void TextField::mouseButtonEvent(int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && !mods) {
            UI::focus(this);
        }
    }

    void TextField::focusEvent() {
        startEditing();
    }

    void TextField::unfocusEvent() {
        if (m_editing) cancelEditing();
    }

    bool TextField::valid(char c) const {
        return Util::isPrintable(c) || c == '\n';
    }

    void TextField::startEditing() {
        m_editing = true;
        m_savedString = Text::string();
        Text::string("");
        borderColor(color());
    }

    void TextField::doneEditing() {
        m_editing = false;
        if (!string(Text::string())) Text::string(move(m_savedString));
        borderColor(vec4());
    }

    void TextField::cancelEditing() {
        m_editing = false;
        Text::string(move(m_savedString));
        borderColor(vec4());
    }

    

    StringField::StringField(const std::string & initStr, int align, const vec4 & color, int minWidth, int maxWidth) :
        TextField(initStr, ivec2(align, 0), color, ivec2(minWidth, 1), ivec2(maxWidth, 1))
    {}

    bool StringField::valid(char c) const {
        return Util::isPrintable(c);
    }

    

    NumberField::NumberField(double initVal, int align, const vec4 & color, int minWidth, int maxWidth, bool fixed, int precision) :
        StringField(Util::numberString(initVal, fixed, precision), align, color, minWidth, maxWidth),
        m_value(initVal),
        m_fixed(fixed),
        m_precision(precision)
    {}

    bool NumberField::string(std::string && str) {
        double val(Number::detValue(str));
        if (std::isnan(val)) {
            return false;
        }
        value(val);
        return true;
    }

    void NumberField::value(double val) {
        m_value = val;
        TextField::string(Util::numberString(val, m_fixed, m_precision));
    }

    bool NumberField::valid(char c) const {
        return std::isdigit(c) || c == '.' || c == '-';
    }

}