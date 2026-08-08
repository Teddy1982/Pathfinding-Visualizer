#pragma once

#include <array>

const int CLOSED_NODE = 1;
const int OPEN_NODE = 2;
const int CURRENT_NODE = 3;

const int OBSTACLE_NODE = 1;
const int START_NODE = 2;
const int END_NODE = 3;

const int SET_OBSTACLE_NODE = 1;
const int SET_START_NODE = 2;
const int SET_END_NODE = 3;
const int SET_ERASE_NODE = 4;
const int SET_ERASE_ALL_NODES = 5;
const int SET_ERASE_ALL_PATH_NODES = 6;
const int SET_SEARCH_PATH = 7;
const int SET_SEARCH_PATH_STEP = 8;
const int SET_SEARCH_PATH_PLAY = 9;

const int MANHATTAN_DISTANCE = 0;
const int EUCLID_DISTANCE = 1;

const int SEARCH_6_DIRECTIONS = 0;
const int SEARCH_26_DIRECTIONS = 1;
const int SEARCH_4_DIRECTIONS = 2;
const int SEARCH_8_DIRECTIONS = 3;

const std::array<float, 4> WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
const std::array<float, 4> RED = { 1.0f, 0.0f, 0.0f, 1.0f };
const std::array<float, 4> GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
const std::array<float, 4> YELLOW = { 1.0f, 1.0f, 0.0f, 1.0f };
const std::array<float, 4> CLEAR = { 0.0f, 0.0f, 0.0f, 0.0f };
const std::array<float, 4> BLACK = { 0.0f, 0.0f, 0.0f, 1.0f };
const std::array<float, 4> BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
const std::array<float, 4> PINK = { 1.0f, 0.0f, 1.0f, 1.0f };

const std::array<float, 4> HALF_SCALE = { 0.5f, 0.5f, 0.5f, 1.0f };
const std::array<float, 4> FULL_SCALE = { 1.0f, 1.0f, 1.0f, 1.0f };
const std::array<float, 4> ZERO_SCALE = { 0.0f, 0.0f, 0.0f, 1.0f };
