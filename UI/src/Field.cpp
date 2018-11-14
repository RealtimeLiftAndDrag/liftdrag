#include "Field.hpp"

#include <cctype>

#include "GLFW/glfw3.h"

#include "Common/Util.hpp"



namespace ui {

    TextField::TextField(const std::string & initStr, const ivec2 & align, const vec4 & color, const ivec2 & minDimensions, const ivec2 & maxDimensions) :
        Text("", align, color, minDimensions, maxDimensions),
        m_editing(false),
        m_savedString()
    {
        string(initStr);
    }

    void TextField::string(std::string && string) {
        if (m_editing) {
            if (verify(string)) m_savedString = move(string);
        }
        else {
            Text::string(move(string));
        }
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
        if (m_editing && valid(c)) m_text.string(m_text.string() + c);
    }

    void TextField::mouseButtonEvent(int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && !mods) {
            ui::focus(this);
        }
    }

    void TextField::focusEvent() {
        startEditing();
    }

    void TextField::unfocusEvent() {
        if (m_editing) cancelEditing();
    }

    void TextField::actionCallback(void (*callback)(void)) {
        m_actionCallback = callback;
    }

    bool TextField::valid(char c) const {
        return util::isPrintable(c) || c == '\n';
    }

    void TextField::startEditing() {
        m_editing = true;
        borderColor(color());
        m_savedString = m_text.string("");
    }

    void TextField::doneEditing() {
        m_editing = false;
        borderColor(vec4());
        string(m_text.string(""));
        if (m_actionCallback) m_actionCallback();
    }

    void TextField::cancelEditing() {
        m_editing = false;
        borderColor(vec4());
        m_text.string(move(m_savedString));
    }



    StringField::StringField(const std::string & initStr, int align, const vec4 & color, int minWidth, int maxWidth) :
        TextField(initStr, ivec2(align, 0), color, ivec2(minWidth, 1), ivec2(maxWidth, 1))
    {}

    bool StringField::verify(std::string & str) {
        for (char c : str) {
            if (!util::isPrintable(c)) {
                return false;
            }
        }
        return true;
    }

    bool StringField::valid(char c) const {
        return util::isPrintable(c);
    }



    NumberField::NumberField(double initVal, int align, const vec4 & color, int minWidth, int maxWidth, bool fixed, int precision) :
        StringField(util::numberString(initVal, fixed, precision), align, color, minWidth, maxWidth),
        m_value(initVal),
        m_fixed(fixed),
        m_precision(precision)
    {}

    bool NumberField::verify(std::string & str) {
        double val(Number::detValue(str));
        if (std::isnan(val)) {
            return false;
        }
        m_value = val;
        return true;
    }

    void NumberField::value(double val) {
        string(util::numberString(val, m_fixed, m_precision));
    }

    bool NumberField::valid(char c) const {
        return std::isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E';
    }



    BoundedNumberField::BoundedNumberField(double initVal, int align, const vec4 & color, int minWidth, int maxWidth, bool fixed, int precision, double minVal, double maxVal) :
        NumberField(initVal, align, color, minWidth, maxWidth, fixed, precision),
        m_minVal(minVal),
        m_maxVal(maxVal)
    {}

    bool BoundedNumberField::verify(std::string & str) {
        double val(Number::detValue(str));
        if (std::isnan(val)) {
            return false;
        }
        if (val < m_minVal) {
            val = m_minVal;
            str = util::numberString(val, m_fixed, m_precision);
        }
        else if (val > m_maxVal) {
            val = m_maxVal;
            str = util::numberString(val, m_fixed, m_precision);
        }
        m_value = val;
        return true;
    }

    void BoundedNumberField::minVal(double minVal) {
        m_minVal = minVal;
        reverify();
    }

    void BoundedNumberField::maxVal(double maxVal) {
        m_maxVal = maxVal;
        reverify();
    }



    VectorField::VectorField(const vec3 & initVal, const vec4 & color, int compWidth, bool fixed, int precision) :
        m_x(new NumberField(initVal.x, 0, color, compWidth, compWidth, fixed, precision)),
        m_y(new NumberField(initVal.y, 0, color, compWidth, compWidth, fixed, precision)),
        m_z(new NumberField(initVal.z, 0, color, compWidth, compWidth, fixed, precision))
    {
        add(shr<String>(new String("<", 0, color, 1, 1)));
        add(m_x);
        add(shr<String>(new String(" ", 0, color, 1, 1)));
        add(m_y);
        add(shr<String>(new String(" ", 0, color, 1, 1)));
        add(m_z);
        add(shr<String>(new String(">", 0, color, 1, 1)));
    }

    void VectorField::value(const vec3 & val) {
        m_x->value(val.x);
        m_y->value(val.y);
        m_z->value(val.z);
    }

    vec3 VectorField::value() const {
        return vec3(m_x->value(), m_y->value(), m_z->value());
    }

}