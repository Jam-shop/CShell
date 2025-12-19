#include <stdbool.h>

#ifndef CFGPARSER_H
#define CFGPARSER_H

typedef struct {
    const char* input;
    int position;
    int length;
} Parser;

//Helper functions
char peek (Parser* p);
char next (Parser* p);
void skip_whitespace (Parser* p);
bool expect (Parser* p, char c);

//CFG rules
bool parse_shell_cmd (Parser* p);
bool parse_cmd_group (Parser* p);
bool parse_atomic (Parser* p);
bool parse_input (Parser* p);
bool parse_output (Parser* p);
bool parse_name (Parser* p);


bool check_input_validity (const char* s);

#endif