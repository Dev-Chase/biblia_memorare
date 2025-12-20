#include "input.h"
#include "constants.h"
#include "global.h"
#include "passage.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const InputOption GLOBAL_INPUT_OPTION;
static const InputOption GET_PASSAGE_OPTION;
static const InputOption SAVE_PASSAGE_OPTION;
static const InputOption GET_SAVED_PASSAGE_OPTION;
static const InputOption SEARCH_SAVED_PASSAGES_OPTION;
static const InputOption SELECT_PASSAGE_FROM_SAVED_LIST_OPTION;
static const InputOption RANDOM_SAVED_PASSAGE_OPTION;
static const InputOption SAVED_PASSAGE_INFO_OPTION;
static const InputOption EDIT_SAVED_PASSAGE_OPTION;
static const InputOption DELETE_SAVED_PASSAGE_OPTION;
static const InputOption QUIZ_OPTION;

// Directions
void input_print_options_list(
    size_t n_sub_options,
    const InputOption *input_options[static n_sub_options]) {
  puts("---------------------");
  puts("Available Options:");
  printf("info/help/list - List Available Options\n");
  for (size_t i = 0; i < n_sub_options; i++) {
    input_options[i]->print_desc();
  }
  printf("current - See Current Option\n");
  printf("clear - Clear console\n");
  printf("exit - Exit Program\n");
  puts("---------------------");
}

// Getting and Processing Input
// NOTE: does not append ": " to message
void input_get(const char *message, size_t buff_len, char *input_buff) {
  printf("%s", message);
  fflush(stdout);
  fgets(input_buff, buff_len, stdin);

  // Remove trailing '\n'
  size_t input_buff_strlen = strnlen(input_buff, buff_len);
  if (input_buff[input_buff_strlen - 1] == '\n') {
    input_buff[input_buff_strlen - 1] = '\0';
  }

  fflush(stdout);
}

// Handling Input
// NOTE: new_data.input_buff is unused
void input_opt_set_data(InputOption *current_option, InputOptionData new_data,
                        bool clean_old_mem) {
  // Free old data if dynamically allocated
  if (clean_old_mem) {
    if (current_option->data.type == SavedPassageList) {
      free(current_option->data.value.saved_passage_list);
      current_option->data.value.saved_passage_list = NULL;
    }
  }

  // TODO: handle books_list case and maybe consider simply copying all the data
  // over after managing dynamically allocated stuff instead of handling each
  // case individually
  current_option->data.type = new_data.type;
  switch (current_option->data.type) {
  case NoData:
    break;
  case RetrievedPassageId:
    memmove(current_option->data.value.passage_id, new_data.value.passage_id,
            sizeof(PassageId));
    break;
  case SavedPassage:
    memmove(current_option->data.value.passage_id, new_data.value.passage_id,
            sizeof(PassageId));
    current_option->data.value.saved_passage_obj =
        new_data.value.saved_passage_obj;
    break;
  case SavedPassageList:
    current_option->data.value.saved_passage_list =
        new_data.value.saved_passage_list;
    current_option->data.value.saved_passage_list_len =
        new_data.value.saved_passage_list_len;
    current_option->data.value.saved_passage_list_size =
        new_data.value.saved_passage_list_size;
    break;
  default:
    error_if(true, "Invalid data type given to replace current_option");
    break;
  }
}

// NOTE: only run after new_opt has been executed
void input_switch_option(InputOption *current_opt, const InputOption *new_opt) {
  InputOptionData copied_data = current_opt->data;
  *current_opt = *new_opt;
  current_opt->data = copied_data;
}

bool input_info_req_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "info") == 0) ||
         (strcmp(input_buff, "help") == 0) || (strcmp(input_buff, "list") == 0);
}

bool current_opt_req_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "current") == 0);
}

bool clear_req_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "clear") == 0);
}

bool exit_req_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "exit") == 0);
}

void input_process(InputOption *current_option, AppEnv env) {
  // Print Available Options if Requested
  if (input_info_req_check(current_option->data.input_buff)) {
    input_print_options_list(current_option->n_sub_options,
                             current_option->sub_options);
    return;
  }

  if (current_opt_req_check(current_option->data.input_buff)) {
    printf("Your Current Option is:\n");
    current_option->print_desc();
    return;
  }

  if (clear_req_check(current_option->data.input_buff)) {
    // Clear the Console
    printf("\033[2J\033[H");
    return;
  }

  if (exit_req_check(current_option->data.input_buff)) {
    puts("Exiting program");
    exit(EXIT_SUCCESS);
    return;
  }

  size_t i = 0;
  for (; i < current_option->n_sub_options; i++) {
    if (current_option->sub_options[i]->input_check(
            current_option->data.input_buff)) {
      break;
    }
  }

  if (i == current_option->n_sub_options) {
    printf("%s is not a valid option, enter 'info' or 'help' or 'list' to see "
           "available "
           "options\n",
           current_option->data.input_buff);
    return;
  }

  const InputOption *selected_option = current_option->sub_options[i];

  // Switch Sub Options if Selected Option Requests it
  if (selected_option->exec(current_option, env)) {
    input_switch_option(current_option, selected_option);
  }
}

