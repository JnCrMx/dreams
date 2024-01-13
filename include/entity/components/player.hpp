#pragma once

namespace entity::components
{
    struct player
    {
        double walkingSpeed = 2.0;

        double motionForward;
        double motionSideward;
    };
}
