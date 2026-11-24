// ui/views/valves/ValvesDemo.h: package E's valvetrain fields for the demo document (ValvesDemo.cpp), until U1's demo
// hook takes them. The values are a picture's: a 268/274 degree cam on 108/116 centrelines, 12.4 and 11.9 mm of lift,
// an intake phaser of 40 degrees, a second stage of 296/300 degrees from 5500 rpm, cylinders 2, 3, 6 and 7 able to rest.
#pragma once

#include "kit/bind/Fields.h"

#include <vector>

namespace tee::ui
{
    std::vector<FieldInfo> valvesDemoFields();
}
