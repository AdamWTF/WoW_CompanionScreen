local addonName, ThorPad = ...

ThorPad.Glyphs = {}

local directional = {
    dpad_up = "glyphs\\generic_up", dpad_down = "glyphs\\generic_down",
    dpad_left = "glyphs\\generic_left", dpad_right = "glyphs\\generic_right",
}
local playstation = { a = "glyphs\\ps_down", b = "glyphs\\ps_right", x = "glyphs\\ps_left", y = "glyphs\\ps_up" }
local xbox = { a = "glyphs\\xbox_down", b = "glyphs\\xbox_right", x = "glyphs\\xbox_left", y = "glyphs\\xbox_up" }
-- Thor uses Nintendo-style physical legends. The positional filenames make the
-- intended physical button explicit while controls remain semantic WoW inputs.
local aynthor = { a = "glyphs\\thor_down", b = "glyphs\\thor_right", x = "glyphs\\thor_left", y = "glyphs\\thor_up" }
local modifiers = { l2 = "generic_l2", r2 = "generic_r2", l3 = "generic_l3", r3 = "generic_r3" }
local fallback = { dpad_up = "UP", dpad_down = "DN", dpad_left = "LT", dpad_right = "RT", a = "A", b = "B", x = "X", y = "Y" }

function ThorPad.Glyphs:Get(control)
    if directional[control] then return ThorPad.Constants.MEDIA .. directional[control], fallback[control], 1, 1, 1 end
    local family = ThorPadDB.controller.glyphFamily
    if family == "playstation" and playstation[control] then
        return ThorPad.Constants.MEDIA .. playstation[control], fallback[control], 1, 1, 1
    end
    local glyphs = family == "aynthor" and aynthor or xbox
    if glyphs[control] then return ThorPad.Constants.MEDIA .. glyphs[control], fallback[control], 1, 1, 1 end
    return nil, fallback[control], .9, .9, .9
end

function ThorPad.Glyphs:GetModifier(modifier)
    if modifiers[modifier] then return ThorPad.Constants.MEDIA .. "glyphs\\" .. modifiers[modifier] end
end
