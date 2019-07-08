#include "logging.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <Windows.h> // OutputDebugString

//------------------------------------------------------------------------------
// Private Variables:
//------------------------------------------------------------------------------

//static FILE *logging_file;

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------

char *file_name(char *path)
{
  char *name = strrchr(path, '\\') + 1;
  if(name == 0)
  {
    return 0;
  }

  return name;
}

void log_file_fn(const char *message, ...)
{
  FILE *logging_file = fopen("log.txt", "at");
  if(!logging_file) return;

  va_list argPtr;
  va_start(argPtr, message);

  if(logging_file == 0)
  {
    printf("Message log file is not open\n");
    return;
  }

  //vfprintf(logging_file, message, argPtr);
  //vprintf(message, argPtr);
  
  char buffer[512];
  vsnprintf(buffer, 510, message, argPtr); // 510 for newline and null characters
  int message_length = strlen(buffer);
  buffer[message_length] = '\n';
  buffer[message_length + 1] = 0;

  fprintf(logging_file, buffer);

  OutputDebugStringA(buffer);


  fclose(logging_file);
  va_end(argPtr);
}

// Initializes the log files
void init_logging()
{
#if 0
  logging_file = fopen("log.txt", "wt");
  if(logging_file == 0)
  {
    printf("Could not open logging file log file\n");
    return;
  }
#endif

  // Clear previous
  FILE *logging_file = fopen("log.txt", "wt");
  if(logging_file == 0)
  {
    printf("Could not open logging file log file\n");
    return;
  }
  //fprintf(logging_file, "\n");
  fclose(logging_file);
}


// Closes the log files
void exit_logging()
{
#if 0
  fclose(logging_file);
#endif
}

