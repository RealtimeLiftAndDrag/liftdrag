#pragma once



#include "Text.hpp"



namespace ui {    

    class TextField : public Text {

        private:

        bool m_editing;
        std::string m_savedString;
        void (*m_actionCallback)(void);

        public:

        TextField(const std::string & initStr, const ivec2 & align, const vec4 & color, const ivec2 & minDimensions, const ivec2 & maxDimensions);

        virtual void string(std::string && string) override;
        virtual const std::string & string() const override;
        using Text::string;

        virtual void keyEvent(int key, int action, int mods) override;

        virtual void charEvent(char c) override;

        virtual void mouseButtonEvent(int button, int action, int mods) override;

        virtual void focusEvent() override;

        virtual void unfocusEvent() override;

        virtual void actionCallback(void (*callback)(void)) final;

        virtual bool valid(char c) const;

        private:

        void startEditing();

        void doneEditing();

        void cancelEditing();

    };

    class StringField : public TextField {

        public:

        StringField(const std::string & initStr, int align, const vec4 & color, int minWidth, int maxWidth);

        virtual bool verify(std::string & string) override;

        virtual bool valid(char c) const override;

    };

    class NumberField : public StringField {

        protected:

        double m_value;
        bool m_fixed;
        int m_precision;

        public:

        NumberField(double initVal, int align, const vec4 & color, int minWidth, int maxWidth, bool fixed, int precision);

        virtual bool verify(std::string & string) override;

        virtual bool valid(char c) const override;

        virtual double value() const final { return m_value; }
        virtual void value(double val);

    };

    class BoundedNumberField : public NumberField {

        protected:

        double m_minVal, m_maxVal;

        public:

        BoundedNumberField(double initVal, int align, const vec4 & color, int minWidth, int maxWidth, bool fixed, int precision, double minVal, double maxVal);

        virtual bool verify(std::string & string) override;

        virtual double minVal() const final { return m_minVal; }
        virtual double maxVal() const final { return m_maxVal; }
        virtual void minVal(double minVal);
        virtual void maxVal(double maxVal);

    };

    class VectorField : public HorizontalGroup {

        protected:

        shr<NumberField> m_x, m_y, m_z;

        public:

        VectorField(const vec3 & initVal, const vec4 & color, int compWidth, bool fixed, int precision);

        virtual void value(const vec3 & val);
        virtual vec3 value() const final;

    };

}