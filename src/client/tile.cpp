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

#include "tile.h"
#include "effect.h"
#include "game.h"
#include "item.h"
#include "lightview.h"
#include "map.h"
#include "protocolgame.h"
#include <framework/core/eventdispatcher.h>
#include <framework/graphics/drawpoolmanager.h>

Tile::Tile(const Position& position) : m_position(position)
{
    m_completelyCoveredCache.fill(-1);
}

void Tile::drawThing(const ThingPtr& thing, const Point& dest, float scaleFactor, int flags, LightView* lightView, bool animate)
{
    thing->draw(dest, scaleFactor, animate, flags, TextureType::NONE, m_selectType != TileSelectType::NONE && m_highlightThing == thing, lightView);

    if (thing->isItem()) {
        m_drawElevation += thing->getElevation();
        if (m_drawElevation > MAX_ELEVATION)
            m_drawElevation = MAX_ELEVATION;
    }
}

void Tile::draw(const Point& dest, const MapPosInfo& mapRect, float scaleFactor, int flags, bool isCovered, LightView* lightView)
{
    m_drawElevation = 0;
    m_lastDrawDest = dest;

    for (const auto& thing : m_things) {
        if (!thing->isGround() && !thing->isGroundBorder())
            break;

        drawThing(thing, dest - m_drawElevation * scaleFactor, scaleFactor, flags, lightView);
    }

    if (m_countFlag.hasBottomItem) {
        for (const auto& item : m_things) {
            if (!item->isOnBottom()) continue;
            drawThing(item, dest - m_drawElevation * scaleFactor, scaleFactor, flags, lightView);
        }
    }

    if (m_countFlag.hasCommonItem) {
        for (auto it = m_things.rbegin(); it != m_things.rend(); ++it) {
            const auto& item = *it;
            if (!item->isCommon()) continue;
            drawThing(item, dest - m_drawElevation * scaleFactor, scaleFactor, flags, lightView);
        }
    }

    // after we render 2x2 lying corpses, we must redraw previous creatures/ontop above them
    for (const auto& tile : m_tilesRedraw) {
        tile->drawCreature(tile->m_lastDrawDest, mapRect, scaleFactor, flags, isCovered, true, lightView);
        tile->drawTop(tile->m_lastDrawDest, scaleFactor, flags, true, lightView);
    }

    // this true value in drawCreature fixes light flickering (turning on and off)
    drawCreature(dest, mapRect, scaleFactor, flags, isCovered, false, lightView);
    drawTop(dest, scaleFactor, flags, false, lightView);
}

void Tile::drawCreature(const Point& dest, const MapPosInfo& mapRect, float scaleFactor, int flags, bool isCovered, bool forceDraw, LightView* lightView)
{
    if (!forceDraw && !m_drawTopAndCreature)
        return;

    if (hasCreature()) {
        for (const auto& thing : m_things) {
            if (!thing->isCreature() || thing->static_self_cast<Creature>()->isWalking()) continue;

            const Point& cDest = dest - m_drawElevation * scaleFactor;
            drawThing(thing, cDest, scaleFactor, flags, lightView);
            thing->static_self_cast<Creature>()->drawInformation(mapRect, cDest, scaleFactor, isCovered, flags);
        }
    }

    for (const auto& creature : m_walkingCreatures) {
        const auto& cDest = Point(
            dest.x + ((creature->getPosition().x - m_position.x) * SPRITE_SIZE - m_drawElevation) * scaleFactor,
            dest.y + ((creature->getPosition().y - m_position.y) * SPRITE_SIZE - m_drawElevation) * scaleFactor
        );
        drawThing(creature, cDest, scaleFactor, flags, lightView);
        creature->drawInformation(mapRect, cDest, scaleFactor, isCovered, flags);
    }
}