// Input Option Specifics
// Getting a Passage from the Bible
void get_passage_option_print_desc(void) {
  puts("get/passage get - Get a Passage from the Bible");
}

bool get_passage_option_fn(InputOption *current_opt, AppEnv env) {
  PassageInfo passage = {0};
  cJSON *passage_data = NULL;
  bool replace_current_opt = true;

  if (current_opt->data.type == SavedPassage) {
    passage_get_info_from_id(current_opt->data.value.passage_id, &passage);
    replace_current_opt = false;
  } else {
    if (!passage_info_get_from_input(
            "What passage are you searching for?: ", &passage, env.curl,
            env.curl_code, env.bible_version, env.bibles_arr, env.books_arr)) {
      return false;
    }
  }

  passage_data =
      passage_get_data(passage, env.curl, env.curl_code, *env.bible_version);
  if (passage_data != NULL) {
    // Printing Passage Text
    passage_print_text(passage_data, env.bible_version->abbr, true);

    // Saving PassageID to current_opt if going to Switch Option
    if (replace_current_opt) {
      InputOptionData new_data = {
          .type = RetrievedPassageId,
          .value = {0},
          .input_buff = {0},
      };
      passage_get_id(passage, new_data.value.passage_id);
      input_opt_set_data(current_opt, new_data, true);
    }

    cJSON_Delete(passage_data);
    return replace_current_opt;
  }

  input_opt_set_data(current_opt, (InputOptionData){.type = NoData}, true);
  return false;
}

bool get_passage_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "get") == 0) ||
         (strcmp(input_buff, "passage get") == 0);
}

static const InputOption GET_PASSAGE_OPTION = {
    .exec = get_passage_option_fn,
    .print_desc = get_passage_option_print_desc,
    .input_check = get_passage_option_input_check,
    .n_sub_options = 5,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
                                &SAVE_PASSAGE_OPTION, &GET_SAVED_PASSAGE_OPTION,
                                &SEARCH_SAVED_PASSAGES_OPTION},
    .data = {.type = NoData}};

// Saving a Passage ID
void save_passage_option_print_desc(void) {
  puts("save/passage save - Save a Passage");
}

bool save_passage_option_fn(InputOption *current_opt, AppEnv env) {
  InputOptionData new_data = {.type = SavedPassage};

  // Set current_opt->data.value.passage_id if not just gotten through
  // retrieving a passage
  if (current_opt->data.type != RetrievedPassageId) {
    puts("Getting a Passage Id to save since none was provided.");
    PassageInfo passage;
    if (!passage_info_get_from_input(
            "Which passage do you want to save?: ", &passage, env.curl,
            env.curl_code, env.bible_version, env.bibles_arr, env.books_arr)) {
      return false;
    }

    passage_get_id(passage, new_data.value.passage_id);

    get_passage_option_fn(current_opt, env);
  }

  passage_save_input(new_data.value.passage_id, env.saved_passages_json);
  // NOTE: no error handling is done because the only error that should be
  // present here is a passage already being saved, in which case a
  // SavedPassage can still be set (might have to change if passage_save_input
  // can error in any other meaningful way)

  new_data.value.saved_passage_obj = passages_get_by_id(
      env.saved_passages_json, current_opt->data.value.passage_id);
  // NOTE: pointer lasts for lifetime of env.saved_passages_json
  input_opt_set_data(current_opt, new_data, true);

  // Only NULL if it was not saved or is not already saved, which should never
  // happen
  error_if(current_opt->data.value.saved_passage_obj == NULL,
           "error saving passage: could not be retrieved after having "
           "been saved");

  return true;
}

bool save_passage_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "save") == 0) ||
         (strcmp(input_buff, "passage save") == 0);
}

static const InputOption SAVE_PASSAGE_OPTION = {
    .exec = save_passage_option_fn,
    .print_desc = save_passage_option_print_desc,
    .input_check = save_passage_option_input_check,
    .n_sub_options = 5,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
                                &SAVED_PASSAGE_INFO_OPTION,
                                &EDIT_SAVED_PASSAGE_OPTION,
                                &DELETE_SAVED_PASSAGE_OPTION},
    .data = {.type = NoData}};

