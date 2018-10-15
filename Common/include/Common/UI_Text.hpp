#pragma once



#include "UI.hpp"
#include "Text.hpp"



namespace UI {

    class Text : public Single {

        public:

        static bool setup(const std::string & resourceDir);

        static const ivec2 & fontSize() { return ::Text::fontSize(); }

        protected:

        ::Text m_text;

        public:
        
        Text(const std::string & string, const ivec2 & align, const vec4 & color);
        Text(const std::string & string, const ivec2 & align, const vec4 & color, const ivec2 & minDimensions, const ivec2 & maxDimensions);

        virtual void render() const override;

        virtual void string(const std::string & str);
        virtual void string(std::string && string);
        virtual const std::string & string() const { return m_text.string(); }

        virtual void align(const ivec2 & align);
        virtual const ivec2 & align() const { return m_text.align(); }

        virtual void color(const vec4 & color);
        virtual const vec4 & color() const { return m_text.color(); }

        virtual bool verify(std::string & str);

    };

    class String : public Text {    

        public:
        
        String(const std::string & initStr, int align, const vec4 & color);
        String(const std::string & initStr, int align, const vec4 & color, int minWidth, int maxWidth);

        virtual bool verify(std::string & string) override;

    };

    class Number : public String {

        public:

        static double detValue(const std::string & str);
    
        protected:

        double m_value;
        bool m_fixed;
        int m_precision;

        public:

        Number(double initVal, int align, const vec4 & color, int minWidth, int maxWidth, bool fixed, int precision);        

        virtual bool verify(std::string & string) override;

        virtual void value(double val);
        virtual double value() const final { return m_value; }
    
    };

    class Vector : public HorizontalGroup {
    
        protected:

        shr<Number> m_x, m_y, m_z;

        public:

        Vector(const vec3 & initVal, const vec4 & color, int compWidth, bool fixed, int precision);

        virtual void value(const vec3 & val);
        virtual vec3 value() const final;
    
    };

    class TextField : public Text {

        private:

        bool m_editing;
        std::string m_savedString;

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

    class VectorField : public HorizontalGroup {
    
        protected:

        shr<NumberField> m_x, m_y, m_z;

        public:

        VectorField(const vec3 & initVal, const vec4 & color, int compWidth, bool fixed, int precision);

        virtual void value(const vec3 & val);
        virtual vec3 value() const final;
    
    };

}