void Tile::drawTop(const Point& dest, float scaleFactor, int flags, bool forceDraw, LightView* lightView)
{
    if (!forceDraw && !m_drawTopAndCreature)
        return;

    if (hasEffect()) {
        int offsetX = 0,
            offsetY = 0;

        if (g_game.getFeature(Otc::GameMapOldEffectRendering)) {
            offsetX = m_position.x - g_map.getCentralPosition().x;
            offsetX = m_position.y - g_map.getCentralPosition().y;
        }

        for (const auto& effect : m_effects) {
            effect->drawEffect(dest - m_drawElevation * scaleFactor, scaleFactor, flags, offsetX, offsetY, lightView);
        }
    }

    if (m_countFlag.hasTopItem) {
        for (const auto& item : m_things) {
            if (!item->isOnTop()) continue;
            drawThing(item, dest, scaleFactor, flags, lightView);
        }
    }
}

void Tile::clean()
{
    while (!m_things.empty())
        removeThing(m_things.front());
    m_tilesRedraw.clear();
    m_countFlag.opaque = 0;
}

void Tile::addWalkingCreature(const CreaturePtr& creature)
{
    m_walkingCreatures.push_back(creature);
    analyzeThing(creature, true);
}

void Tile::removeWalkingCreature(const CreaturePtr& creature)
{
    const auto it = std::find(m_walkingCreatures.begin(), m_walkingCreatures.end(), creature);
    if (it != m_walkingCreatures.end()) {
        analyzeThing(creature, false);
        m_walkingCreatures.erase(it);
    }
}

// TODO: Need refactoring
// Redo Stack Position System
void Tile::addThing(const ThingPtr& thing, int stackPos)
{
    if (!thing)
        return;

    if (thing->isEffect()) {
        const EffectPtr& newEffect = thing->static_self_cast<Effect>();

        const bool mustOptimize = g_app.mustOptimize() || g_app.isForcedEffectOptimization();

        for (const EffectPtr& prevEffect : m_effects) {
            if (!prevEffect->canDraw())
                continue;

            if (mustOptimize && newEffect->getSize() > prevEffect->getSize()) {
                prevEffect->canDraw(false);
            } else if (mustOptimize || newEffect->getId() == prevEffect->getId()) {
                newEffect->waitFor(prevEffect);
            }
        }

        if (newEffect->isTopEffect())
            m_effects.insert(m_effects.begin(), newEffect);
        else
            m_effects.push_back(newEffect);

        analyzeThing(thing, true);

        thing->setPosition(m_position);
        thing->onAppear();
        return;
    }

    const uint8_t size = m_things.size();

    // priority                                    854
    // 0 - ground,                        -->      -->
    // 1 - ground borders                 -->      -->
    // 2 - bottom (walls),                -->      -->
    // 3 - on top (doors)                 -->      -->
    // 4 - creatures, from top to bottom  <--      -->
    // 5 - items, from top to bottom      <--      <--
    if (stackPos < 0 || stackPos == 255) {
        const int priority = thing->getStackPriority();

        // -1 or 255 => auto detect position
        // -2        => append

        bool append;
        if (stackPos == -2)
            append = true;
        else {
            append = priority <= 3;

            // newer protocols does not store creatures in reverse order
            if (g_game.getClientVersion() >= 854 && priority == 4)
                append = !append;
        }

        for (stackPos = 0; stackPos < size; ++stackPos) {
            const int otherPriority = m_things[stackPos]->getStackPriority();
            if ((append && otherPriority > priority) || (!append && otherPriority >= priority))
                break;
        }
    } else if (stackPos > static_cast<int>(size))
        stackPos = size;

    // common items are drawn from back to front, so if the item being added now is common and has elevation,
    // look for any item behind it and destroy its buffer.
    if (m_countFlag.hasCommonItem && thing->isCommon() && thing->hasElevation()) {
        for (auto it = m_things.rbegin(); it != m_things.rend(); ++it) {
            if (const auto& item = *it; item->isCommon())
                item->destroyBuffer();
        }
    }

    m_things.insert(m_things.begin() + stackPos, thing);

    // get the elevation status before analyze the new item.
    const bool hasElev = hasElevation();

    analyzeThing(thing, true);
    checkForDetachableThing();

    if (size > MAX_THINGS)
        removeThing(m_things[MAX_THINGS]);

    // Do not change if you do not understand what is being done.
    {
        if (m_ground) {
            --stackPos;
            if (m_ground->isTopGround()) {
                m_ground->destroyBuffer();
                thing->destroyBuffer();
            }
        } else if (thing->isGround())
            m_ground = thing->static_self_cast<Item>();
    }

    thing->setPosition(m_position, stackPos, hasElev);
    thing->onAppear();

    if (thing->isTranslucent())
        checkTranslucentLight();
}

