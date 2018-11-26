#include "Text.hpp"

#include "Common/Util.hpp"



namespace ui {

    bool Text::setup() {
        return ::Text::setup();
    }

    Text::Text(const std::string & string, const ivec2 & align, const vec4 & color) :
        Text(string, align, color, ::Text::detDimensions(string), ::Text::detDimensions(string))
    {}

    Text::Text(const std::string & string, const ivec2 & align, const vec4 & color, const ivec2 & minDimensions, const ivec2 & maxDimensions) :
        Single(::Text::detSize(minDimensions), ::Text::detSize(maxDimensions)),
        m_text("", align, color)
    {
        std::string str(string);
        if (verify(str)) m_text.string(move(str));
    }

    void Text::render() const {
        Component::render();
        m_text.renderWithin(position(), size());
    }

    void Text::string(const std::string & str) {
        string(std::string(str));
    }

    void Text::string(std::string && str) {
        if (verify(str)) m_text.string(move(str));
    }

    void Text::align(const ivec2 & align) {
        m_text.align(align);
    }

    void Text::color(const vec4 & color) {
        m_text.color(color);
    }

    bool Text::verify(std::string & str) {
        for (char c : str) {
            if (!(util::isPrintable(c) || c == '\n')) {
                return false;
            }
        }
        return true;
    }

    void Text::reverify() {
        string(m_text.string(""));
    }



    String::String(const std::string & initStr, int align, const vec4 & color) :
        String(initStr, align, color, ::Text::detDimensions(initStr).x, ::Text::detDimensions(initStr).x)
    {}

    String::String(const std::string & initStr, int align, const vec4 & color, int minWidth, int maxWidth) :
        Text(initStr, ivec2(align, 0), color, ivec2(minWidth, 1), ivec2(maxWidth, 1))
    {}

    bool String::verify(std::string & str) {
        for (char c : str) {
            if (!util::isPrintable(c)) {
                return false;
            }
        }
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
                val = k_nan<double>;
            }
        }
        return val;
    }

    Number::Number(double initVal, int align, const vec4 & color, int minWidth, int maxWidth, bool fixed, int precision) :
        String(util::numberString(initVal, fixed, precision), align, color, minWidth, maxWidth),
        m_value(initVal),
        m_fixed(fixed),
        m_precision(precision)
    {}

    bool Number::verify(std::string & str) {
        double val(detValue(str));
        if (std::isnan(val)) {
            return false;
        }
        m_value = val;
        return true;
    }

    void Number::value(double val) {
        string(util::numberString(val, m_fixed, m_precision));
    }



    Vector::Vector(const vec3 & initVal, const vec4 & color, int compWidth, bool fixed, int precision) :
        m_x(new Number(initVal.x, 0, color, compWidth, compWidth, fixed, precision)),
        m_y(new Number(initVal.y, 0, color, compWidth, compWidth, fixed, precision)),
        m_z(new Number(initVal.z, 0, color, compWidth, compWidth, fixed, precision))
    {
        add(shr<String>(new String("<", 0, color, 1, 1)));
        add(m_x);
        add(shr<String>(new String(" ", 0, color, 1, 1)));
        add(m_y);
        add(shr<String>(new String(" ", 0, color, 1, 1)));
        add(m_z);
        add(shr<String>(new String(">", 0, color, 1, 1)));
    }

    void Vector::value(const vec3 & val) {
        m_x->value(val.x);
        m_y->value(val.y);
        m_z->value(val.z);
    }

    vec3 Vector::value() const {
        return vec3(m_x->value(), m_y->value(), m_z->value());
    }

}