HOTKEY_USE = nil
HOTKEY_USEONSELF = 1
HOTKEY_USEONTARGET = 2
HOTKEY_USEWITH = 3

local maxSlots = 76
actionBar = nil
actionBarPanel_1 = nil
actionBarPanel_2 = nil
actionBarPanel_3 = nil
actionBarPanel = nil
slotToEdit = nil
spellAssignWindow = nil
spellsPanel = nil
textAssignWindow = nil
objectAssignWindow = nil
mouseGrabberWidget = nil
actionRadioGroup = nil
editHotkeyWindow = nil
missedSlotToEdit = nil
itemDragRetry = nil
slotReassign = nil
lastHotkeyTime = 0
cooldown = {}
groupCooldown = {}

local ProgressCallback = {
    update = 1,
    finish = 2
}

function init()
    actionBarPanel = modules.game_interface.getActionBarPanel()
    actionBar = g_ui.loadUI('game_actionbar', actionBarPanel)
    actionBarPanel_1 = actionBar:getChildById('actionBarPanel_1')
    actionBarPanel_2 = actionBar:getChildById('actionBarPanel_2')
    actionBarPanel_3 = actionBar:getChildById('actionBarPanel_3')
    mouseGrabberWidget = g_ui.createWidget('UIWidget')
    mouseGrabberWidget:setVisible(false)
    mouseGrabberWidget:setFocusable(false)
    mouseGrabberWidget.onMouseRelease = onChooseItemMouseRelease

    local console = modules.game_console.consolePanel
    if console then
        console:addAnchor(AnchorTop, actionBar:getId(), AnchorBottom)
    end

    if g_game.isOnline() then
        addEvent(function()
            setupActionBar()
            loadActionBar()
        end)
    end

    connect(g_game, {
        onGameStart = online,
        onGameEnd = offline,
        onSpellGroupCooldown = onSpellGroupCooldown,
        onSpellCooldown = onSpellCooldown
    })

end

function terminate()
    actionBar:destroy()
    mouseGrabberWidget:destroy()
    disconnect(g_game, {
        onGameStart = online,
        onGameEnd = offline,
        onSpellGroupCooldown = onSpellGroupCooldown,
        onSpellCooldown = onSpellCooldown
    })
    if spellAssignWindow then
        closeSpellAssignWindow()
    end
    if objectAssignWindow then
        closeObjectAssignWindow()
    end
    if textAssignWindow then
        closeTextAssignWindow()
    end
    if editHotkeyWindow then
        closeEditHotkeyWindow()
    end
    if spellsPanel then
        disconnect(spellsPanel, {
            onChildFocusChange = function(self, focusedChild)
                if focusedChild == nil then
                    return
                end
                updatePreviewSpell(focusedChild)
            end
        })
    end

    local console = modules.game_console.consolePanel
    if console then
        console:removeAnchor(AnchorTop)
        console:fill('parent')
    end
end

function online()
    actionBarPanel_1:destroyChildren()
    actionBarPanel_2:destroyChildren()
    actionBarPanel_3:destroyChildren()
    addEvent(function()
        setupActionBar()
        loadActionBar()
    end)
end

function offline()
    saveActionBar()
    unbindHotkeys()
end

function copySlot(fromSlotId, toSlotId, visible)
    local fromSlot = actionBarPanel_1:getChildById(fromSlotId)
    local tmpslot = actionBarPanel_1:getChildById(toSlotId)
    if not tmpslot then
        tmpslot = g_ui.createWidget('ActionSlot', actionBarPanel_1)
        tmpslot:setId(toSlotId)
    end
    tmpslot:setVisible(visible)
    local tmptext = not fromSlot.text
    local tmpid = not fromSlot.itemId
    local tmpwords = not fromSlot.words
    local imageSource = fromSlot:getImageSource()
    local imageClip = fromSlot:getImageClip()
    local imgsrcbool = not imageSource
    local imgclipbool = not imageClip
    imageSource = (imgsrcbool or (tmptext and tmpid and tmpwords)) and '/images/game/actionbar/slot-actionbar' or
                      imageSource
    imageClip = imgclipbool and '0 0 0 0' or imageClip
    tmpslot:setImageSource(imageSource)
    tmpslot:setImageClip(imageClip)
    local tmpItem = fromSlot:getItem()
    if tmpItem then
        tmpslot:setItem(tmpItem)
    else
        tmpslot:setItem(nil)
    end
    tmpslot:setText(fromSlot:getText())
    tmpslot.autoSend = fromSlot.autoSend
    tmpslot.itemId = fromSlot.itemId
    tmpslot.subType = fromSlot.subType
    tmpslot.words = fromSlot.words
    tmpslot.text = fromSlot.text
    tmpslot.parameter = fromSlot.parameter
    tmpslot.useType = fromSlot.useType
    tmpslot:getChildById('text'):setText(fromSlot:getChildById('text'):getText())
    tmpslot:setTooltip(fromSlot:getTooltip())
end