// TODO: Need refactoring
bool Tile::removeThing(const ThingPtr thing)
{
    if (!thing) return false;

    if (thing->isEffect()) {
        const EffectPtr& effect = thing->static_self_cast<Effect>();
        const auto it = std::find(m_effects.begin(), m_effects.end(), effect);
        if (it == m_effects.end())
            return false;

        analyzeThing(thing, false);

        m_effects.erase(it);
        return true;
    }

    const auto it = std::find(m_things.begin(), m_things.end(), thing);
    if (it == m_things.end())
        return false;

    if (thing->isGround()) m_ground = nullptr;

    analyzeThing(thing, false);

    m_things.erase(it);

    checkForDetachableThing();

    thing->onDisappear();

    if (thing->isTranslucent())
        checkTranslucentLight();

    return true;
}

ThingPtr Tile::getThing(int stackPos)
{
    if (stackPos >= 0 && stackPos < static_cast<int>(m_things.size()))
        return m_things[stackPos];

    return nullptr;
}

std::vector<CreaturePtr> Tile::getCreatures()
{
    std::vector<CreaturePtr> creatures;
    if (hasCreature()) {
        for (const ThingPtr& thing : m_things) {
            if (thing->isCreature())
                creatures.push_back(thing->static_self_cast<Creature>());
        }
    }

    return creatures;
}

int Tile::getThingStackPos(const ThingPtr& thing)
{
    for (int stackpos = -1, s = m_things.size(); ++stackpos < s;) {
        if (thing == m_things[stackpos]) return stackpos;
    }

    return -1;
}

bool Tile::hasThing(const ThingPtr& thing)
{
    return std::find(m_things.begin(), m_things.end(), thing) != m_things.end();
}

ThingPtr Tile::getTopThing()
{
    if (isEmpty())
        return nullptr;

    for (const ThingPtr& thing : m_things)
        if (thing->isCommon())
            return thing;

    return m_things[m_things.size() - 1];
}

std::vector<ItemPtr> Tile::getItems()
{
    std::vector<ItemPtr> items;
    for (const ThingPtr& thing : m_things) {
        if (!thing->isItem())
            continue;

        items.push_back(thing->static_self_cast<Item>());
    }
    return items;
}

EffectPtr Tile::getEffect(uint16_t id)
{
    for (const EffectPtr& effect : m_effects)
        if (effect->getId() == id)
            return effect;

    return nullptr;
}

int Tile::getGroundSpeed()
{
    if (const ItemPtr& ground = getGround())
        return ground->getGroundSpeed();

    return 100;
}

uint8_t Tile::getMinimapColorByte()
{
    if (m_minimapColor != 0)
        return m_minimapColor;

    for (auto it = m_things.rbegin(); it != m_things.rend(); ++it) {
        const auto& thing = *it;
        if (thing->isCreature() || thing->isCommon())
            continue;

        const uint8_t c = thing->getMinimapColor();
        if (c != 0) return c;
    }

    return 255;
}

ThingPtr Tile::getTopLookThing()
{
    if (isEmpty())
        return nullptr;

    for (const auto& thing : m_things) {
        if (!thing->isIgnoreLook() && (!thing->isGround() && !thing->isGroundBorder() && !thing->isOnBottom() && !thing->isOnTop()))
            return thing;
    }

    return m_things[0];
}

ThingPtr Tile::getTopUseThing()
{
    if (isEmpty())
        return nullptr;

    for (const auto& thing : m_things) {
        if (thing->isForceUse() || (!thing->isGround() && !thing->isGroundBorder() && !thing->isOnBottom() && !thing->isOnTop() && !thing->isCreature() && !thing->isSplash()))
            return thing;
    }

    for (const auto& thing : m_things) {
        if (!thing->isGround() && !thing->isGroundBorder() && !thing->isCreature() && !thing->isSplash())
            return thing;
    }

    return m_things[0];
}