// Getting a Saved Passage
void get_saved_passage_option_print_desc(void) {
  puts("saved/get saved - Get a saved passage");
}

bool get_saved_passage_option_fn(InputOption *current_opt, AppEnv env) {
  PassageInfo passage;
  PassageId passage_id;
  if (current_opt->data.type == RetrievedPassageId) {
    // NOTE: no need for bounds checking since both are of type PassageId
    // (char[] of the same length) and should both be null-terminated
    strcpy(passage_id, current_opt->data.value.passage_id);
    passage_get_info_from_id(passage_id, &passage);
  } else {
    if (!passage_info_get_from_input(
            "What saved passage are you looking for?: ", &passage, env.curl,
            env.curl_code, env.bible_version, env.bibles_arr, env.books_arr)) {
      return false;
    }

    passage_get_id(passage, passage_id);
  }

  cJSON *passage_obj = passages_get_by_id(env.saved_passages_json, passage_id);
  if (passage_obj == NULL) {
    passage_print_reference(passage, *env.books_arr, false);
    printf(" is not saved in " PASSAGES_FILE "\n");
    return false;
  }

  printf("Found ");
  passage_print_reference(passage, *env.books_arr, false);
  printf(" in " PASSAGES_FILE "!\n");

  InputOptionData new_data = {.type = SavedPassage,
                              .value = {.saved_passage_obj = passage_obj}};
  strcpy(new_data.value.passage_id, passage_id);
  input_opt_set_data(current_opt, new_data, true);

  return true;
}

bool get_saved_passage_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "saved") == 0) ||
         (strcmp(input_buff, "get saved") == 0);
}

static const InputOption GET_SAVED_PASSAGE_OPTION = {
    .exec = get_saved_passage_option_fn,
    .print_desc = get_saved_passage_option_print_desc,
    .input_check = get_saved_passage_option_input_check,
    .n_sub_options = 8,
    .sub_options =
        (const InputOption *[]){
            &GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
            &GET_SAVED_PASSAGE_OPTION, &RANDOM_SAVED_PASSAGE_OPTION,
            &SEARCH_SAVED_PASSAGES_OPTION, &SAVED_PASSAGE_INFO_OPTION,
            &EDIT_SAVED_PASSAGE_OPTION, &DELETE_SAVED_PASSAGE_OPTION},
    .data = {.type = NoData}};

// Searching Through Saved Passages
void search_saved_passages_option_print_desc(void) {
  puts("search/find/search passages - Search through saved passages");
}

void saved_passages_list_print(InputOptionData input_opt_data) {
  for (size_t i = 0; i < input_opt_data.value.saved_passage_list_len; i++) {
    cJSON *passage_id = passage_obj_get_field(
        input_opt_data.value.saved_passage_list[i], PassageObjId);
    printf("%zu - %s \n", i, passage_id->valuestring);
  }
}

bool search_saved_passages_option_fn(InputOption *current_opt, AppEnv env) {
  PassageInfo search_key_info;
  passage_info_get_from_input("What is your search key?: ", &search_key_info,
                              env.curl, env.curl_code, env.bible_version,
                              env.bibles_arr, env.books_arr);

  InputOptionData new_data = {.type = SavedPassageList};
  new_data.value.saved_passage_list_len = 0;
  new_data.value.saved_passage_list_size = 10;
  new_data.value.saved_passage_list =
      (cJSON **)malloc(new_data.value.saved_passage_list_size *
                       sizeof(new_data.value.saved_passage_list[0]));
  if (new_data.value.saved_passage_list == NULL) {
    printf("Failed to allocate memory needed for saved_passage_list");
    return false;
  }

  cJSON *passages_arr = passages_array_get(env.saved_passages_json);
  cJSON *passage_obj = NULL;
  cJSON_ArrayForEach(passage_obj, passages_arr) {
    if (passages_passage_matches_key(passage_obj, search_key_info)) {
      // Add Object to the List
      new_data.value.saved_passage_list[new_data.value.saved_passage_list_len] =
          passage_obj;
      new_data.value.saved_passage_list_len++;

      // Make list bigger if need be
      if (new_data.value.saved_passage_list_len >=
          new_data.value.saved_passage_list_size) {
        new_data.value.saved_passage_list_size *= 2;
        new_data.value.saved_passage_list =
            realloc(new_data.value.saved_passage_list,
                    new_data.value.saved_passage_list_size *
                        sizeof(new_data.value.saved_passage_list[0]));
        error_if(new_data.value.saved_passage_list == NULL,
                 "Failed to reallocate memory for saved_passage_list");
      }
    }
  }

  printf("Passages:\n");
  saved_passages_list_print(new_data);

  input_opt_set_data(current_opt, new_data, true);
  return true;
}

