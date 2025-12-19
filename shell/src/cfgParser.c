// ############## LLM Generated Code Begins ##############
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "../include/cfgParser.h"

//HELPER FUNCTIONS
char peek (Parser* p) {
    if (p->position >= p->length) return '\0';
    return p->input[p->position];
}

char next (Parser* p) {
    if (p->position >= p->length) return '\0';
    return p->input[p->position++];
}

void skip_whitespace (Parser* p) {
    while (peek(p) == ' ' || peek(p) == '\t' || peek(p) == '\n' || peek(p) == '\r') p->position++;
}

bool expect (Parser* p, char c) {
    skip_whitespace(p);
    if (peek(p) == c) {
        p->position++;
        return true;
    }
    return false;
}
// ############## LLM Generated Code Ends ################




//RULES

//shell_cmd  ->  cmd_group ((& | ;) cmd_group)* &?
bool parse_shell_cmd (Parser* p) {
    if (!parse_cmd_group(p)) return false;

    while (true) {
        int saved_pos = p->position;
        skip_whitespace(p);

        if (peek(p) == '&' || peek(p) == ';') {
            p->position++;
            skip_whitespace(p);
            if (peek(p) == '\0') {
                p->position = saved_pos;
                break;
            }
            p->position = saved_pos;
        }
        else {
            p->position = saved_pos;
            break;
        }

        if (!(expect(p, '&') || expect(p, ';'))) break;
        if (!parse_cmd_group(p)) return false;
    }

    skip_whitespace(p);
    expect(p, '&');

    return true;
}

//cmd_group ->  atomic (\| atomic)*
bool parse_cmd_group (Parser* p) {
    if (!parse_atomic(p)) return false;

    while(true) {
        skip_whitespace(p);

        if (!expect(p, '|')) break;
        if (!parse_atomic(p)) return false;
    }

    skip_whitespace(p);

    return true;
}

//atomic -> name (name | input | output)*
bool parse_atomic (Parser* p) {
    if (!parse_name(p)) return false;

    while(true) {
        skip_whitespace(p);

        if (!(parse_name(p) || parse_input(p) || parse_output(p))) break;
    }

    return true;
}

//input -> < name | <name
bool parse_input (Parser* p) {
    int start_position = p->position;

    if (expect(p, '<')) {
        skip_whitespace(p);
        if (parse_name(p)) return true;
    }

    p->position = start_position;

    return false;
}

//output -> > name | >name | >> name | >>name
bool parse_output (Parser* p) {
    int start_position = p->position;

    if (expect(p, '>')) {
        expect(p, '>');
        skip_whitespace(p);
        if (parse_name(p)) return true;
    }

    p->position = start_position;
    return false;
}

//name -> r"[^|&><;]+"
bool parse_name (Parser* p) {
    // int start_position = p->position;
    skip_whitespace(p);

    int current_pos = p->position;
    while(true) {
        char c = peek(p);
        if (c == '\0' || strchr("|&><;", c) != NULL || isspace(c)) break;
        p->position++;
    }

    return p->position > current_pos;
}



bool check_input_validity (const char* s) {
    Parser p = {
        .input = s,
        .position = 0,
        .length = strlen(s)
    };

    bool result = parse_shell_cmd(&p);
    skip_whitespace(&p);
    return result && (peek(&p) == '\0');
}