CreaturePtr Tile::getTopCreature(const bool checkAround)
{
    if (!hasCreature()) return nullptr;

    CreaturePtr creature;
    for (const auto& thing : m_things) {
        if (thing->isLocalPlayer()) // return local player if there is no other creature
            creature = thing->static_self_cast<Creature>();
        else if (thing->isCreature())
            return thing->static_self_cast<Creature>();
    }

    if (creature)
        return creature;

    if (!m_walkingCreatures.empty())
        return m_walkingCreatures.back();

    // check for walking creatures in tiles around
    if (checkAround) {
        for (const auto& pos : m_position.getPositionsAround()) {
            const TilePtr& tile = g_map.getTile(pos);
            if (!tile) continue;

            for (const CreaturePtr& c : tile->getCreatures()) {
                if (c->isWalking() && c->getLastStepFromPosition() == m_position && c->getStepProgress() < .75f) {
                    return c;
                }
            }
        }
    }

    return nullptr;
}

ThingPtr Tile::getTopMoveThing()
{
    if (isEmpty())
        return nullptr;

    for (int8_t i = -1, s = m_things.size(); ++i < s;) {
        const ThingPtr& thing = m_things[i];
        if (thing->isCommon()) {
            if (i > 0 && thing->isNotMoveable())
                return m_things[i - 1];

            return thing;
        }
    }

    for (const ThingPtr& thing : m_things) {
        if (thing->isCreature())
            return thing;
    }

    return m_things[0];
}

ThingPtr Tile::getTopMultiUseThing()
{
    if (isEmpty())
        return nullptr;

    if (const CreaturePtr& topCreature = getTopCreature())
        return topCreature;

    for (const auto& thing : m_things) {
        if (thing->isForceUse())
            return thing;
    }

    for (int8_t i = -1, s = m_things.size(); ++i < s;) {
        const ThingPtr& thing = m_things[i];
        if (!thing->isGround() && !thing->isGroundBorder() && !thing->isOnBottom() && !thing->isOnTop()) {
            if (i > 0 && thing->isSplash())
                return m_things[i - 1];

            return thing;
        }
    }

    for (const auto& thing : m_things) {
        if (!thing->isGround() && !thing->isOnTop())
            return thing;
    }

    return m_things[0];
}

bool Tile::isWalkable(bool ignoreCreatures)
{
    if (m_countFlag.notWalkable > 0 || !getGround()) {
        return false;
    }

    if (!ignoreCreatures && hasCreature()) {
        for (const auto& thing : m_things) {
            if (!thing->isCreature()) continue;

            const auto& creature = thing->static_self_cast<Creature>();
            if (!creature->isPassable() && creature->canBeSeen())
                return false;
        }
    }

    return true;
}

bool Tile::isCompletelyCovered(uint8_t firstFloor, bool resetCache)
{
    if (resetCache)
        m_completelyCoveredCache.fill(-1);

    if (hasCreature() || !m_walkingCreatures.empty() || hasLight())
        return false;

    auto& is = m_completelyCoveredCache[firstFloor];
    if (is == -1) {
        is = g_map.isCompletelyCovered(m_position, firstFloor);
    }

    return is == 1;
}

bool Tile::isCovered(int8_t firstFloor)
{
    return firstFloor == m_lastFloorMin ? m_covered :
        m_covered = g_map.isCovered(m_position, m_lastFloorMin = firstFloor);
}

bool Tile::isClickable()
{
    bool hasGround = false;
    bool hasOnBottom = false;
    bool hasIgnoreLook = false;
    for (const ThingPtr& thing : m_things) {
        if (thing->isGround())
            hasGround = true;
        else if (thing->isOnBottom())
            hasOnBottom = true;

        if (thing->isIgnoreLook())
            hasIgnoreLook = true;

        if ((hasGround || hasOnBottom) && !hasIgnoreLook)
            return true;
    }

    return false;
}

