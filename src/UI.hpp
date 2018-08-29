#pragma once



#include <vector>

#include "glad/glad.h"

#include "Global.hpp"



namespace UI {

    namespace detail {

        inline std::vector<int> distribute(int total, int n) {
            std::vector<int> vals(n, total / n);
            int excess(total % n);
            for (int i(0); i < excess; ++i) ++vals[i];
            return std::move(vals);
        }

    }

    class Component {

        protected:

        ivec2 m_size;
    
        public:

        virtual void update() {};

        virtual void render(const ivec2 & position) const = 0;

        virtual void pack(const ivec2 & size) = 0;

        virtual ivec2 minSize() const = 0;

        virtual ivec2 maxSize() const = 0;

        virtual int minWidth() const = 0;

        virtual int minHeight() const = 0;

        virtual int maxWidth() const = 0;

        virtual int maxHeight() const = 0;

    };

    class Single : public Component {

        protected:

        ivec2 m_minSize, m_maxSize;

        Single() :
            m_minSize(),
            m_maxSize()
        {}

        Single(const ivec2 & minSize, const ivec2 & maxSize) :
            m_minSize(minSize),
            m_maxSize(maxSize)
        {}

        public:

        virtual ivec2 minSize() const override { return m_minSize; }

        virtual ivec2 maxSize() const override { return m_maxSize; }

        virtual int minWidth() const override { return m_minSize.x;}

        virtual int minHeight() const override { return m_minSize.y; }

        virtual int maxWidth() const override { return m_maxSize.x; }

        virtual int maxHeight() const override { return m_maxSize.y; }

    };

    class Group : public Component {

        protected:

        struct SubComponent {

            shr<Component> component;
            ivec2 position;
            ivec2 tempSize;
            ivec2 tempMinSize;
            ivec2 tempMaxSize;

            SubComponent(shr<Component> component) :
                component(component)
            {}

        };
        
        std::vector<SubComponent> m_subComponents;

        public:

        virtual void render(const ivec2 & position) const override {
            for (const auto & comp : m_subComponents) {
                comp.component->render(position + comp.position);
            }
        }

        virtual ivec2 minSize() const override { return ivec2(minWidth(), minHeight()); }

        virtual ivec2 maxSize() const override { return ivec2(maxWidth(), maxHeight()); }

        virtual void add(shr<Component> component) {
            m_subComponents.emplace_back(component);
        }

        protected:

        virtual void setTempDimensions() {
            for (auto & subComp : m_subComponents) {
                subComp.tempSize = ivec2(0);
                subComp.tempMinSize = subComp.component->minSize();
                subComp.tempMaxSize = subComp.component->maxSize();
            }
        }

        virtual void packSubComponents() {
            for (auto & subComp : m_subComponents) {
                subComp.component->pack(subComp.tempSize);
            }
        }

    };

    class HorizGroup : public Group {

        public:

        virtual void pack(const ivec2 & size) override {
            setTempDimensions();

            int remainingWidth(size.x);
            for (auto & subComp : m_subComponents) {
                subComp.tempSize.x = subComp.tempMinSize.x;
                subComp.tempSize.y = size.y;
                remainingWidth -= subComp.tempSize.x;
            }

            bool wasChange;
            while (remainingWidth) {
                wasChange = false;
                for (auto & subComp : m_subComponents) {
                    if (subComp.tempSize.x < subComp.tempMaxSize.x || !subComp.tempMaxSize.x) {
                        ++subComp.tempSize.x;
                        --remainingWidth;
                        wasChange = true;
                        if (!remainingWidth) {
                            break;
                        }
                    }
                }
                if (!wasChange) {
                    break;
                }
            }

            ivec2 pos;
            for (auto & subComp : m_subComponents) {
                subComp.position = pos;
                pos.x += subComp.tempSize.x;
            }

            packSubComponents();
        }
    
        virtual int minWidth() const override {
            int minW(0);
            for (const auto & subComp : m_subComponents) {
                minW += subComp.component->minWidth();
            }
            return minW;
        }

        virtual int minHeight() const override {
            int minH(0);
            for (const auto & subComp : m_subComponents) {
                minH = glm::max(minH, subComp.component->minHeight());
            }
            return minH;
        }

        virtual int maxWidth() const override {
            int maxW(0);
            for (const auto & subComp : m_subComponents) {
                int w(subComp.component->maxWidth());
                if (!w) {
                    return 0;
                }
                maxW += w;
            }
            return maxW;
        }
    
        virtual int maxHeight() const override {
            int maxH(std::numeric_limits<int>::max());
            bool set(false);
            for (const auto & subComp : m_subComponents) {
                int h(subComp.component->maxHeight());
                if (h && h < maxH) {
                    maxH = h;
                    set = true;
                }
            }
            return set ? maxH : 0;
        }

    };

    class VertGroup : public Group {

        public:

        virtual void pack(const ivec2 & size) override {
            setTempDimensions();

            int remainingHeight(size.y);
            for (auto & subComp : m_subComponents) {
                subComp.tempSize.x = size.x;
                subComp.tempSize.y = subComp.tempMinSize.y;
                remainingHeight -= subComp.tempSize.y;
            }

            bool wasChange;
            while (remainingHeight) {
                wasChange = false;
                for (auto & subComp : m_subComponents) {
                    if (subComp.tempSize.y < subComp.tempMaxSize.y || !subComp.tempMaxSize.y) {
                        ++subComp.tempSize.y;
                        --remainingHeight;
                        wasChange = true;
                        if (!remainingHeight) {
                            break;
                        }
                    }
                }
                if (!wasChange) {
                    break;
                }
            }

            ivec2 pos;
            for (auto & subComp : m_subComponents) {
                subComp.position = pos;
                pos.y += subComp.tempSize.y;
            }

            packSubComponents();
        }

        virtual int minWidth() const override {
            int minW(0);
            for (const auto & subComp : m_subComponents) {
                minW = glm::max(minW, subComp.component->minWidth());
            }
            return minW;
        }
    
        virtual int minHeight() const override {
            int minH(0);
            for (const auto & subComp : m_subComponents) {
                minH += subComp.component->minHeight();
            }
            return minH;
        }
    
        virtual int maxWidth() const override {
            int maxW(std::numeric_limits<int>::max());
            bool set(false);
            for (const auto & subComp : m_subComponents) {
                int w(subComp.component->maxWidth());
                if (w && w < maxW) {
                    maxW = w;
                    set = true;
                }
            }
            return set ? maxW : 0;
        }

        virtual int maxHeight() const override {
            int maxH(0);
            for (const auto & subComp : m_subComponents) {
                int h(subComp.component->maxHeight());
                if (!h) {
                    return 0;
                }
                maxH += h;
            }
            return maxH;
        }

    };

}