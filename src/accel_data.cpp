#include "accel_data.h"

#include "behavior.h"
#include "parameters.h"

void fetch_new_dataframe(std::vector<DataFrame> &frames, const sensors_vec_t new_value) {
    if (frames.size() > 4) {
        frames.erase(frames.begin());
    }
    auto xa = new_value.x;
    auto ya = new_value.y;
    auto za = new_value.z;
    auto roll = abs(atan2(ya, za) * 180.0 / PI);
    auto pitch = abs(asin(xa / sqrt(xa * xa + ya * ya + za * za)) * 180.0 / PI);
    frames.emplace_back(DataFrame{pitch, roll});
}

auto get_lowest(const std::vector<DataFrame> &frames) -> DataFrame {
    DataFrame smallest{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    for (auto frame : frames) {
        if (frame.pitch < smallest.pitch) {
            smallest.pitch = frame.pitch;
        }
        if (frame.roll < smallest.roll) {
            smallest.roll = frame.roll;
        }
    }
    Serial.print("pitch: ");
    Serial.print(smallest.pitch);
    Serial.print(" roll: ");
    Serial.println(smallest.roll);
    return smallest;
}

auto get_behavior_from_accel(const std::vector<DataFrame> &accel_data, const Difficulty &difficulty) -> Behavior {
    auto lowest_data = get_lowest(accel_data);
    if (std::max(lowest_data.pitch, lowest_data.roll) > difficulty.okey_threshold) {
        return Behavior::bad;
    }
    if (std::max(lowest_data.pitch, lowest_data.roll) > difficulty.good_threshold) {
        return Behavior::okey;
    }
    return Behavior::good;
}
auto detect_tamper(const std::vector<DataFrame> &accel_data) -> bool {
    DataFrame avg{0.0f, 0.0f};
    for (auto frame : accel_data) {
        avg.pitch += frame.pitch;
        avg.roll += frame.roll;
    }
    avg.pitch = avg.pitch / accel_data.size();
    avg.roll = avg.roll / accel_data.size();

    for (auto frame : accel_data) {
        if (abs(avg.pitch - frame.pitch) > tamper_theshold) {
            return false;
        }
        if (abs(avg.roll - frame.roll) > tamper_theshold) {
            return false;
        }
    }
    return true;
}