bool search_saved_passages_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "search") == 0) ||
         (strcmp(input_buff, "find") == 0) ||
         (strcmp(input_buff, "search passages") == 0);
}

// TODO: add as sub-option to more options
static const InputOption SEARCH_SAVED_PASSAGES_OPTION = {
    .exec = search_saved_passages_option_fn,
    .print_desc = search_saved_passages_option_print_desc,
    .input_check = search_saved_passages_option_input_check,
    .n_sub_options = 7,
    .sub_options =
        (const InputOption *[]){&GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
                                &GET_SAVED_PASSAGE_OPTION,
                                &SEARCH_SAVED_PASSAGES_OPTION,
                                &RANDOM_SAVED_PASSAGE_OPTION, &QUIZ_OPTION,
                                &SELECT_PASSAGE_FROM_SAVED_LIST_OPTION},
    .data = {.type = NoData}};

// Selecting a Passage from a Saved Passage List
void select_passage_from_saved_list_option_print_desc(void) {
  puts("select/pick - Select a passage from the list");
}

bool select_passage_from_saved_list_option_fn(InputOption *current_opt,
                                              AppEnv _) {
  error_if(current_opt->data.type != SavedPassageList,
           "Attempted to Get a Saved Passage's information when there is no "
           "Saved Passage given");
  error_if(current_opt->data.value.saved_passage_list == NULL,
           "Attempted to Get Information from a NULL Saved Passage");

  printf("Pick a passage:\n");
  saved_passages_list_print(current_opt->data);
  char input_buff[INPUT_BUFF_LEN] = "\0";
  input_get("Passage Index: ", INPUT_BUFF_LEN, input_buff);
  long inputted_num = strtol(input_buff, (char **)NULL, 10);

  // Validating Input
  if (inputted_num < 0) {
    printf("%s is not a valid index\n", input_buff);
    return false;
  }

  if ((size_t)inputted_num >= current_opt->data.value.saved_passage_list_len) {
    printf("%s (converted %ld) is larger than the list's length\n", input_buff,
           inputted_num);
    return false;
  }

  // Selecting Passage
  InputOptionData new_data = {.type = SavedPassage};
  cJSON *selected_passage =
      current_opt->data.value.saved_passage_list[inputted_num];
  cJSON *selected_passage_id =
      passage_obj_get_field(selected_passage, PassageObjId);
  new_data.value.saved_passage_obj = selected_passage;
  strncpy(new_data.value.passage_id, selected_passage_id->valuestring,
          MAX_PASSAGE_ID_LEN - 1);
  // NOTE: bounds checking is done to prevent overflows, but it does not
  // guarantee that the saved passage id is valid

  input_opt_set_data(current_opt, new_data, true);
  printf("Successfully selected %s\n", current_opt->data.value.passage_id);

  return true;
}

bool select_passage_from_saved_list_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "select") == 0) ||
         (strcmp(input_buff, "pick") == 0);
}

static const InputOption SELECT_PASSAGE_FROM_SAVED_LIST_OPTION = {
    .exec = select_passage_from_saved_list_option_fn,
    .print_desc = select_passage_from_saved_list_option_print_desc,
    .input_check = select_passage_from_saved_list_option_input_check,
    .n_sub_options = 9,
    .sub_options =
        (const InputOption *[]){
            &GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
            &GET_SAVED_PASSAGE_OPTION, &RANDOM_SAVED_PASSAGE_OPTION,
            &SEARCH_SAVED_PASSAGES_OPTION, &SAVED_PASSAGE_INFO_OPTION,
            &EDIT_SAVED_PASSAGE_OPTION, &DELETE_SAVED_PASSAGE_OPTION,
            &QUIZ_OPTION},
    .data = {.type = NoData}};

// Getting a Random Saved Passage
void random_saved_passage_option_print_desc(void) {
  puts("random/get random - Get a random saved passage");
}

bool random_saved_passage_option_fn(InputOption *current_opt, AppEnv env) {
  cJSON *passage_obj = passages_get_random_entry(env.saved_passages_json);
  // Error Handling already done

  printf("Successfully Retrieved a Random Passage\n");
  InputOptionData new_data = {.type = SavedPassage,
                              .value = {.saved_passage_obj = passage_obj}};

  // Getting Passage ID
  cJSON *passage_obj_id = passage_obj_get_field(passage_obj, PassageObjId);

  strncpy(new_data.value.passage_id, passage_obj_id->valuestring,
          MAX_PASSAGE_ID_LEN - 1);
  // NOTE: bounds checking is done to prevent overflows, but it does not
  // guarantee that the saved passage id is valid

  input_opt_set_data(current_opt, new_data, true);

  return true;
}

