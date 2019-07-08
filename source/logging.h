#pragma once

char *file_name(char *path);

// Logs a message to the message file and the command prompt WITH a newline character.
// For example: Use this to record the time since last frame.
//#define log_file(formatString, ...) (log_file_fn("%s:%d - " formatString, file_name(__FILE__), __LINE__, __VA_ARGS__))
#define log_file(formatString, ...) (log_file_fn(formatString, __VA_ARGS__))

void log_file_fn(const char *message, ...);

// Initializes the log files
void init_logging();

// Closes the log files
void exit_logging();

