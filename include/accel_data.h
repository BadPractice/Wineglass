#include <Adafruit_MPU6050.h>

#include <vector>

struct DataFrame {
    double pitch;
    double roll;
};

// std::vector<DataFrame> accel_data;

void fetch_new_dataframe(std::vector<DataFrame> &frames, const sensors_vec_t new_value);