bool random_saved_passage_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "random") == 0) ||
         (strcmp(input_buff, "get random") == 0);
}

static const InputOption RANDOM_SAVED_PASSAGE_OPTION = {
    .exec = random_saved_passage_option_fn,
    .print_desc = random_saved_passage_option_print_desc,
    .input_check = random_saved_passage_option_input_check,
    .n_sub_options = 7,
    .sub_options =
        (const InputOption *[]){
            &GLOBAL_INPUT_OPTION, &GET_PASSAGE_OPTION,
            &GET_SAVED_PASSAGE_OPTION, &RANDOM_SAVED_PASSAGE_OPTION,
            &SAVED_PASSAGE_INFO_OPTION, &EDIT_SAVED_PASSAGE_OPTION,
            &DELETE_SAVED_PASSAGE_OPTION},
    .data = {.type = NoData}};

// Getting a Saved Passage's Information
void saved_passage_info_option_print_desc(void) {
  puts("show/get info/get field - Get the passage's saved information");
}

bool saved_passage_info_option_fn(InputOption *current_opt, AppEnv env) {
  error_if(current_opt->data.type != SavedPassage,
           "Attempted to Get a Saved Passage's information when there is no "
           "Saved Passage given");
  error_if(current_opt->data.value.saved_passage_obj == NULL,
           "Attempted to Get Information from a NULL Saved Passage");

  char input_buff[INPUT_BUFF_LEN] = "\0";
  const char *input_start = "show";
  size_t input_start_len = strlen(input_start);
  if (strncmp(current_opt->data.input_buff, input_start, input_start_len) ==
          0 &&
      current_opt->data.input_buff[input_start_len] == ' ') {
    strncpy(input_buff, &current_opt->data.input_buff[input_start_len + 1],
            INPUT_BUFF_LEN - 1);
  } else {
    input_get("What field would you like to see (id/name, content, message, or "
              "context)?: ",
              INPUT_BUFF_LEN, input_buff);
  }

  PassageObjField req_field;
  if (strcmp(input_buff, "id") == 0 || strcmp(input_buff, "name") == 0 ||
      strcmp(input_buff, "reference") == 0) {
    req_field = PassageObjId;
  } else if (strcmp(input_buff, "message") == 0) {
    req_field = PassageObjMessage;
  } else if (strcmp(input_buff, "context") == 0) {
    req_field = PassageObjContext;
  } else if (strcmp(input_buff, "content") == 0 ||
             strcmp(input_buff, "text") == 0) {
    get_passage_option_fn(current_opt, env);
    return false;
  } else {
    fprintf(stderr, "%s is not a valid field for a saved passage\n",
            input_buff);
    return false;
  }

  cJSON *field = passage_obj_get_field(
      current_opt->data.value.saved_passage_obj, req_field);
  printf("Here is the passage's saved %s:\n", input_buff);
  printf("%s\n", field->valuestring);
  if (req_field == PassageObjId) {
    if (strcmp(field->valuestring, current_opt->data.value.passage_id) != 0) {
      printf("This is what the ID should be: %s\n",
             current_opt->data.value.passage_id);
      error_if(true, "That's weird! The Saved Passage's ID doesn't match the "
                     "one stored in code!\n");
    }

    printf("It's reference is: ");
    PassageInfo passage_info;
    passage_get_info_from_id(field->valuestring, &passage_info);
    passage_print_reference(passage_info, *env.books_arr, false);
    printf("\n");
  }

  return false;
}

bool saved_passage_info_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strncmp(input_buff, "show", strlen("show")) == 0) ||
         (strcmp(input_buff, "get info") == 0) ||
         (strcmp(input_buff, "get field") == 0);
}

static const InputOption SAVED_PASSAGE_INFO_OPTION = {
    .exec = saved_passage_info_option_fn,
    .print_desc = saved_passage_info_option_print_desc,
    .input_check = saved_passage_info_option_input_check,
    // NOTE: sub_options should not be accessible here anyway
    .n_sub_options = 1,
    .sub_options = (const InputOption *[]){&GLOBAL_INPUT_OPTION},
    .data = {.type = NoData}};

// Editing a Saved Passage
void edit_saved_passage_option_print_desc(void) {
  puts("edit/edit info/edit field - Edit the passage's saved information");
}

