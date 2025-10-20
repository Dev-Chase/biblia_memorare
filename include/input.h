#ifndef INPUT_H
#define INPUT_H

#include "app.h"
#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stddef.h>
#ifdef _cplusplus
extern "C" {
#endif

#define INPUT_BUFF_LEN 20

typedef struct InputOption InputOption;

typedef bool (*OptionFn)(InputOption *self, AppEnv env);
typedef void (*OptionPrintDescFn)(void);
typedef bool (*OptionInputCheckFn)(char input_buff[static INPUT_BUFF_LEN]);

typedef enum InputOptionDataType {
  RetrievedPassage,
  SavedPassage,
  BooksList,
} InputOptionDataType;

typedef union InputOptionDataUnion {
  cJSON *retrieved_passage;
  cJSON *saved_passage;
  cJSON *books_list;
} InputOptionDataUnion;

typedef struct InputOptionData {
  InputOptionDataType type;
  InputOptionDataUnion value;
} InputOptionData;

// exec_fn is run initially and then sub_options are available
typedef struct InputOption {
  OptionFn exec;
  OptionPrintDescFn print_desc;
  OptionInputCheckFn input_check;
  size_t n_sub_options;
  const InputOption **sub_options; // Array of pointers
  InputOptionData data;
} InputOption;

extern const InputOption GLOBAL_INPUT_OPTION;

// Giving Directions and Information
void input_print_options_list(
    size_t n_sub_options,
    const InputOption *input_options[static n_sub_options]);

// Getting and Processing Input
void input_get(const char *message, char input_buff[static INPUT_BUFF_LEN]);

// Handling Input
void input_switch_option(InputOption *current_opt, const InputOption *new_opt);
void input_process(InputOption *current_option,
                   char input_buff[static INPUT_BUFF_LEN], AppEnv env);

#ifdef _cplusplus
}
#endif
#endif