function onDropFunc(slotId)
    if slotReassign then
        local fromSlotId = slotToEdit
        local toSlotId = slotId
        local fromSlot = actionBarPanel_1:getChildById(fromSlotId)
        local toSlot = actionBarPanel_1:getChildById(toSlotId)
        if fromSlot and toSlot then
            local tmpslotid = 'slot' .. maxSlots + 1
            copySlot(fromSlotId, tmpslotid, false)
            copySlot(toSlotId, fromSlotId, true)
            copySlot(tmpslotid, toSlotId, true)
            clearSlotById(tmpslotid)
        end
        slotReassign = nil
        slotToEdit = nil
    end
    slotToEdit = slotId
    if itemDragRetry and missedSlotToEdit then -- first drag doesn't register slotToEdit
        local widget1 = missedSlotToEdit[1]
        local mousePos1 = missedSlotToEdit[2]
        local item1 = missedSlotToEdit[3]
        if widget1 and mousePos1 and item1 then
            onChooseItemByDrag(widget1, mousePos1, item1)
        end
        itemDragRetry = nil
        missedSlotToEdit = nil
    end
    setupHotkeys()
end



function setupActionBar()
    for i = 1, 75 do
        if i >= 1 and i <= 25 then
            slotSet(i, actionBarPanel_1)
        end
        if i >= 26 and i <= 50 then
            slotSet(i, actionBarPanel_2)
        end
        if i >= 51 and i <= 75 then
            slotSet(i, actionBarPanel_3)
        end
    end
end
function slotSet(slotNumber, actionBarPanel)
    slot = g_ui.createWidget('ActionSlot', actionBarPanel)
    slot:setId('slot' .. slotNumber)
    slot:setVisible(true)
    slot.itemId = nil
    slot.subType = nil
    slot.words = nil
    slot.text = nil
    slot.useType = nil

    g_mouse.bindPress(slot, function()
        slotToEdit = 'slot' .. slotNumber .. ''
    end, MouseLeftButton)

    g_mouse.bindPress(slot, function()
        createMenu('slot' .. slotNumber, actionBarPanel)
    end, MouseRightButton)

    g_mouse.bindOnDrop(slot, function()
        if slotToEdit == 'slot' .. slotNumber then
            slotReassign = 'slot' .. slotNumber
        end
        onDropFunc('slot' .. slotNumber)
    end)

    if slotNumber == 1 or slotNumber == 26 or slotNumber == 51 then
        slot:addAnchor(AnchorTop, 'parent', AnchorTop)
    end
end
function createMenu(slotId, actionBarPanel)
    local menu = g_ui.createWidget('PopupMenu')
    slotToEdit = slotId
    --menu:addOption('Assign Spell', function()
    --    openSpellAssignWindow()
    --end)
    menu:addOption('Object Action', function()
        startChooseItem()
        openObjectAssignWindow()
    end)
    menu:addOption('Text Action', function()
        openTextAssignWindow()
    end)
    menu:addOption('Add/Edit Hotkey', function()
        openEditHotkeyWindow()
    end)
    local actionSlot = actionBarPanel:recursiveGetChildById(slotToEdit)
    if actionSlot.itemId or actionSlot.words or actionSlot.text or actionSlot.useType or actionSlot.hotkey then
        menu:addOption('Clear Slot', function()
            clearSlot()
            clearHotkey()
        end)
    end
    menu:display()
end

-- function openSpellAssignWindow()
--     spellAssignWindow = g_ui.loadUI('assign_spell', g_ui.getRootWidget())
--     spellsPanel = spellAssignWindow:getChildById('spellsPanel')
--     addEvent(function()
--         initializeSpelllist()
--     end)
--     spellAssignWindow:raise()
--     spellAssignWindow:focus()
--     spellAssignWindow:getChildById('filterTextEdit'):focus()
--     modules.game_hotkeys.enableHotkeys(false)
-- end

-- function closeSpellAssignWindow()
--     spellAssignWindow:destroy()
--     spellAssignWindow = nil
--     spellsPanel = nil
--     modules.game_hotkeys.enableHotkeys(true)
-- end

-- function initializeSpelllist()
--     g_keyboard.bindKeyPress('Down', function()
--         spellsPanel:focusNextChild(KeyboardFocusReason)
--     end, spellsPanel:getParent())
--     g_keyboard.bindKeyPress('Up', function()
--         spellsPanel:focusPreviousChild(KeyboardFocusReason)
--     end, spellsPanel:getParent())

--     for spellProfile, _ in pairs(SpelllistSettings) do
--         for i = 1, #SpelllistSettings[spellProfile].spellOrder do
--             local spell = SpelllistSettings[spellProfile].spellOrder[i]
--             local info = SpellInfo[spellProfile][spell]
--             if info then
--                 local tmpLabel = g_ui.createWidget('SpellListLabel', spellsPanel)
--                 tmpLabel:setId(spell)
--                 tmpLabel:setText(spell .. '\n\'' .. info.words .. '\'')
--                 tmpLabel:setPhantom(false)
--                 tmpLabel.defaultHeight = tmpLabel:getHeight()
--                 tmpLabel.words = info.words:lower()
--                 tmpLabel.name = spell:lower()

--                 local iconId = tonumber(info.icon)
--                 if not iconId and SpellIcons[info.icon] then
--                     iconId = SpellIcons[info.icon][1]
--                 end

