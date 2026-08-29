local source = assert(arg[1], "UINavigation.lua path required")
local sourceFile = assert(io.open(source, "rb"))
local sourceText = sourceFile:read("*a"); sourceFile:close()
assert(not sourceText:find(":HookScript(", 1, true), "UI navigation must not hook protected Blizzard frame scripts")
assert(not sourceText:find("ToggleGameMenu", 1, true), "UI navigation must not invoke the protected game-menu toggle from Lua")
local WCS = {}
assert(loadfile(source))("WCS", WCS)
local navigation = assert(WCS.UINavigation)

local function candidate(x, y, order) return { x = x, y = y, order = order } end
local center = candidate(100, 100, 1)
local up, down = candidate(100, 160, 2), candidate(100, 40, 3)
local left, right = candidate(40, 100, 4), candidate(160, 100, 5)
local cross = { center, up, down, left, right }
local cases = {
    { candidates = cross, direction = "up", expected = up },
    { candidates = cross, direction = "down", expected = down },
    { candidates = cross, direction = "left", expected = left },
    { candidates = cross, direction = "right", expected = right },
    { candidates = { center, left }, direction = "right", expected = nil },
}
for _, case in ipairs(cases) do assert(navigation:FindDirectional(case.candidates, center, case.direction) == case.expected) end

local aligned = candidate(180, 100, 2)
local diagonal = candidate(130, 140, 3)
assert(navigation:FindDirectional({ center, aligned, diagonal }, center, "right") == aligned)

local tieFirst = candidate(150, 110, 2)
local tieSecond = candidate(150, 90, 3)
assert(navigation:FindDirectional({ center, tieSecond, tieFirst }, center, "right") == tieFirst)

local disabled = candidate(120, 100, 2); disabled.disabled = true
assert(navigation:FindDirectional({ center, disabled, right }, center, "right") == right)

local topLeft = candidate(20, 200, 3)
assert(navigation:TopLeft({ candidate(10, 100, 1), candidate(40, 200, 2), topLeft }) == topLeft)

local replacement = candidate(110, 100, 4)
assert(navigation:FindDirectional({ replacement, right }, center, "right") == replacement)
print("WCS UI navigation spatial tests passed")
