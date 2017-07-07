#include <stdio.h>
#include <string.h> 
#include "str_cmd_parse.h"

static int correct = 1;
static int cmp_str_idx = 0;
static char *correct_strs[] = {
    "123",
    "789-hi bob",
    NULL,
    NULL,
    NULL,
    NULL,
    "123",
    "789-hi",
    ""
};

void fun (char *arg, void *aux)
{
    char *_aux = aux;
    printf("%s: %s\n",_aux,arg);
    if (arg && correct_strs[cmp_str_idx]) {
        correct &= (strcmp(correct_strs[cmp_str_idx],arg) == 0);
    } else {
        correct &= (arg == correct_strs[cmp_str_idx]);
    }
}

sgetopt_t cmds[] = {
    {"-word",fun,"WORD"},
    {"-hi",fun,"HI"},
    {"-hia",fun,"HIA"},
    {"-ho",fun,"HO"}
};

int main (void) {
    char str1[] = " -hi 123 -ho 789-hi bob -hi -ho";
    char str2[] = " -hi -ho -hi 123 -ho 789-hi -hi ";
    sgetopt_t *pcmds[] = {
        &cmds[0],&cmds[1],&cmds[2],&cmds[3],NULL
    };
    str_cmd_parse(str1,pcmds);
    return correct == 1 ? 0 : -1;
}

