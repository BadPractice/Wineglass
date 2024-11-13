#pragma once
#include <Adafruit_MPU6050.h>

#include <vector>

#include "behavior.h"
#include "parameters.h"

struct DataFrame {
    double pitch;
    double roll;
};

void fetch_new_dataframe(std::vector<DataFrame> &frames, const sensors_vec_t new_value);
auto get_lowest(const std::vector<DataFrame> &frames) -> DataFrame;

auto get_behavior_from_accel(const std::vector<DataFrame> &accel_data, const Difficulty &difficulty) -> Behavior;
auto detect_tamper(const std::vector<DataFrame> &accel_data) -> bool;

void callibrate();
