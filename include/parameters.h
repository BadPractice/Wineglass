#pragma once

struct Difficulty {
    float good_threshold;
    float okey_threshold;
};

const Difficulty easy_difficulty{6.0f, 8.0f};
const Difficulty medium_difficulty{4.0f, 6.0f};
const Difficulty hard_difficulty{2.0f, 4.0f};
const float tamper_theshold = 0.3f;

const int tamper_counter_limit = 30;