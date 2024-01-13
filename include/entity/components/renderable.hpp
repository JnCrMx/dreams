#pragma once

namespace entity::components
{
    struct renderable
    {
        bool shadowCaster = true;
        bool shadowCatcher = true;
    };
}
