#include "Group.hpp"



namespace ui {

    void HorizontalGroup::pack() {
        std::vector<int> tempWidths(m_components.size());

        int remainingWidth(size().x);
        for (int i(0); i < int(m_components.size()); ++i) {
            tempWidths[i] = m_components[i]->minSize().x;
            remainingWidth -= tempWidths[i];
        }

        bool wasChange;
        while (remainingWidth > 0) {
            wasChange = false;
            for (int i(0); i < int(m_components.size()); ++i) {
                if (tempWidths[i] < m_components[i]->maxSize().x || !m_components[i]->maxSize().x) {
                    ++tempWidths[i];
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

        ivec2 pos(position());
        for (int i(0); i < int(m_components.size()); ++i) {
            m_components[i]->position(pos);
            m_components[i]->size(ivec2(tempWidths[i], size().y));
            pos.x += tempWidths[i];
        }

        packComponents();
    }

    duo<ivec2> HorizontalGroup::detSizeExtrema() const {
        ivec2 minSize(0, 0), maxSize(0, std::numeric_limits<int>::max());
        bool isMaxWidth(true);

        for (const auto & comp : m_components) {
            minSize.x += comp->minSize().x;

            if (comp->minSize().y > minSize.y) minSize.y = comp->minSize().y;

            if (comp->maxSize().x) maxSize.x += comp->maxSize().x;
            else isMaxWidth = false;

            if (comp->maxSize().y && comp->maxSize().y < maxSize.y) maxSize.y = comp->maxSize().y;
        }

        if (!isMaxWidth) maxSize.x = 0;
        if (maxSize.y == std::numeric_limits<int>::max()) maxSize.y = 0;

        return { minSize, maxSize };
    }



    void VerticalGroup::pack() {
        std::vector<int> tempHeights(m_components.size());

        int remainingHeight(size().y);
        for (int i(0); i < int(m_components.size()); ++i) {
            tempHeights[i] = m_components[i]->minSize().y;
            remainingHeight -= tempHeights[i];
        }

        bool wasChange;
        while (remainingHeight > 0) {
            wasChange = false;
            for (int i(0); i < int(m_components.size()); ++i) {
                if (tempHeights[i] < m_components[i]->maxSize().y || !m_components[i]->maxSize().y) {
                    ++tempHeights[i];
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

        ivec2 pos(position().x, position().y + size().y);
        for (int i(0); i < int(m_components.size()); ++i) {
            pos.y -= tempHeights[i];
            m_components[i]->position(pos);
            m_components[i]->size(ivec2(size().x, tempHeights[i]));
        }

        packComponents();
    }

    duo<ivec2> VerticalGroup::detSizeExtrema() const {
        ivec2 minSize(0, 0), maxSize(std::numeric_limits<int>::max(), 0);
        bool isMaxHeight(true);

        for (const auto & comp : m_components) {
            if (comp->minSize().x > minSize.x) minSize.x = comp->minSize().x;

            minSize.y += comp->minSize().y;

            if (comp->maxSize().x && comp->maxSize().x < maxSize.x) maxSize.x = comp->maxSize().x;

            if (comp->maxSize().y) maxSize.y += comp->maxSize().y;
            else isMaxHeight = false;
        }

        if (maxSize.x == std::numeric_limits<int>::max()) maxSize.x = 0;
        if (!isMaxHeight) maxSize.y = 0;

        return { minSize, maxSize };
    }



    void LayerGroup::pack() {
        for (auto & comp : m_components) {
            comp->size(size());
            comp->position(position());
        }

        packComponents();
    }

    duo<ivec2> LayerGroup::detSizeExtrema() const {
        ivec2 minSize(std::numeric_limits<int>::max()), maxSize(0);

        for (const auto & comp : m_components) {
            ivec2 compMinSize(comp->minSize());
            if (compMinSize.x && compMinSize.x < minSize.x) minSize.x = compMinSize.x;
            if (compMinSize.y && compMinSize.y < minSize.y) minSize.y = compMinSize.y;

            maxSize = glm::max(maxSize, comp->maxSize());
        }

        if (maxSize.x == std::numeric_limits<int>::max()) maxSize.x = 0;
        if (maxSize.y == std::numeric_limits<int>::max()) maxSize.y = 0;

        return { minSize, maxSize };
    }

}