#include "software_renderer.h"


#include "my_math.h"

#include <stdio.h>

static void printmat4(mat4 m)
{
  for(int i = 0; i < 4; i++)
  {
    printf("| ");

    for(int j = 0; j < 4; j++)
    {
      printf("%f ", m[i][j]);
    }
    printf("|\n");
  }
}

int main()
{

  mat4 m = 
  {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  mat4 n = 
  {
    10.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 5.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, -5.0f
  };

  mat4 result = n * m;




  init_renderer();

  // Make model

  while(true)
  {
    // Update
    poll_events();

    // Draw
    render();

    swap_buffers();
  }

  // Cleanup

  return 0;
}
