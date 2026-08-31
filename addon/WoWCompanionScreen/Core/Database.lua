local addonName, WCS = ...

WCS.Database = {}

local function tableValue(parent, key)
    if type(parent[key]) ~= "table" then parent[key] = {} end
    return parent[key]
end

local function booleanDefault(parent, key, value)
    if type(parent[key]) ~= "boolean" then parent[key] = value end
end

local function numberDefault(parent, key, value)
    if type(parent[key]) ~= "number" then parent[key] = value end
end

function WCS.Database:Initialize()
    if type(WCSDB) ~= "table" then WCSDB = {} end
    WCS.Migrations:Run(WCSDB)

    local controller = tableValue(WCSDB, "controller")
    booleanDefault(controller, "enabled", true)
    booleanDefault(controller, "uiNavigation", true)
    if controller.menuConfirm ~= "south" and controller.menuConfirm ~= "east" then controller.menuConfirm = "south" end
    if controller.glyphFamily ~= "auto" and controller.glyphFamily ~= "xbox" and controller.glyphFamily ~= "playstation" and controller.glyphFamily ~= "aynthor" then controller.glyphFamily = "auto" end
    local assignments = tableValue(controller, "assignments")
    for _, layer in ipairs(WCS.Constants.CONTROLLER_LAYERS) do
        local values = tableValue(assignments, layer)
        for control, assignment in pairs(values) do
            if type(assignment) ~= "table" or assignment.type ~= "system" or type(assignment.action) ~= "string" then values[control] = nil end
        end
    end

    local secondScreen = tableValue(WCSDB, "secondScreen")
    booleanDefault(secondScreen, "enabled", true)
    booleanDefault(secondScreen, "reduceUI", true)

    local display = tableValue(WCSDB, "display")
    numberDefault(display, "actionScale", 1)
    numberDefault(display, "glyphScale", 1)
    display.actionScale = math.max(.75, math.min(1.35, display.actionScale))
    display.glyphScale = math.max(.75, math.min(1.35, display.glyphScale))

    local minimap = tableValue(WCSDB, "minimap")
    numberDefault(minimap, "angle", 225)
    booleanDefault(minimap, "hidden", false)
    booleanDefault(WCSDB, "debug", false)
end

function WCS.Database:Reset()
    WCSDB = nil
    self:Initialize()
end
