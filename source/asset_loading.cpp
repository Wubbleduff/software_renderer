#include "asset_loading.h"

#include <stdio.h>
#include <string.h>

void load_obj(const s8 *path_to_obj, std::vector<v3> *vertices, std::vector<v2> *texture_coords, std::vector<v3> *normals, std::vector<u32> *indices)
{
  FILE *file = fopen(path_to_obj, "rt");
  if(!file)
  {
    vertices = 0;
    texture_coords = 0;
    normals = 0;
    indices = 0;
    printf("Could not find obj file %s\n", path_to_obj);
    return;
  }

  u32 file_size;
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // + 1 for null termination
  s8 *data = new s8[file_size + 1];
  fread(data, file_size, 1, file);
  data[file_size] = '\0';

  const s8 * delims = " /\t\n";

  s8 *tokens = strtok(data, delims);

  while(tokens != 0)
  {
    if(strcmp(tokens, "v") == 0)
    {
      v3 vertex;

      tokens = strtok(NULL, delims);
      vertex.x = (f32)atof(tokens);
      tokens = strtok(NULL, delims);
      vertex.y = (f32)atof(tokens);
      tokens = strtok(NULL, delims);
      vertex.z = (f32)atof(tokens);

      vertices->push_back(vertex);
    }
    else if(strcmp(tokens, "f") == 0)
    {
      u32 vertex_index;

      tokens = strtok(NULL, delims);
      vertex_index = atoi(tokens) - 1;
      indices->push_back(vertex_index);
      tokens = strtok(NULL, delims);
      tokens = strtok(NULL, delims);

      tokens = strtok(NULL, delims);
      vertex_index = atoi(tokens) - 1;
      indices->push_back(vertex_index);
      tokens = strtok(NULL, delims);
      tokens = strtok(NULL, delims);

      tokens = strtok(NULL, delims);
      vertex_index = atoi(tokens) - 1;
      indices->push_back(vertex_index);
      tokens = strtok(NULL, delims);
      tokens = strtok(NULL, delims);
    }

    tokens = strtok(NULL, delims);
  }

  delete [] data;
}

void normalize_mesh(std::vector<v3> *vertices)
{
  // Get the min and max values for each axis
  float min_x = INFINITY;
  float max_x = -INFINITY;
  float min_y = INFINITY;
  float max_y = -INFINITY;
  float min_z = INFINITY;
  float max_z = -INFINITY;
  v3 sum_points = {0.0f, 0.0f, 0.0f};
  for(unsigned i = 0; i < vertices->size(); i++)
  {
    min_x = min(min_x, (*vertices)[i].x);
    max_x = max(max_x, (*vertices)[i].x);
    min_y = min(min_y, (*vertices)[i].y);
    max_y = max(max_y, (*vertices)[i].y);
    min_z = min(min_z, (*vertices)[i].z);
    max_z = max(max_z, (*vertices)[i].z);

    sum_points += (*vertices)[i];
  }

  // Find center of model
  sum_points /= (float)vertices->size();

  // Get the max difference in an axis
  float diff_x = max_x - min_x;
  float diff_y = max_y - min_y;
  float diff_z = max_z - min_z;

  float max_diff = max(diff_x, max(diff_x, diff_y));

  // Move vertices to center and scale down
  for(unsigned i = 0; i < vertices->size(); i++)
  {
    // Move centroid to origin
    (*vertices)[i] -= sum_points;

    // Scale down to between -1 and 1
    (*vertices)[i] = ((*vertices)[i] / max_diff) * 2.0f;
  }
}