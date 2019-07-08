/* Start Header -------------------------------------------------------
Copyright (C) 2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior written
consent of DigiPen Institute of Technology is prohibited.
File Name: asset_loading.cpp
Purpose: This file reads and loads OBJ files into memory
Language: C++
Platform: Windows 10 OpenGL 4.0
Project: michael.fritz_CS300_1
Author: Michael Fritz, michael.fritz, 180000117
Creation date: 5/31/2019
End Header --------------------------------------------------------*/

#include "asset_loading.h"

#include <stdio.h>
#include <string.h>

static bool is_digit(char c)
{
  if(c >= '0' && c <= '9') return true;
  return false;
}

void load_obj(const char *path_to_obj, std::vector<v3> *vertices, std::vector<v2> *texture_coords, std::vector<v3> *normals, std::vector<unsigned> *indices)
{
  FILE *file = fopen(path_to_obj, "rb");
  if(!file)
  {
    vertices = 0;
    texture_coords = 0;
    normals = 0;
    indices = 0;
    printf("Could not find obj file %s\n", path_to_obj);
    return;
  }

  unsigned file_size;
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // + 1 for null termination
  char *data = new char[file_size + 1];
  fread(data, file_size, 1, file);
  data[file_size] = 0;

  const char * delims = " \t\n\r";

  // Tokenize the file
  char *tokens = strtok(data, delims);

  unsigned lines = 0;
  while(tokens != 0)
  {
    lines++;
    bool read_next_token = true;
    if(strcmp(tokens, "v") == 0)
    {
      // Vertices have format v 0.0 1.0 2.0
      v3 vertex;

      tokens = strtok(NULL, delims);
      vertex.x = (float)atof(tokens);
      tokens = strtok(NULL, delims);
      vertex.y = (float)atof(tokens);
      tokens = strtok(NULL, delims);
      vertex.z = (float)atof(tokens);

      vertices->push_back(vertex);
    }
    else if(strcmp(tokens, "f") == 0)
    {
      // Faces have format f 1 2 3 ...
      unsigned index0;
      unsigned index1;
      unsigned index2;

      tokens = strtok(NULL, delims);
      index0 = atoi(tokens); 
      tokens = strtok(NULL, delims);
      index1 = atoi(tokens);
      tokens = strtok(NULL, delims);
      index2 = atoi(tokens);

      indices->push_back(index0 - 1); // - 1 for OBJ files reading indices starting at 1 (not 0)
      indices->push_back(index1 - 1);
      indices->push_back(index2 - 1);

      read_next_token = false;
      tokens = strtok(NULL, delims);

      while(tokens != 0 && is_digit(tokens[0]))
      {
        /*
        index0 = index1;
        index1 = index2;
        index2 = atoi(tokens);
        */

        index0 = index0;
        index1 = index2;
        index2 = atoi(tokens);;

        indices->push_back(index0 - 1);
        indices->push_back(index1 - 1);
        indices->push_back(index2 - 1);
        
        tokens = strtok(NULL, delims);
      }
    }

    if(read_next_token)
    {
      tokens = strtok(NULL, delims);
    }
  }

  delete [] data;
}

void normalize_mesh(std::vector<v3> *in_vertices)
{
  std::vector<v3> &vertices = *in_vertices;
  // Get the min and max values for each axis
  float min_x = INFINITY;
  float max_x = -INFINITY;
  float min_y = INFINITY;
  float max_y = -INFINITY;
  float min_z = INFINITY;
  float max_z = -INFINITY;
  v3 sum_points = {0.0f, 0.0f, 0.0f};
  for(unsigned i = 0; i < vertices.size(); i++)
  {
    min_x = min(min_x, vertices[i].x);
    max_x = max(max_x, vertices[i].x);
    min_y = min(min_y, vertices[i].y);
    max_y = max(max_y, vertices[i].y);
    min_z = min(min_z, vertices[i].z);
    max_z = max(max_z, vertices[i].z);

    sum_points += vertices[i];
  }

  // Find center of model
  sum_points /= (float)vertices.size();

  // Get the max difference in an axis
  float diff_x = max_x - min_x;
  float diff_y = max_y - min_y;
  float diff_z = max_z - min_z;

  float max_diff = max(diff_x, max(diff_x, diff_y));

  // Move vertices to center and scale down
  for(unsigned i = 0; i < vertices.size(); i++)
  {
    // Move centroid to origin
    vertices[i] -= sum_points;

    // Scale down to between -1 and 1
    vertices[i] = (vertices[i] / max_diff) * 2.0f;
  }
}