void Tile::onAddInMapView()
{
    m_drawTopAndCreature = true;
    m_tilesRedraw.clear();

    if (m_countFlag.correctCorpse) {
        uint8_t redrawPreviousTopW = 0,
            redrawPreviousTopH = 0;

        for (const auto& item : m_things) {
            if (!item->isLyingCorpse()) continue;

            redrawPreviousTopW = std::max<int>(item->getWidth() - 1, redrawPreviousTopW);
            redrawPreviousTopH = std::max<int>(item->getHeight() - 1, redrawPreviousTopH);
        }

        for (int x = -redrawPreviousTopW; x <= 0; ++x) {
            for (int y = -redrawPreviousTopH; y <= 0; ++y) {
                if (x == 0 && y == 0)
                    continue;

                const TilePtr& tile = g_map.getTile(m_position.translated(x, y));
                if (tile && (tile->hasCreature() || tile->hasEffect() || tile->hasTopItem())) {
                    tile->m_drawTopAndCreature = false;
                    m_tilesRedraw.push_back(tile);
                }
            }
        }
    }
}

bool Tile::canShade(const MapViewPtr& mapView)
{
    for (const auto dir : { Otc::North, Otc::NorthWest, Otc::West }) {
        const auto& pos = m_position.translatedToDirection(dir);
        const auto& tile = g_map.getTile(pos);

        if ((!tile && mapView->isInRangeEx(pos, true)) || (tile && !tile->isFullyOpaque() && !tile->isFullGround() && !tile->hasTopGround(true)))
            return false;
    }

    return isFullyOpaque() || hasTopGround(true) || isFullGround();
}

bool Tile::hasBlockingCreature()
{
    for (const ThingPtr& thing : m_things)
        if (thing->isCreature() && !thing->static_self_cast<Creature>()->isPassable() && !thing->isLocalPlayer())
            return true;
    return false;
}

bool Tile::limitsFloorsView(bool isFreeView)
{
    // ground and walls limits the view
    const ThingPtr& firstThing = getThing(0);
    return firstThing && (firstThing->isGround() || (isFreeView ? firstThing->isOnBottom() : firstThing->isOnBottom() && firstThing->blockProjectile()));
}

void Tile::checkTranslucentLight()
{
    if (m_position.z != SEA_FLOOR)
        return;

    Position downPos = m_position;
    if (!downPos.down()) return;

    const auto& tile = g_map.getOrCreateTile(downPos);
    if (!tile)
        return;

    for (const auto& thing : m_things) {
        if (thing->isTranslucent() || thing->hasLensHelp()) {
            tile->m_flags |= TILESTATE_TRANSLUECENT_LIGHT;
            return;
        }
    }

    tile->m_flags &= ~TILESTATE_TRANSLUECENT_LIGHT;
}

bool Tile::checkForDetachableThing()
{
    if ((m_highlightThing = getTopCreature()))
        return true;

    if (m_countFlag.hasCommonItem) {
        for (const auto& item : m_things) {
            if ((!item->isCommon() || !item->canDraw() || item->isIgnoreLook() || item->isCloth()) && (!item->isUsable()) && (!item->hasLight())) {
                continue;
            }

            m_highlightThing = item;
            return true;
        }
    }

    if (m_countFlag.hasBottomItem) {
        for (auto it = m_things.rbegin(); it != m_things.rend(); ++it) {
            const auto& item = *it;
            if (!item->isOnBottom() || !item->canDraw() || item->isIgnoreLook() || item->isFluidContainer()) continue;
            m_highlightThing = item;
            return true;
        }
    }

    if (m_countFlag.hasTopItem) {
        for (auto it = m_things.rbegin(); it != m_things.rend(); ++it) {
            const auto& item = *it;
            if (!item->isOnTop()) break;
            if (!item->canDraw() || item->isIgnoreLook()) continue;

            if (item->hasLensHelp()) {
                m_highlightThing = item;
                return true;
            }
        }
    }

    return false;
}

