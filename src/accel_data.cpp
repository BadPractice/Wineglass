#pragma once
#include "accel_data.h"

void fetch_new_dataframe(std::vector<DataFrame> &frames, const sensors_vec_t new_value) {
    if (frames.size() > 4) {
        frames.erase(frames.begin());
    }
    auto xa = new_value.x;
    auto ya = new_value.y;
    auto za = new_value.z;
    auto roll = abs(atan2(ya, za) * 180.0 / PI);
    auto pitch = abs(atan2(-xa, sqrt(ya * ya + za * za)) * 180.0 / PI);
    frames.emplace_back(DataFrame{pitch, roll});
}