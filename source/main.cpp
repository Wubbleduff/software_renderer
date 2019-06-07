#include "software_renderer.h"

#include <stdio.h>

int main()
{
  init_renderer();

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