// NOTE: edited fields must be present in the PASSAGES_FILE file
bool edit_saved_passage_option_fn(InputOption *current_opt, AppEnv env) {
  error_if(current_opt->data.type != SavedPassage,
           "Attempted to Edit a Saved Passage's information when there is no "
           "Saved Passage given");
  error_if(current_opt->data.value.saved_passage_obj == NULL,
           "Attempted to Edit a NULL Saved Passage");

  char input_buff[INPUT_BUFF_LEN] = "\0";
  const char *input_start = "edit";
  size_t input_start_len = strlen(input_start);
  if (strncmp(current_opt->data.input_buff, input_start, input_start_len) ==
          0 &&
      current_opt->data.input_buff[input_start_len] == ' ') {
    strncpy(input_buff, &current_opt->data.input_buff[input_start_len + 1],
            INPUT_BUFF_LEN - 1);
  } else {
    input_get("What field would you like to edit (id, message, or context): ",
              INPUT_BUFF_LEN, input_buff);
  }

  PassageObjField req_field;
  size_t field_input_buff_size = 0;
  if (strcmp(input_buff, "id") == 0) {
    req_field = PassageObjId;
    field_input_buff_size = PASSAGE_INPUT_BUFF_SIZE;
  } else if (strcmp(input_buff, "message") == 0) {
    req_field = PassageObjMessage;
    field_input_buff_size = PASSAGE_MESSAGE_BUFF_SIZE;
  } else if (strcmp(input_buff, "context") == 0) {
    req_field = PassageObjContext;
    field_input_buff_size = PASSAGE_CONTEXT_BUFF_SIZE;
  } else {
    fprintf(stderr, "%s is not a valid field for a saved passage\n",
            input_buff);
    return false;
  }

  char *field_input_buff = (char *)malloc(field_input_buff_size * sizeof(char));
  error_if(field_input_buff == NULL, "failed to malloc field editing buffer");

  // Getting Input
  printf("What would you like to change the passage's %s to%s", input_buff,
         (req_field == PassageObjId) ? " (enter the passage in regularly)"
                                     : "");
  input_get("?: ", field_input_buff_size, field_input_buff);

  // Changing Field in Object
  if (req_field == PassageObjId) {
    PassageInfo passage = {0};
    PassageId passage_id;
    if (!passage_info_get_from_string(field_input_buff, &passage, env.curl,
                                      env.curl_code, env.bible_version,
                                      env.bibles_arr, env.books_arr)) {
      printf("Failed to parse given passage\n");
      free(field_input_buff);
      return false;
    }

    passage_get_id(passage, passage_id);
    cJSON_ReplaceItemInObject(current_opt->data.value.saved_passage_obj,
                              input_buff, cJSON_CreateString(passage_id));

    // Changing current_opt passage_id to the new one
    InputOptionData new_data = current_opt->data;
    strncpy(new_data.value.passage_id, passage_id, MAX_PASSAGE_ID_LEN - 1);
    input_opt_set_data(current_opt, new_data, true);
  } else {
    cJSON_ReplaceItemInObject(current_opt->data.value.saved_passage_obj,
                              input_buff, cJSON_CreateString(field_input_buff));
  }

  // Writing Changes to PASSAGES_FILE
  // Serialize new json As text
  char *json_txt = cJSON_Print(env.saved_passages_json);
  error_if(json_txt == NULL, "failed to parse json after adding new passage");

  // Write json Text to PASSAGES_FILE
  FILE *file = fopen(PASSAGES_FILE, "w");
  error_if(file == NULL, "failed to open " PASSAGES_FILE " up for writing");
  fprintf(file, "%s", json_txt);

  // Clean Up Memory
  free(json_txt);
  fclose(file);
  free(field_input_buff);

  return false;
}

bool edit_saved_passage_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strncmp(input_buff, "edit", strlen("edit")) == 0) ||
         (strcmp(input_buff, "edit info") == 0) ||
         (strcmp(input_buff, "edit field") == 0);
}

static const InputOption EDIT_SAVED_PASSAGE_OPTION = {
    .exec = edit_saved_passage_option_fn,
    .print_desc = edit_saved_passage_option_print_desc,
    .input_check = edit_saved_passage_option_input_check,
    // NOTE: sub_options should not be accessible here anyway
    .n_sub_options = 1,
    .sub_options = (const InputOption *[]){&GLOBAL_INPUT_OPTION},
    .data = {.type = NoData}};

// Deleting a Saved Passage
void delete_saved_passage_option_print_desc(void) {
  puts("del/delete/delete passage - Delete the passage from " PASSAGES_FILE);
}

