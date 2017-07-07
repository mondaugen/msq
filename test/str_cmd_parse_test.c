#include <stdio.h>
#include <string.h> 
#include "str_cmd_parse.h"

static int correct = 1;
static int cmp_str_idx = 0;
static int incorrect_idx;
static char *correct_strs[] = {
    "123",
    "789-hi bob",
    NULL,
    NULL,
    NULL,
    NULL,
    "-hoa",
    "789-hi",
    "",
    "bob",
    "bobby "
};

void fun (char *arg, void *aux)
{
    char *_aux = aux;
    printf("%s: %s|\n",_aux,arg);
    if (arg && correct_strs[cmp_str_idx]) {
        correct &= (strcmp(correct_strs[cmp_str_idx],arg) == 0);
    } else {
        correct &= (arg == correct_strs[cmp_str_idx]);
    }
    if (!correct) { incorrect_idx = cmp_str_idx; }
    cmp_str_idx++;
}

sgetopt_t cmds[] = {
    {"-word",fun,"WORD"},
    {"-hi",fun,"HI"},
    {"-hia",fun,"HIA"},
    {"-ho",fun,"HO"}
};

int main (void) {
    char str1[] = " -hi 123 -ho 789-hi bob -hi -ho";
    char str2[] = " -hi -ho -hi -hoa -ho 789-hi -hi ";
    char str3[] = "-hi bob";
    char str4[] = "-ho bobby ";
    char str5[] = "";
    sgetopt_t *pcmds[] = {
        &cmds[0],&cmds[1],&cmds[2],&cmds[3],NULL
    };
    str_cmd_parse(str1,pcmds);
    str_cmd_parse(str2,pcmds);
    str_cmd_parse(str3,pcmds);
    str_cmd_parse(str4,pcmds);
    str_cmd_parse(str5,pcmds);
    if (correct) { printf("correct\n"); }
    else { printf("incorrect starting: %d\n",incorrect_idx); }
    return correct == 1 ? 0 : -1;
}

