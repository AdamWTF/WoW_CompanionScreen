local addonName, ThorPad = ...

ThorPad.Migrations = {}

function ThorPad.Migrations:Run(database)
    local version = tonumber(database.version) or 0

    -- Glyph artwork now uses a smaller intrinsic display scale: the new 100%
    -- is the same on-screen size as the old 75%. Preserve existing users'
    -- visual size while exposing the corrected percentage in settings.
    if version > 0 and version < 2 and type(database.display) == "table" and type(database.display.glyphScale) == "number" then
        database.display.glyphScale = database.display.glyphScale / .75
    end

    -- Version 3 introduces SavedVariables-backed System Action overrides. The
    -- database initializer creates and validates the new assignment tables;
    -- existing WoW action slots need no migration because they remain native.

    database.version = ThorPad.Constants.DB_VERSION
    return database
end