bool delete_saved_passage_option_fn(InputOption *current_opt, AppEnv env) {
  error_if(
      current_opt->data.type != SavedPassage,
      "Attempted to Delete a Saved Passage when no Saved Passage was given");
  error_if(current_opt->data.value.saved_passage_obj == NULL,
           "Attempted to Delete a NULL Saved Passage");

  // Finding the Saved Passage's Index in the JSON
  int passage_obj_i = passages_get_passage_ind(
      env.saved_passages_json, current_opt->data.value.saved_passage_obj);
  if (passage_obj_i < 0) {
    printf(
        "Failed to delete passage: passage was not found in passages_json\n");
    return false;
  }

  // Deleting the Saved Passage Object from the JSON
  cJSON_DeleteItemFromArray(passages_array_get(env.saved_passages_json),
                            passage_obj_i);

  // Writing Changes to PASSAGES_FILE
  // Serialize new json As text
  char *json_txt = cJSON_Print(env.saved_passages_json);
  error_if(json_txt == NULL, "failed to parse json after adding new passage");

  // Write json Text to PASSAGES_FILE
  FILE *file = fopen(PASSAGES_FILE, "w");
  error_if(file == NULL, "failed to open " PASSAGES_FILE " up for writing");
  fprintf(file, "%s", json_txt);

  // Clean Up Memory
  free(json_txt);
  fclose(file);

  printf("Succesfully Deleted the Saved Passage from " PASSAGES_FILE "\n");

  // Clearing current_opt->data
  input_opt_set_data(current_opt, (InputOptionData){.type = NoData}, true);

  // Redirecting Back to Home
  input_switch_option(current_opt, &GLOBAL_INPUT_OPTION);

  return false;
}

bool delete_saved_passage_option_input_check(
    char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "del") == 0) ||
         (strcmp(input_buff, "delete") == 0) ||
         (strcmp(input_buff, "delete passage") == 0);
}

static const InputOption DELETE_SAVED_PASSAGE_OPTION = {
    .exec = delete_saved_passage_option_fn,
    .print_desc = delete_saved_passage_option_print_desc,
    .input_check = delete_saved_passage_option_input_check,
    // NOTE: sub_options should not be accessible here anyway
    .n_sub_options = 1,
    .sub_options = (const InputOption *[]){&GLOBAL_INPUT_OPTION},
    .data = {.type = NoData}};

// Quiz Option
// TODO: add quiz option
//   - include getting a passage's content without getting its reference then
//   guessing it and vice versa (verifying if the reference is right, but
//   perhaps not the wording of the passage unless a library is found to compare
//   text
//   - include guessing the message & context
// TODO: implement several "modes" (e.g. guessing reference, guessing text,
// guessing meaning, guessing context, guessing book or translation, etc)
// or difficultly levels
// (Consider adding being quizzed on passages fitting a key [e.g. on a book or
// chapter instead of everything])
void quiz_option_print_desc(void) {
  puts("quiz - Take a quiz on random saved passages");
}

