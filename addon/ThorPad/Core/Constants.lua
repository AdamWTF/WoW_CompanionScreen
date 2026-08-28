local addonName, ThorPad = ...
_G.ThorPad = ThorPad

ThorPad.Constants = {
    DB_VERSION = 3,
    DEFAULT_PORT = 18423,
    MEDIA = [[Interface\AddOns\ThorPad\media\]],
    CONTROLLER_LAYERS = { "default", "l2", "r2", "l2r2" },
    CONTROLLER_LAYER_LABELS = { default = "Default", l2 = "L2", r2 = "R2", l2r2 = "L2 + R2" },
    CONTROLLER_CONTROLS = { "dpad_up", "dpad_down", "dpad_left", "dpad_right", "south", "east", "west", "north" },
    CONTROLLER_CONTROL_LABELS = {
        dpad_up = "D-pad Up", dpad_down = "D-pad Down", dpad_left = "D-pad Left", dpad_right = "D-pad Right",
        south = "South", east = "East", west = "West", north = "North",
    },
    CONTROLLER_BUTTON_PREFIXES = {
        default = "ThorPadControllerDefault", l2 = "ThorPadControllerL2_", r2 = "ThorPadControllerR2_", l2r2 = "ThorPadControllerL2R2_",
    },
    -- The three horizontal Blizzard bars: MainMenuBar (1-12),
    -- MultiBarBottomLeft (49-60), and MultiBarBottomRight (61-72).
    CONTROLLER_ACTIONS = {
        default = { 1, 2, 3, 4, 5, 6, 7, 8 },
        l2 = { 9, 10, 11, 12, 49, 50, 51, 52 },
        r2 = { 53, 54, 55, 56, 57, 58, 59, 60 },
        l2r2 = { 61, 62, 63, 64, 65, 66, 67, 68 },
    },
    SECOND_SCREEN_SLOT_COUNT = 24,
    SECOND_SCREEN_FIRST_ACTION = 25,
    SECOND_SCREEN_LAST_ACTION = 48,
}
