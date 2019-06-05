#pragma once

#include "my_math.h" // v2, v3

#include <vector>

void load_obj(const s8 *path_to_obj, std::vector<v3> *vertices, std::vector<v2> *texture_coords, std::vector<v3> *normals, std::vector<u32> *indices);

void normalize_mesh(std::vector<v3> *vertices);