--                 tmpLabel:setHeight(SpelllistSettings[spellProfile].iconSize.height + 4)
--                 tmpLabel:setTextOffset(topoint((SpelllistSettings[spellProfile].iconSize.width + 10) .. ' ' ..
--                                                    (SpelllistSettings[spellProfile].iconSize.height - 32) / 2 + 3))
--                 tmpLabel:setImageSource(SpelllistSettings[spellProfile].iconFile)
--                 tmpLabel:setImageClip(Spells.getImageClip(iconId, spellProfile))
--                 tmpLabel:setImageSize(tosize(SpelllistSettings[spellProfile].iconSize.width .. ' ' ..
--                                                  SpelllistSettings[spellProfile].iconSize.height))
--             end
--         end
--     end

--     for v, k in ipairs(spellsPanel:getChildren()) do
--         if k:isVisible() then
--             spellsPanel:focusChild(k, KeyboardFocusReason)
--             updatePreviewSpell(k)
--             break
--         end
--     end
--     connect(spellsPanel, {
--         onChildFocusChange = function(self, focusedChild)
--             if focusedChild == nil then
--                 return
--             end
--             updatePreviewSpell(focusedChild)
--         end
--     })
-- end

function updatePreviewSpell(focusedChild)
    local spellName = focusedChild:getId()
    iconId = tonumber(Spells.getClientId(spellName))
    local spell = Spells.getSpellByName(spellName)
    local profile = Spells.getSpellProfileByName(spellName)
    spellsPanel:getParent():getChildById('previewSpell'):setImageSource(SpelllistSettings[profile].iconFile)
    spellsPanel:getParent():getChildById('previewSpell'):setImageClip(Spells.getImageClip(iconId, profile))
    spellsPanel:getParent():getChildById('previewSpellName'):setText(spellName)
    spellsPanel:getParent():getChildById('previewSpellWords'):setText('\'' .. spell.words .. '\'')
    if spell.parameter then
        spellAssignWindow:getChildById('parameterTextEdit'):enable()
    else
        spellAssignWindow:getChildById('parameterTextEdit'):disable()
    end
end

function spellAssignAccept()
    clearSlot()
    local focusedChild = spellsPanel:getFocusedChild()
    if not focusedChild then
        return
    end
    local spellName = focusedChild:getId()
    iconId = tonumber(Spells.getClientId(spellName))
    local spell = Spells.getSpellByName(spellName)
    local profile = Spells.getSpellProfileByName(spellName)
    local slot = actionBarPanel_1:getChildById(slotToEdit)
    slot:setImageSource(Spells.getIconFileByProfile(profile))
    slot:setImageClip(Spells.getImageClip(iconId, profile))
    slot.words = spell.words
    slot.itemId = 469
    slot:setItemId(469)
    if spell.parameter then
        slot.parameter = spellAssignWindow:getChildById('parameterTextEdit'):getText():gsub('"', '')
    else
        slot.parameter = nil
    end
    closeSpellAssignWindow()
    setupHotkeys()
end

function getSlotById(id)
    numid = tonumber(string.sub(id, 5, 6))
    if numid >= 1 and numid <= 25 then
        return actionBarPanel_1:getChildById(id)
    end
    if numid >= 26 and numid <= 50 then
        return actionBarPanel_2:getChildById(id)
    end
    if numid >= 51 and numid <= 75 then
        return actionBarPanel_3:getChildById(id)
    end
    return nil
end
function getChildrenBySlotId(id)
    numid = tonumber(string.sub(id, 5, 6))
    if numid >= 1 and numid <= 25 then
        return actionBarPanel_1:getChildren()
    end
    if numid >= 26 and numid <= 50 then
        return actionBarPanel_2:getChildren()
    end
    if numid >= 51 and numid <= 75 then
        return actionBarPanel_3:getChildren()
    end
    return nil
