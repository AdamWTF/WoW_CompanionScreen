local addonName, ThorPad = ...

ThorPad.ActionSlots = {
    layers = {
        { 1, 2, 3, 4, 5, 6, 7, 8 },
        { 9, 10, 11, 12, 49, 50, 51, 52 },
        { 53, 54, 55, 56, 57, 58, 59, 60 },
        { 61, 62, 63, 64, 65, 66, 67, 68 },
        { 69, 70, 71, 72, 0, 0, 0, 0 },
    },
}

function ThorPad.ActionSlots:GetSlot(layer, buttonIndex)
    local buttons = self.layers[layer]
    return buttons and buttons[buttonIndex] or 0
end
