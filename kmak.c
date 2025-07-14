// *********************************************
// KMAK - Komodore Make
//
// Author: Marc-Daniel DALEBA
// Date:
// Description:
//   KMAK is a simple build system inspired by
//   Makefile with task support.
//
//
//
//
//
// *********************************************
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_GLOBAL_VARS   200
#define MAX_LOCAL_VARS    100
#define MAX_TASK_NAME_LEN 32
#define MAX_TASK_LINES    64
#define MAX_TASKS         64
#define INVALID           -1

typedef struct _KMK_VAR {
  char name[128];
  char value[256];
} KMK_Var;

typedef struct _KMK_TASK {
  char name[MAX_TASK_NAME_LEN];
  char *lines[MAX_TASK_LINES];
  int line_count;
} KMK_Task;

void usage(char *app)
{
  printf("usage: %s InputFile <Task>\n", app);
}

void trim_left(char **str);
char *start_with_word(char *line, char *word);
void ignore_comments(char *line);
char *get_variable_value(char *name);
int process_variable_substitution(char *line);
int is_task(char *task);
int parse_variable_definition(char *line);
int parse_task(char *line);
int parse_print(char *line);
int parse_call(char *line);
int parse_cmd(char *line);
int run_task(char *name);

char *gFileContent = NULL;
KMK_Var gLocalVariables[MAX_LOCAL_VARS];
KMK_Var gGlobalVariables[MAX_GLOBAL_VARS];
KMK_Task gTasks[MAX_TASKS];
KMK_Task *gCurrentTask;
int gLocalVarsCount = 0;
int gGlobalVarsCount = 0;
int gTasksCount = 0;
int gInsideTask = 0;
int gLine = 1;

void terminate(void)
{
  free(gFileContent);
  for (int i = 0; i < gTasksCount; ++i) {
    for (int j = 0; j < gTasks[i].line_count; ++i) {
      free(gTasks[i].lines[j]);
    }
  }
  ExitProcess(INVALID);
}

void run_err(const char *error)
{
  printf("Runtime Error: ");
  printf(error);
  terminate();
}

void perr(const char *error)
{
  printf("error: line %d: ", gLine);
  printf(error);
  terminate();
}

int char_in_tok(char c, char *toks)
{
  size_t toks_len = strlen(toks);
  for (size_t i = 0; i < toks_len; ++i)
    if (c == toks[i])
      return 1;
  return 0;
}

char *find_next(char **p, char *src, char *toks)
  // /!\ desctructive for the source `src`.
{
  char *st = src;
  char *ptr = src;
  
  if (!(*st)) {
    return NULL;
  }
  
  while (*ptr) {
    if (char_in_tok(*ptr, toks)) {
      *ptr = '\0';
      ptr++;
      break;
    }
    ptr++;
  }
  *p = ptr;
  return st;
}

int main(int argc, char **argv)
{
  if (argc < 3) {
    printf("error: No input file provided.\n");
    usage(argv[0]);
    return -1;
  }
  
  FILE *fp = fopen(argv[1], "r");
  if (!fp) {
    printf("error: Couldn't open %s\n", argv[1]);
    usage(argv[0]);
    return -1;
  }
  
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  
  gFileContent = malloc(len + 1);
  memset(gFileContent, '\0', len + 1);
  if (!gFileContent) {
    run_err("Could not allocate input file data.\n");
    fclose(fp);
    return -1;
  }
  fread(gFileContent, 1, len, fp);
  fclose(fp);
  
  gLine = 0;
  gInsideTask = 0;
  char *p, *p2;
  char *line = find_next(&p, gFileContent, "\r\n");
  while (line) {
    char line_buf[1024];
    strncpy(line_buf, line, sizeof(line_buf) - 1);
    line_buf[sizeof(line_buf) - 1] = '\0';
    char *ln = line_buf;
    
    gLine++;
    ignore_comments(ln);
    if (gInsideTask) {
      if ((*ln) && (*ln != ' ') && (*ln != '\t')) {
        gCurrentTask = NULL;
        gInsideTask = 0;
      }
    }
    trim_left(&ln);
    process_variable_substitution(ln);
    if (parse_task(ln)) {
      line = find_next(&p2, p, "\r\n");
      p = p2;
      continue;
    } else
    if (parse_variable_definition(ln)) {
    
    }
    
    if (gInsideTask && gCurrentTask) {
      gCurrentTask->lines[gCurrentTask->line_count] = _strdup(ln);
      gCurrentTask->line_count++;
    }
    line = find_next(&p2, p, "\r\n");
    p = p2;
  }
  
  free(gFileContent);
  
  if (argc >= 3) {
    run_task(argv[2]);
  }
  
  for (int i = 0; i < gTasksCount; ++i) {
    for (int j = 0; j < gTasks[i].line_count; ++j) {
      free(gTasks[i].lines[j]);
    }
  }
  return 0;
}

void trim_left(char **str)
{
  while (isspace(**str) && **str) {
    (*str)++;
  }
}

