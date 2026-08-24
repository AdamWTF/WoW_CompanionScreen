local addonName, ThorPad = ...

ThorPad.Config = {
    mediaPath = [[Interface\AddOns\ThorPad\media\]],
    glyphScale = 0.48,
    overlayCellSize = 50,
    settingsCellSize = 64,
    layerPollInterval = 0.10,
    dockReconcileInterval = 0.50,
    minimap = { defaultAngle = 225, radius = 80 },
    layerNames = { "Base", "L1", "L2", "R1", "R2" },
    glyphs = {
        "CP_R_DOWN", "CP_R_RIGHT", "CP_R_LEFT", "CP_R_UP",
        "CP_L_UP", "CP_L_DOWN", "CP_L_LEFT", "CP_L_RIGHT",
    },
    overlayPositions = {
        { 370, -124 }, { 420, -74 }, { 320, -74 }, { 370, -24 },
        { 68, -24 }, { 68, -124 }, { 18, -74 }, { 118, -74 },
    },
    settingsPositions = {
        { 292, -235 }, { 350, -177 }, { 234, -177 }, { 292, -119 },
        { 70, -119 }, { 70, -235 }, { 12, -177 }, { 128, -177 },
    },
    minimapTexture = [[Interface\Icons\INV_Misc_EngGizmos_03]],
}
