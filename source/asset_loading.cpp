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
      vertex.x = atof(tokens);
      tokens = strtok(NULL, delims);
      vertex.y = atof(tokens);
      tokens = strtok(NULL, delims);
      vertex.z = atof(tokens);

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

