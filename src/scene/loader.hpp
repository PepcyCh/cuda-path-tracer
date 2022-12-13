#pragma once

#include <filesystem>

#include "core.hpp"

bool LoadObjScene(Scene &scene, const std::filesystem::path &path, int bsdf_type);

bool LoadJsonScene(Scene &scene, const std::filesystem::path &path);

bool LoadXmlScene(Scene &scene, const std::filesystem::path &path);
