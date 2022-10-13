/*
 * Copyright (c) 2010-2022 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include "creature.h"
#include "declarations.h"
#include <framework/ui/uiwidget.h>

class UICreature : public UIWidget
{
public:
    void drawSelf(Fw::DrawPane drawPane) override;

    void setCreature(const CreaturePtr& creature) { m_creature = creature; }
    void setFixedCreatureSize(bool fixed) { m_fixedCreatureSize = fixed; }
    void setOutfit(const Outfit& outfit);

    CreaturePtr getCreature() { return m_creature; }
    Outfit getOutfit() { return m_creature ? m_creature->getOutfit() : Outfit(); }

    bool isFixedCreatureSize() { return m_fixedCreatureSize; }

    void setAutoRotating(bool value) { m_autoRotating = value; }
    void setDirection(Otc::Direction direction) { m_direction = direction; }
    Otc::Direction getDirection() { return m_direction; }
    void setAnimate(bool value) { m_animate = value; }
    bool isAnimating() { return m_animate; }
    void setCenter(bool value);
    void setOldScaling(bool value) { m_oldScaling = value; }

protected:
    void onStyleApply(const std::string_view styleName, const OTMLNodePtr& styleNode) override;
    bool m_autoRotating{ false };
    bool m_animate{ false };
    bool m_oldScaling{ false };
    Otc::Direction m_direction = Otc::South;
    CreaturePtr m_creature;
    bool m_fixedCreatureSize{ false };
};
