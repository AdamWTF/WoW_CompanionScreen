local addonName, ThorPad = ...

ThorPad.Glyphs = {}

local directional = {
    dpad_up = "glyphs\\generic_up", dpad_down = "glyphs\\generic_down",
    dpad_left = "glyphs\\generic_left", dpad_right = "glyphs\\generic_right",
}
local playstation = { south = "glyphs\\ps_down", east = "glyphs\\ps_right", west = "glyphs\\ps_left", north = "glyphs\\ps_up" }
local xbox = { south = "glyphs\\xbox_down", east = "glyphs\\xbox_right", west = "glyphs\\xbox_left", north = "glyphs\\xbox_up" }
-- Thor uses Nintendo-style physical legends. The positional filenames make the
-- intended physical button explicit while controls remain semantic WoW inputs.
local aynthor = { south = "glyphs\\thor_down", east = "glyphs\\thor_right", west = "glyphs\\thor_left", north = "glyphs\\thor_up" }
local modifiers = { l2 = "generic_l2", r2 = "generic_r2", l3 = "generic_l3", r3 = "generic_r3" }
local fallback = { dpad_up = "UP", dpad_down = "DN", dpad_left = "LT", dpad_right = "RT", south = "S", east = "E", west = "W", north = "N" }

function ThorPad.Glyphs:Get(control)
    if directional[control] then return ThorPad.Constants.MEDIA .. directional[control], fallback[control], 1, 1, 1 end
    local configured = WXLGamepadConfiguredGlyphStyle
    local family
    if configured == "PlayStation" then family = "playstation"
    elseif configured == "Xbox" then family = "xbox"
    elseif configured == "Thor" then family = "aynthor"
    else
        family = ThorPadDB.controller.glyphFamily
        if family == "auto" then
            local detected = WXLGamepadDetectedGlyphStyle
            family = detected == "PlayStation" and "playstation" or detected == "Thor" and "aynthor" or "xbox"
        end
    end
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
