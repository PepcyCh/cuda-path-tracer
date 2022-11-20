#pragma once

#include <filesystem>

#include "core.hpp"

bool LoadObjScene(Scene &scene, const std::filesystem::path &path);

bool LoadJsonScene(Scene &scene, const std::filesystem::path &path);