end
local AllSlotChildrens = nil
function getAllChildrens()
    if AllSlotChildrens == nil then
        AllSlotChildrens = {}
        local table = {actionBarPanel_1:getChildren(), actionBarPanel_2:getChildren(), actionBarPanel_3:getChildren()}
        for i in pairs(table) do
            for index in pairs(table[i]) do 
                AllSlotChildrens[#AllSlotChildrens + 1] = table[i][index]
            end
        end
    end
    return AllSlotChildrens
end
function clearSlot()
    local slot = getSlotById(slotToEdit)
    slot:setImageSource('/images/game/actionbar/slot-actionbar')
    slot:setImageClip('0 0 0 0')
    slot:clearItem()
    slot:setText('')
    slot.itemId = nil
    slot.subType = nil
    slot.words = nil
    slot.text = nil
    slot.useType = nil
    slot:getChildById('text'):setText('')
    slot:setTooltip('')
end

function clearSlotById(slotId)
    local slot = getSlotById(slotId)
    slot:setImageSource('/images/game/actionbar/slot-actionbar')
    slot:setImageClip('0 0 0 0')
    slot:clearItem()
    slot:setText('')
    slot.itemId = nil
    slot.subType = nil
    slot.words = nil
    slot.text = nil
    slot.useType = nil
    slot:getChildById('text'):setText('')
    slot:setTooltip('')
end

function clearHotkey()
    local slot = getSlotById(slotToEdit)
    slot.hotkey = nil
    slot:getChildById('key'):setText('')
end

function openTextAssignWindow()
    textAssignWindow = g_ui.loadUI('assign_text', g_ui.getRootWidget())
    textAssignWindow:raise()
    textAssignWindow:focus()
    modules.game_hotkeys.enableHotkeys(false)
end

function closeTextAssignWindow()
    textAssignWindow:destroy()
    textAssignWindow = nil
    modules.game_hotkeys.enableHotkeys(true)
end

function textAssignAccept()
    local text = textAssignWindow:getChildById('textToSendTextEdit'):getText()
    if text == '' then
        return
    end
    local checkForParameter = text:split(' "')
    local name, parameter = nil, nil
    if #checkForParameter == 2 then
        name = checkForParameter[1]
        parameter = checkForParameter[2]
    else
        name = text
    end

    local spell, profile, spellName = Spells.getSpellByWords(name)

    local slot = getSlotById(slotToEdit)
    if spellName then
        iconId = tonumber(Spells.getClientId(spellName))
        clearSlot()
        slot:setImageSource(Spells.getIconFileByProfile(profile))
        slot:setImageClip(Spells.getImageClip(iconId, profile))
        slot.words = spell.words
        slot.itemId = 469
        slot:setItemId(469)
        if parameter and spell.parameter then
            slot.parameter = parameter
        else
            slot.parameter = nil
        end
    else
        clearSlot()
        slot:getChildById('text'):setText(text)
        while slot:getChildById('text'):getTextSize().height > 30 do
            local subString = slot:getChildById('text'):getText()
            subString = string.sub(subString, 1, #subString - 1)
            slot:getChildById('text'):setText(subString)
        end
        slot:setImageSource('/images/game/actionbar/item-background')
        slot.text = text
        slot.itemId = 469
        slot:setItemId(469)
        slot.autoSend = textAssignWindow:recursiveGetChildById('sendAutomaticallyCheckBox'):isChecked()
        slot:setTooltip(slot.text)
        setupHotkeys()
    end
    closeTextAssignWindow()
end

function openObjectAssignWindow()
    if objectAssignWindow ~= nil then
        objectAssignWindow:destroy()
    end
    objectAssignWindow = g_ui.loadUI('assign_object', g_ui.getRootWidget())
    actionRadioGroup = UIRadioGroup.create()
    actionRadioGroup:addWidget(objectAssignWindow:getChildById('useOnYourselfCheckbox'))
    actionRadioGroup:addWidget(objectAssignWindow:getChildById('useOnTargetCheckbox'))
    actionRadioGroup:addWidget(objectAssignWindow:getChildById('useWithCrosshairCheckbox'))
    actionRadioGroup:addWidget(objectAssignWindow:getChildById('equipCheckbox'))
    actionRadioGroup:addWidget(objectAssignWindow:getChildById('useCheckbox'))
    objectAssignWindow:setVisible(false)
end

function closeObjectAssignWindow()
    objectAssignWindow:destroy()
    objectAssignWindow = nil
    actionRadioGroup = nil
    modules.game_hotkeys.enableHotkeys(true)
end

function startChooseItem()
    if g_ui.isMouseGrabbed() then
        return
    end
    mouseGrabberWidget:grabMouse()
    g_mouse.pushCursor('target')
end

function objectAssignAccept()
    clearSlot()
    local item = objectAssignWindow:getChildById('previewItem'):getItem()
    if not item then
        return
    end
    local slot = getSlotById(slotToEdit)
    slot:setItem(item)
    slot:setImageSource('/images/game/actionbar/item-background')
    slot:setBorderWidth(0)
    slot.itemId = item:getId()
    if item:isFluidContainer() then
        slot.subType = item:getSubType()
    end
    if objectAssignWindow:getChildById('equipCheckbox'):isChecked() then
        slot.useType = 'equip'
    elseif objectAssignWindow:getChildById('useCheckbox'):isChecked() then
        slot.useType = 'use'
    elseif objectAssignWindow:getChildById('useOnYourselfCheckbox'):isChecked() then
        slot.useType = 'useOnSelf'
    elseif objectAssignWindow:getChildById('useOnTargetCheckbox'):isChecked() then
        slot.useType = 'useOnTarget'
    elseif objectAssignWindow:getChildById('useWithCrosshairCheckbox'):isChecked() then
        slot.useType = 'useWith'
    end
    setupHotkeys()
    closeObjectAssignWindow()
end

function onChooseItemMouseRelease(self, mousePosition, mouseButton)
    local item = nil
    if mouseButton == MouseLeftButton then
        local clickedWidget = modules.game_interface.getRootPanel():recursiveGetChildByPos(mousePosition, false)
        if clickedWidget then
            if clickedWidget:getClassName() == 'UIItem' and not clickedWidget:isVirtual() then
                item = clickedWidget:getItem()
            end
        end
    end

    if item and item:getPosition().x == 65535 and slotToEdit then
        objectAssignWindow:getChildById('previewItem'):setItemId(item:getId())
        objectAssignWindow:getChildById('previewItem'):setItemCount(1)
        objectAssignWindow:getChildById('equipCheckbox'):setEnabled(false)
        objectAssignWindow:getChildById('useCheckbox'):setEnabled(false)
        objectAssignWindow:getChildById('useOnYourselfCheckbox'):setEnabled(false)
        objectAssignWindow:getChildById('useOnTargetCheckbox'):setEnabled(false)
        objectAssignWindow:getChildById('useWithCrosshairCheckbox'):setEnabled(false)
        if item:getClothSlot() > 0 then
            objectAssignWindow:getChildById('equipCheckbox'):setEnabled(true)
            if item:isMultiUse() then
                objectAssignWindow:getChildById('useOnYourselfCheckbox'):setEnabled(true)
                objectAssignWindow:getChildById('useOnTargetCheckbox'):setEnabled(true)
                objectAssignWindow:getChildById('useWithCrosshairCheckbox'):setEnabled(true)
            else
                objectAssignWindow:getChildById('useCheckbox'):setEnabled(true)
            end
            actionRadioGroup:selectWidget(objectAssignWindow:getChildById('equipCheckbox'))
        elseif item:isMultiUse() then
            objectAssignWindow:getChildById('useOnYourselfCheckbox'):setEnabled(true)
            objectAssignWindow:getChildById('useOnTargetCheckbox'):setEnabled(true)
            objectAssignWindow:getChildById('useWithCrosshairCheckbox'):setEnabled(true)
            objectAssignWindow:getChildById('equipCheckbox'):setEnabled(true)
            actionRadioGroup:selectWidget(objectAssignWindow:getChildById('useOnYourselfCheckbox'))
        else
            objectAssignWindow:getChildById('useCheckbox'):setEnabled(true)
            actionRadioGroup:selectWidget(objectAssignWindow:getChildById('useCheckbox'))
        end
        if not objectAssignWindow:isVisible() then
            objectAssignWindow:show()
        end
        objectAssignWindow:raise()
        objectAssignWindow:focus()
    end
    g_mouse.popCursor('target')
    self:ungrabMouse()
    return true
end

function onChooseItemByDrag(self, mousePosition, item)
    if item and item:getPosition().x == 65535 and slotToEdit then
        openObjectAssignWindow()
        objectAssignWindow:getChildById('previewItem'):setItemId(item:getId())
        objectAssignWindow:getChildById('previewItem'):setItemCount(1)
        objectAssignWindow:getChildById('equipCheckbox'):setEnabled(false)
        objectAssignWindow:getChildById('useCheckbox'):setEnabled(false)
        objectAssignWindow:getChildById('useOnYourselfCheckbox'):setEnabled(false)
        objectAssignWindow:getChildById('useOnTargetCheckbox'):setEnabled(false)
        objectAssignWindow:getChildById('useWithCrosshairCheckbox'):setEnabled(false)
        if item:getClothSlot() > 0 then
            objectAssignWindow:getChildById('equipCheckbox'):setEnabled(true)
            if item:isMultiUse() then
                objectAssignWindow:getChildById('useOnYourselfCheckbox'):setEnabled(true)
                objectAssignWindow:getChildById('useOnTargetCheckbox'):setEnabled(true)
                objectAssignWindow:getChildById('useWithCrosshairCheckbox'):setEnabled(true)
            else
                objectAssignWindow:getChildById('useCheckbox'):setEnabled(true)
            end
            actionRadioGroup:selectWidget(objectAssignWindow:getChildById('equipCheckbox'))
        elseif item:isMultiUse() then
            objectAssignWindow:getChildById('useOnYourselfCheckbox'):setEnabled(true)
            objectAssignWindow:getChildById('useOnTargetCheckbox'):setEnabled(true)
            objectAssignWindow:getChildById('useWithCrosshairCheckbox'):setEnabled(true)
            objectAssignWindow:getChildById('equipCheckbox'):setEnabled(true)
            actionRadioGroup:selectWidget(objectAssignWindow:getChildById('useOnYourselfCheckbox'))
        else
            objectAssignWindow:getChildById('useCheckbox'):setEnabled(true)
            actionRadioGroup:selectWidget(objectAssignWindow:getChildById('useCheckbox'))
        end
        if not objectAssignWindow:isVisible() then
            objectAssignWindow:show()
        end
        objectAssignWindow:raise()
        objectAssignWindow:focus()
    elseif not slotToEdit then
        itemDragRetry = true
        missedSlotToEdit = {self, mousePosition, item}
    end
end

function onDragReassign(self, item)
    slotReassign = self
end

function openEditHotkeyWindow()
    editHotkeyWindow = g_ui.loadUI('edit_hotkey', g_ui.getRootWidget())
    editHotkeyWindow:grabKeyboard()

    local comboLabel = editHotkeyWindow:recursiveGetChildById('comboPreview')
    comboLabel.keyCombo = ''
    editHotkeyWindow.onKeyDown = hotkeyCapture
    editHotkeyWindow:raise()
    editHotkeyWindow:focus()
    modules.game_hotkeys.enableHotkeys(false)
end

function closeEditHotkeyWindow()
    editHotkeyWindow:destroy()
    editHotkeyWindow = nil
    modules.game_hotkeys.enableHotkeys(true)
end

function unbindHotkeys()
    for v, slot in pairs(actionBarPanel_1:getChildren()) do
        if slot.hotkey and slot.hotkey ~= '' then
            g_keyboard.unbindKeyPress(slot.hotkey)
        end
    end
    for v, slot in pairs(actionBarPanel_2:getChildren()) do
        if slot.hotkey and slot.hotkey ~= '' then
            g_keyboard.unbindKeyPress(slot.hotkey)
        end
    end
    for v, slot in pairs(actionBarPanel_3:getChildren()) do
        if slot.hotkey and slot.hotkey ~= '' then
            g_keyboard.unbindKeyPress(slot.hotkey)
        end
    end
end

function setupHotkeys()
    unbindHotkeys()
    local slotTable = getAllChildrens()
    for v, slot in pairs(slotTable) do
        slot.onMouseRelease = function()
            if g_clock.millis() - lastHotkeyTime < modules.client_options.getOption('hotkeyDelay') then
                return
            end

            lastHotkeyTime = g_clock.millis()
            if slot.itemId and slot.useType then
                if slot.useType == 'use' then
                    modules.game_hotkeys.executeHotkeyItem(HOTKEY_USE, slot.itemId, slot.subType)
                elseif slot.useType == 'useOnTarget' then
                    modules.game_hotkeys.executeHotkeyItem(HOTKEY_USEONTARGET, slot.itemId, slot.subType)
                elseif slot.useType == 'useWith' then
                    modules.game_hotkeys.executeHotkeyItem(HOTKEY_USEWITH, slot.itemId, slot.subType)
                elseif slot.useType == 'useOnSelf' then
                    modules.game_hotkeys.executeHotkeyItem(HOTKEY_USEONSELF, slot.itemId, slot.subType)
                elseif slot.useType == 'equip' then
                    local item = g_game.findPlayerItem(slot.itemId, -1)
                    if item then
                        g_game.equipItem(item)
                    end
                end
            elseif slot.words then
                if slot.parameter and slot.parameter ~= '' then
                    g_game.talk(slot.words .. ' "' .. slot.parameter)
                else
                    g_game.talk(slot.words)
                end
            elseif slot.text then
                if slot.autoSend then
                    g_game.talk(slot.text)
                else
                    if not modules.game_console.isChatEnabled() then
                        modules.game_console.switchChatOnCall()
                    end
                    modules.game_console.setTextEditText(slot.text)
                end
            end
        end

        if slot.hotkey and slot.hotkey ~= '' then
            g_keyboard.bindKeyPress(slot.hotkey, function()
                if not modules.game_hotkeys.canPerformKeyCombo(slot.hotkey) then
                    return
                end
                if g_clock.millis() - lastHotkeyTime < modules.client_options.getOption('hotkeyDelay') then
                    return
                end

                lastHotkeyTime = g_clock.millis()
                if slot.itemId and slot.useType then
                    if slot.useType == 'use' then
                        modules.game_hotkeys.executeHotkeyItem(HOTKEY_USE, slot.itemId, slot.subType)
                    elseif slot.useType == 'useOnTarget' then
                        modules.game_hotkeys.executeHotkeyItem(HOTKEY_USEONTARGET, slot.itemId, slot.subType)
                    elseif slot.useType == 'useWith' then
                        modules.game_hotkeys.executeHotkeyItem(HOTKEY_USEWITH, slot.itemId, slot.subType)
                    elseif slot.useType == 'useOnSelf' then
                        modules.game_hotkeys.executeHotkeyItem(HOTKEY_USEONSELF, slot.itemId, slot.subType)
                    elseif slot.useType == 'equip' then
                        local item = g_game.findPlayerItem(slot.itemId, -1)
                        if item then
                            g_game.equipItem(item)
                        end
                    end
                elseif slot.words then
                    if slot.parameter and slot.parameter ~= '' then
                        g_game.talk(slot.words .. ' "' .. slot.parameter)
                    else
                        g_game.talk(slot.words)
                    end
                elseif slot.text then
                    if slot.autoSend then
                        modules.game_console.sendMessage(slot.text)
                    else
                        scheduleEvent(function()
                            if not modules.game_console.isChatEnabled() then
                                modules.game_console.switchChatOnCall()
                            end
                            modules.game_console.setTextEditText(slot.text)
                        end, 1)
                    end
                end
            end)
        end
    end
end

function checkHotkey(hotkey)
    for v, k in pairs(actionBarPanel_1:getChildren()) do
        if k.hotkey == hotkey then
            return true
        end
    end    
    for v, k in pairs(actionBarPanel_2:getChildren()) do
        if k.hotkey == hotkey then
            return true
        end
    end    
    for v, k in pairs(actionBarPanel_3:getChildren()) do
        if k.hotkey == hotkey then
            return true
        end
    end
end

function hotkeyCapture(assignWindow, keyCode, keyboardModifiers)
    local hotkeyAlreadyUsed = false
    assignWindow:raise()
    assignWindow:focus()
    local keyCombo = determineKeyComboDesc(keyCode, keyboardModifiers)
    local comboPreview = assignWindow:recursiveGetChildById('comboPreview')
    local errorLabel = editHotkeyWindow:recursiveGetChildById('errorLabel')
    if checkHotkey(keyCombo) then
        errorLabel:setVisible(true)
        editHotkeyWindow:setHeight(180)
    else
        errorLabel:setVisible(false)
        editHotkeyWindow:setHeight(160)
    end
    comboPreview:setText(tr('Current hotkey to change: %s', keyCombo))
    comboPreview.keyCombo = keyCombo
    comboPreview:resizeToText()
    assignWindow:getChildById('applyButton'):enable()
    return true
end

function hotkeyClear(assignWindow)
    local comboPreview = assignWindow:recursiveGetChildById('comboPreview')
    comboPreview:setText(tr('Current hotkey to change: none'))
    comboPreview.keyCombo = ''
    comboPreview:resizeToText()
    assignWindow:getChildById('applyButton'):disable()
end

function hotkeyCaptureOk(assignWindow, keyCombo)
    local slot = getSlotById(slotToEdit)
    if checkHotkey(keyCombo) then

        for v, k in pairs(getChildrenBySlotId(slotToEdit)) do
            if k.hotkey == keyCombo then
                k.hotkey = ''
                k:getChildById('key'):setText('')
            end
        end
    end
    unbindHotkeys()
    slot.hotkey = keyCombo
    local text = slot.hotkey
    text = text:gsub('Shift', 'S')
    text = text:gsub('Alt', 'A')
    text = text:gsub('Ctrl', 'C')
    text = text:gsub('+', '')
    slot:getChildById('key'):setText(text)
    setupHotkeys()
    if assignWindow == editHotkeyWindow then
        closeEditHotkeyWindow()
        return
    end
    assignWindow:destroy()
end

function saveActionBar()
    local hotkeySettings = g_settings.getNode('game_actionbar') or {}
    local hotkeys = hotkeySettings

    local char = g_game.getCharacterName()
    if not hotkeys[char] then
        hotkeys[char] = {}
    end
    hotkeys = hotkeys[char]

    table.clear(hotkeys)
    local currentHotkeys = getAllChildrens()
    for v, slot in ipairs(currentHotkeys) do
        hotkeys[slot:getId()] = {
            hotkey = slot.hotkey,
            autoSend = slot.autoSend,
            itemId = slot.itemId,
            subType = slot.subType,
            useType = slot.useType,
            text = slot.text,
            words = slot.words,
            parameter = slot.parameter
        }
    end

    g_settings.setNode('game_actionbar', hotkeySettings)
    g_settings.save()
end

function loadSpell(slot)
    local spell, profile, spellName = Spells.getSpellByWords(slot.words)
    iconId = tonumber(Spells.getClientId(spellName))
    slot:setImageSource(Spells.getIconFileByProfile(profile))
    slot:setImageClip(Spells.getImageClip(iconId, profile))
    slot:getChildById('text'):setText('')
    slot:setBorderWidth(0)
    setupHotkeys()
end

function loadObject(slot)
    slot:setItemId(slot.itemId)
    slot:setImageSource('/images/game/actionbar/item-background')
    slot:setImageClip('0 0 0 0')
    slot:getChildById('text'):setText('')
    slot:setBorderWidth(0)
    setupHotkeys()
end

function loadText(slot)
    slot:getChildById('text'):setText(slot.text)
    while slot:getChildById('text'):getTextSize().height > 30 do
        local subString = slot:getChildById('text'):getText()
        subString = string.sub(subString, 1, #subString - 1)
        slot:getChildById('text'):setText(subString)
    end
    slot:setImageSource('/images/game/actionbar/item-background')
    slot:setImageClip('0 0 0 0')
    setupHotkeys()
end

function loadActionBar()
    unbindHotkeys()
    local hotkeySettings = g_settings.getNode('game_actionbar')
    local hotkeys = {}

    if not table.empty(hotkeySettings) then
        hotkeys = hotkeySettings
    end
    if not table.empty(hotkeys) then
        hotkeys = hotkeys[g_game.getCharacterName()]
    end
    if hotkeys then
        for slot, setting in pairs(hotkeys) do
            slot = getSlotById(slot)
            if slot then
                slot.itemId = setting.itemId
                slot:setItemId(setting.itemId)
                slot.subType = setting.subType
                slot.words = setting.words
                slot.text = setting.text
                slot.hotkey = setting.hotkey
                slot.useType = setting.useType
                slot.autoSend = setting.autoSend
                slot.parameter = setting.parameter
                if slot.hotkey then
                    local text = slot.hotkey
                    if type(text) == 'string' then
                        text = text:gsub('Shift', 'S')
                        text = text:gsub('Alt', 'A')
                        text = text:gsub('Ctrl', 'C')
                        text = text:gsub('+', '')
                    end
                    slot:getChildById('key'):setText(text)
                end
                if slot.words then
                    loadSpell(slot)
                elseif slot.text then
                    loadText(slot)
                elseif slot.itemId and slot.itemId > 0 then
                    loadObject(slot)
                end
            end
        end
    end
    setupHotkeys()
end

function round(n)
    return n % 1 >= 0.5 and math.ceil(n) or math.floor(n)
end

function updateCooldown(progressRect, duration, spellId, count)
    progressRect:setPercent(progressRect:getPercent() + 10000 / duration)
    local cd = round(duration - (progressRect:getPercent() * duration / 100)) / 1000
    if cd > 0 then
        progressRect:setText(cd .. 's')
    end

    if progressRect:getPercent() < 100 then
        removeEvent(progressRect.event)
        cooldown[spellId] = duration - count * 100
        progressRect.event = scheduleEvent(function()
            updateCooldown(progressRect, duration, spellId, count + 1)
        end, 100)
    else
        cooldown[spellId] = nil
        progressRect:destroy()
    end
end

function updateGroupCooldown(progressRect, duration, groupId)
    progressRect:setPercent(progressRect:getPercent() + 10000 / duration)
    local cd = round(duration - (progressRect:getPercent() * duration / 100)) / 1000
    if cd > 0 then
        progressRect:setText(cd .. 's')
    end

    if progressRect:getPercent() < 100 then
        removeEvent(progressRect.event)
        progressRect.event = scheduleEvent(function()
            updateGroupCooldown(progressRect, duration, groupId)
        end, 100)
    else
        groupCooldown[groupId] = nil
        progressRect:destroy()
    end
end

function onSpellCooldown(spellId, duration)
    local slot
    local allSlots = getAllChildrens()
    for v, k in pairs(allSlots) do
        local spell, profile, spellName = Spells.getSpellByIcon(spellId)
        if k.words == spell.words or spell.clientId and spell.clientId == k.itemId then
            slot = k
            local progressRect = slot:recursiveGetChildById('progress' .. spell.id)
            if not progressRect then
                progressRect = g_ui.createWidget('SpellProgressRect', slot)
                progressRect:setId('progress' .. spell.id)
                progressRect.item = slot
                progressRect:fill('parent')
                progressRect:setFont('verdana-11px-rounded')
            else
                progressRect:setPercent(0)
            end

            local updateFunc = function()
                updateCooldown(progressRect, duration, spell.id, 0)
            end
            local finishFunc = function()
                cooldown[spell.id] = nil
                progressRect:hide()
            end
            progressRect:setPercent(0)
            updateFunc()
            cooldown[spell.id] = duration
        end
    end
end

function onSpellGroupCooldown(groupId, duration)
    local slot
    local spellGroup = 0
    local allSlots = getAllChildrens()
    for v, k in pairs(allSlots) do
        local spell, profile, spellName
        if k.words then
            spell, profile, spellName = Spells.getSpellByWords(k.words)
        else
            if k.itemId and k.itemId > 0 then
                spell, profile, spellName = Spells.getSpellByClientId(k.itemId)
            end
        end
        if spell then
            if table.contains(spell.group, groupId) then
                local continue = false
                if not cooldown[spell.id] or cooldown[spell.id] and cooldown[spell.id] < duration then
                    local oldProgressBar = k:recursiveGetChildById('progress' .. spell.id)
                    if oldProgressBar then
                        cooldown[spell.id] = nil
                        oldProgressBar:hide()
                    end
                    continue = true
                elseif cooldown[spell.id] and cooldown[spell.id] >= duration then
                    continue = false
                end
                if continue then
                    slot = k
                    local progressRect = slot:recursiveGetChildById('progress' .. groupId)
                    if not progressRect then
                        progressRect = g_ui.createWidget('SpellProgressRect', slot)
                        progressRect:setId('progress' .. groupId)
                        progressRect.item = slot
                        progressRect:fill('parent')
                        progressRect:setFont('verdana-11px-rounded')
                    else
                        progressRect:setPercent(0)
                    end

                    local updateFunc = function()
                        updateGroupCooldown(progressRect, duration, groupId)
                    end
                    local finishFunc = function()
                        groupCooldown[groupId] = false
                        progressRect:hide()
                    end
                    progressRect:setPercent(0)
                    updateFunc()
                    groupCooldown[groupId] = true
                end
            end
        end
    end
end

function filterSpells(text)
    if #text > 0 then
        text = text:lower()

        for index, spellListLabel in pairs(spellsPanel:getChildren()) do
            if string.find(spellListLabel.name:lower(), text) or string.find(spellListLabel.words:lower(), text) then
                showSpell(spellListLabel)
            else
                hideSpell(spellListLabel)
            end
        end

    else
        for index, spellListLabel in pairs(spellsPanel:getChildren()) do
            showSpell(spellListLabel)
        end
    end
end

function hideSpell(spellListLabel)
    if spellListLabel:isVisible() then
        spellListLabel:hide()
        spellListLabel:setHeight(0)
    end
end

function showSpell(spellListLabel)
    if not spellListLabel:isVisible() then
        spellListLabel:setHeight(spellListLabel.defaultHeight)
        spellListLabel:show()
    end
end
