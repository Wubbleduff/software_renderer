#include "profiling.h"

#include <time.h>
#include <map>
#include <vector>
#include <assert.h>

#include <stdio.h>

struct TimeBlock
{
  const char *name;
  f32 total_time;
  u32 counts;

  f32 start_time;

  TimeBlock()
  {
    name = "";
    total_time = 0.0f;
    counts = 0;
    start_time = 0.0f;
  }

  TimeBlock(const char *in_name)
  {
    name = in_name;
    total_time = 0.0f;
    counts = 0;
    start_time = 0.0f;
  }
};

static std::map<std::string, TimeBlock> time_block_map;

static std::vector<std::string> time_block_stack;

void time_block(const char *name)
{
  std::string name_string = name;

  std::map<std::string, TimeBlock>::iterator it;
  it = time_block_map.find(name_string);

  if(it == time_block_map.end())
  {
    TimeBlock new_block = TimeBlock(name);
    new_block.start_time = clock();
    time_block_map[name_string] = new_block;
  }
  else
  {
    it->second.start_time = (f32)clock();
  }

  time_block_stack.push_back(name_string);
}

void end_time_block()
{
  if(time_block_stack.size() == 0)
  {
    assert(0);
    return;
  }

  std::string block_name = time_block_stack.back();
  time_block_stack.pop_back();

  TimeBlock &block = time_block_map[block_name];

  f32 end_time = (f32)clock();
  block.total_time += end_time - block.start_time;
  block.counts++;
}

void dump_profile_info()
{
  FILE *file = fopen("profile.txt", "wt");

  for(auto it : time_block_map)
  {
    TimeBlock &block = it.second;
    float average_time = block.total_time / block.counts;
    fprintf(file, "%s\n", block.name);
    fprintf(file, "  average time: %f ms\n", average_time);
    fprintf(file, "\n");
  }

  fclose(file);
}

