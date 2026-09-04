#include "GameInput.hpp"

#include <cassert>

int main()
{
    unsigned view = 1;
    for (unsigned expected = 2; expected <= 5; ++expected)
    {
        view = wcs_gamepad::AdvanceCameraView(view);
        assert(view == expected);
    }
    assert(wcs_gamepad::AdvanceCameraView(view) == 1);
}