bool quiz_option_fn(InputOption *current_opt, AppEnv env) {
  bool strict_comparison = false;
  char comparison_input[4] = "\0";
  input_get("Do you want exact comparison (Y/n)?: ", sizeof(comparison_input),
            comparison_input);

  if (tolower(comparison_input[0]) == 'y') {
    strict_comparison = true;
    printf("Okay, I'll be strict.\n");
  }

  cJSON **quiz_passages = NULL;
  size_t n_passages = 0;
  size_t n_correct = 0;
  bool free_memory = false;

  // Either Using Passages from a List or Retrieving Random Ones
  if (current_opt->data.type == SavedPassageList) {
    // Copying the Passages from the List
    quiz_passages =
        (cJSON **)malloc(current_opt->data.value.saved_passage_list_len *
                         sizeof(current_opt->data.value.saved_passage_list[0]));
    if (quiz_passages == NULL) {
      fprintf(stderr, "Failed to allocate memory needed for quiz_passages\n");
      return false;
    }

    n_passages = current_opt->data.value.saved_passage_list_len;
    memcpy(quiz_passages, current_opt->data.value.saved_passage_list,
           n_passages * sizeof(current_opt->data.value.saved_passage_list[0]));

    // Shuffling the Passages
    for (size_t i = 0; i < n_passages; i++) {
      size_t j = (size_t)(rand() % n_passages);
      cJSON *tmp = quiz_passages[j];
      quiz_passages[j] = quiz_passages[i];
      quiz_passages[i] = tmp;
    }

  } else {
    // Getting a Inputted Number of Passages to be Quizzed on
    n_passages = 5; // Defaulting to 5 Passages
    char input_num_buff[INPUT_BUFF_LEN];
    input_get("How many passages do you want to be quizzed on?: ",
              INPUT_BUFF_LEN, input_num_buff);
    long inputted_num = strtol(input_num_buff, (char **)NULL, 10);
    if (inputted_num > 0) {
      n_passages = (size_t)inputted_num;
    } else {
      printf("Inputted Number was invalid, defaulting to %zu instead\n",
             n_passages);
    }

    // Allocate Memory for Random Passages
    quiz_passages = (cJSON **)malloc(n_passages * sizeof(cJSON *));
    free_memory = true;
    if (quiz_passages == NULL) {
      fprintf(stderr, "Failed to allocate memory needed for quiz_passages\n");
      return false;
    }

    // Getting Random Passages
    // TODO: implement handling case where more passages are supplied than are
    // stored in the PASSAGES_FILE (currently will cause infinite loop)
    puts("Collecting Quiz Passages");
    for (size_t i = 0; i < n_passages; i++) {
      cJSON *random_passage = NULL;
      bool duplicate_passage = false;

      do {
        random_passage = passages_get_random_entry(env.saved_passages_json);
        duplicate_passage = false;
        for (size_t j = 0; j < i; j++) {
          if (quiz_passages[j] == random_passage) {
            duplicate_passage = true;
            break;
          }
        }

      } while (duplicate_passage);

      quiz_passages[i] = random_passage;
    }
    puts("Successfully Collected Random Quiz Passages!");
  }

  // Guessing
  for (size_t i = 0; i < n_passages; i++) {
    PassageInfo passage_info;
    cJSON *passage_id = passage_obj_get_field(quiz_passages[i], PassageObjId);
    passage_get_info_from_id(passage_id->valuestring, &passage_info);

    // Retrieving the Passage and Printing it
    cJSON *passage_data = passage_get_data(passage_info, env.curl,
                                           env.curl_code, *env.bible_version);
    if (passage_data != NULL) {
      passage_print_text(passage_data, env.bible_version->abbr, false);
      cJSON_Delete(passage_data);
    }

    // Guess Reference
    PassageInfo guessed_passage;
    PassageId guessed_passage_id;
    while (!passage_info_get_from_input(
        "What passage is that?: ", &guessed_passage, env.curl, env.curl_code,
        env.bible_version, env.bibles_arr, env.books_arr)) {
      printf("Hmm, that doesn't look quite right. Try again.\n");
    }
    passage_get_id(guessed_passage, guessed_passage_id);

    // Verify Answer
    bool correct = false;
    if (strict_comparison) {
      correct = (strncmp(passage_id->valuestring, guessed_passage_id,
                        MAX_PASSAGE_ID_LEN - 1) == 0);
    } else {
      PassageInfo guessed_key;
      passage_get_info_from_id(guessed_passage_id, &guessed_key);
      correct = passage_id_matches_key(passage_id->valuestring, guessed_key);
    }

    if (correct) {
      puts("You got that one right!");
      n_correct++;
    } else {
      PassageInfo correct_passage;
      passage_get_info_from_id(passage_id->valuestring, &correct_passage);

      printf("Sorry, that was wrong. The correct answer was: ");
      passage_print_reference(correct_passage, *env.books_arr, true);
    }
  }

  printf("You got %zu/%zu questions right. That's a(n) %.2f%%\n", n_correct,
         n_passages, (((double)(n_correct) / (double)(n_passages)) * 100.0));

  if (free_memory) {
    free(quiz_passages);
  }

  return false;
}

bool quiz_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "quiz") == 0);
}

// TODO: consider adding sub option to analyze results
// TODO: make sub-option to more options
static const InputOption QUIZ_OPTION = {
    .exec = quiz_option_fn,
    .print_desc = quiz_option_print_desc,
    .input_check = quiz_option_input_check,
    // NOTE: sub_options should not be accessible here anyway
    .n_sub_options = 1,
    .sub_options = (const InputOption *[]){&GLOBAL_INPUT_OPTION},
    .data = {.type = NoData}};

// Global/Home Option
void global_option_print_desc(void) {
  puts("root/global/home - Go back to application home");
}

bool global_option_fn(InputOption *current_opt, AppEnv _) {
  input_opt_set_data(current_opt, (InputOptionData){.type = NoData}, true);
  return true;
}

bool global_option_input_check(char input_buff[static INPUT_BUFF_LEN]) {
  return (strcmp(input_buff, "root") == 0) ||
         (strcmp(input_buff, "global") == 0) ||
         (strcmp(input_buff, "home") == 0);
}

const InputOption GLOBAL_INPUT_OPTION = {
    .exec = global_option_fn,
    .print_desc = global_option_print_desc,
    .input_check = global_option_input_check,
    .n_sub_options = 6,
    .sub_options =
        (const InputOption *[]){&GET_PASSAGE_OPTION, &SAVE_PASSAGE_OPTION,
                                &SEARCH_SAVED_PASSAGES_OPTION,
                                &RANDOM_SAVED_PASSAGE_OPTION,
                                &GET_SAVED_PASSAGE_OPTION, &QUIZ_OPTION},
    .data = {.type = NoData}};