void Tile::analyzeThing(const ThingPtr& thing, bool add)
{
    const int value = add ? 1 : -1;

    if (thing->hasLight())
        m_countFlag.hasLight += value;

    if (thing->hasDisplacement())
        m_countFlag.hasDisplacement += value;

    if (thing->isEffect()) return;

    if (thing->isCommon())
        m_countFlag.hasCommonItem += value;

    if (thing->isOnTop())
        m_countFlag.hasTopItem += value;

    if (thing->isCreature())
        m_countFlag.hasCreature += value;

    if (thing->isSingleGroundBorder())
        m_countFlag.hasGroundBorder += value;

    if (thing->isTopGroundBorder())
        m_countFlag.hasTopGroundBorder += value;

    if (thing->isLyingCorpse() && !g_game.getFeature(Otc::GameMapDontCorrectCorpse))
        m_countFlag.correctCorpse += value;

    // Creatures and items
    if (thing->isOnBottom()) {
        m_countFlag.hasBottomItem += value;

        if (thing->isHookSouth())
            m_countFlag.hasHookSouth += value;

        if (thing->isHookEast())
            m_countFlag.hasHookEast += value;
    }

    if (hasElevation())
        m_countFlag.hasThingWithElevation += value;

    // best option to have something more real, but in some cases as a custom project,
    // the developers are not defining crop size
    //if(thing->getRealSize() > SPRITE_SIZE)
    if (!thing->isSingleDimension() || thing->hasElevation() || thing->hasDisplacement())
        m_countFlag.notSingleDimension += value;

    if (thing->getHeight() > 1)
        m_countFlag.hasTallThings += value;

    if (thing->getWidth() > 1)
        m_countFlag.hasWideThings += value;

    if (!thing->isItem()) return;

    if (thing->getHeight() > 1)
        m_countFlag.hasTallItems += value;

    if (thing->getWidth() > 1)
        m_countFlag.hasWideItems += value;

    if (thing->getWidth() > 1 && thing->getHeight() > 1)
        m_countFlag.hasWall += value;

    if (thing->isNotWalkable())
        m_countFlag.notWalkable += value;

    if (thing->isNotPathable())
        m_countFlag.notPathable += value;

    if (thing->blockProjectile())
        m_countFlag.blockProjectile += value;

    m_totalElevation += thing->getElevation() * value;

    if (thing->isFullGround())
        m_countFlag.fullGround += value;

    if (thing->hasElevation())
        m_countFlag.elevation += value;

    if (thing->isOpaque()) {
        m_countFlag.opaque += value;
    }

    if (thing->isGroundBorder() && thing->isNotWalkable())
        m_countFlag.hasNoWalkableEdge += value;
}

void Tile::select(TileSelectType selectType)
{
    if (selectType == TileSelectType::NO_FILTERED) {
        m_highlightThing = getTopCreature();
        if (!m_highlightThing)
            m_highlightThing = m_things.back();
    }

    m_selectType = selectType;
}


void Tile::unselect()
{
    if (m_selectType == TileSelectType::NO_FILTERED)
        checkForDetachableThing();

    m_selectType = TileSelectType::NONE;
}

bool Tile::canRender(uint32_t& flags, const Position& cameraPosition, const AwareRange viewPort, LightView* lightView)
{
    const int8_t dz = m_position.z - cameraPosition.z;
    const Position checkPos = m_position.translated(dz, dz);

    bool draw = true;

    // Check for non-visible tiles on the screen and ignore them
    {
        if ((cameraPosition.x - checkPos.x >= viewPort.left) || (checkPos.x - cameraPosition.x == viewPort.right && !hasWideThings() && !hasDisplacement() && !hasThingWithElevation() && m_walkingCreatures.empty()))
            draw = false;
        else if ((cameraPosition.y - checkPos.y >= viewPort.top) || (checkPos.y - cameraPosition.y == viewPort.bottom && !hasTallThings() && !hasDisplacement() && !hasThingWithElevation() && m_walkingCreatures.empty()))
            draw = false;
        else if ((checkPos.x - cameraPosition.x > viewPort.right && (!hasWideThings() || !hasDisplacement() || !hasThingWithElevation())) || (checkPos.y - cameraPosition.y > viewPort.bottom))
            draw = false;
    }

    if (!draw) {
        flags &= ~Otc::DrawThings;
        if (!hasLight())
            flags &= ~Otc::DrawLights;
        if (!hasCreature())
            flags &= ~(Otc::DrawManaBar | Otc::DrawNames | Otc::DrawBars);
    }

    return flags > 0;
}