char *start_with_word(char *line, char *word)
{
  size_t len = strlen(word);
  if (strncmp(line, word, len))
    return NULL;
  
  return (char *)(line + len);
}

void ignore_comments(char *line)
{
  char *ptr;
  
  ptr = line;
  while (ptr = strchr(line, '#')) {
    if (ptr != line) {
      if (*(ptr-1) == '\\') {
        ptr++;
        continue;
      }
    }
    *ptr = '\0';
    return;
  }
}

char *get_variable_value(char *name)
{
  for (int i = 0; i < gGlobalVarsCount; ++i)
    if (!strcmp(gGlobalVariables[i].name, name))
      return gGlobalVariables[i].value;
  return NULL;
}

int is_task(char *task)
{
  for (int i = 0; i < gTasksCount; ++i)
    if (!strcmp(gTasks[i].name, task))
      return i;
  return -1;
}

int process_variable_substitution(char *line)
{
  int i;
  char name[128];
  char buffer[1024];
  char *src = line;
  char *dst = buffer;

  while (*src) {
    if (*src == '$' && *(src+1) == '(') {
      src += 2; // skip "$("

      i = 0;
      while (*src && *src != ')' && i < sizeof(name) - 1) {
        name[i++] = *src++;
      }
      name[i] = '\0';

      if (*src != ')') {
        perr("missing closing ')' in variable\n");
      }
      src++; // skip ')'

      char *value = get_variable_value(name);
      if (!value) {
        perr("referenced an undefined variable\n");
      }

      while (*value) {
        *dst++ = *value++;
      }
    } else {
      *dst++ = *src++;
    }
  }

  *dst = '\0';
  strcpy(line, buffer);
  return 0;
}

int parse_variable_definition(char *line){
  char *ptr, *name_trim_ptr;
  char *name, *value;
  
  ptr = strchr(line, '=');
  if (!ptr)
    return 0;
  
  if (gGlobalVarsCount >= MAX_GLOBAL_VARS) {
    perr("too many global variables.\n");
  }
  
  name = line;
  value = ptr + 1;
  *ptr = '\0';
  
  trim_left(&name);
  trim_left(&value);
  
  // trim right the name
  name_trim_ptr = ptr-1;
  while (isspace(*name_trim_ptr)) {
    if (name_trim_ptr <= line) {
      run_err("parse_variable_definition() (1)\n");
      return -1;
    }
    name_trim_ptr--;
  }
  *(name_trim_ptr + 1) = '\0';
  
  strncpy(gGlobalVariables[gGlobalVarsCount].name, name, 128);
  strncpy(gGlobalVariables[gGlobalVarsCount].value, value, 256);
  gGlobalVarsCount++;
  return 1;
}

int parse_task(char *line)
{
  char *ptr;
  char *name;
  KMK_Task *current_task = NULL;
  
  ptr = start_with_word(line, "task");
  if (!ptr)
    return 0;
  
  name = ptr;
  trim_left(&name);
  
  gCurrentTask = &gTasks[gTasksCount];
  strncpy(gCurrentTask->name, name, MAX_TASK_NAME_LEN);
  
  gInsideTask = 1;
  gTasksCount++;
  return 1;
}

int parse_print(char *line)
{
  char *ptr;
  
  ptr = start_with_word(line, "print");
  if (!ptr)
    return 0;
  
  trim_left(&ptr);
  printf("%s\n", ptr);
  return 1;
}

int parse_call(char *line)
{
  char *ptr;
  
  ptr = start_with_word(line, "call");
  if (!ptr)
    return 0;
  
  trim_left(&ptr);
  run_task(ptr);
  return 1;
}

int run_command(const char *cmdline)
{
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  DWORD exit_code;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

  char sn[1024];
  snprintf(sn, 1024, "cmd /C %s", cmdline);
  if (!CreateProcessA(NULL, (LPSTR)sn, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
    run_err("run_command: CreateProcess() failed\n");
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  GetExitCodeProcess(pi.hProcess, &exit_code);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return exit_code;
}

int parse_cmd(char *line)
{
  char *ptr;
  char *command;
  ptr = start_with_word(line, "cmd");
  if (!ptr)
    return 0;

  command = ptr;
  trim_left(&command);

  if (process_variable_substitution(command) < 0) {
    run_err("failed to process variable substitution\n");
    return 0;
  }
  printf("[CMD] %s\n", command);
  
  int res = run_command(command);
  if (res != 0) {
    run_err("command failed\n");
    return 0;
  }
  return 1;
}

int run_task(char *name)
{
  int task_idx;
  KMK_Task *task;
  
  task_idx = is_task(name);
  if (task_idx < 0) {
    printf("error: task %s isn't defined.\n", name);
    return -1;
  }
  
  task = &gTasks[task_idx];
  for (int i = 0; i < task->line_count; ++i) {
    char *line = task->lines[i];
    if (parse_print(line)) {

    } else
    if (parse_call(line)) {
    
    } else
    if (parse_cmd(line)) {
    
    }
  }
  return 0;
}