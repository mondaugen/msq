/* 
   str_cmd_parse.c
   parse commands given by options in a string
 */

#include "str_cmd_parse.h" 
#include <stddef.h> 
#include <string.h> 

void
str_cmd_parse (char *str, sgetopt_t **cmds)
{
    char *optstarts[strlen(str)+1];
    size_t optstart_i;
    optstarts[0] = str;
    optstart_i++;
    char *nextstart = NULL;
    while ((nextstart = strchr(optstarts[optstart_i-1],' '))) {
        optstarts[optstart_i++] = nextstart;
    }
    optstarts[optstart_i] = str + strlen(str);
    size_t i;
    sgetopt_t *lastcmd, *done = NULL;
    char *lastarg = NULL;
    for (i = 0; i < optstart_i; i++) {
        size_t slen = optstarts[i+1]-optstarts[i];
        sgetopt_t **_cmd = cmds;
        while (*_cmd) {
            if (strncmp(optstarts[i],(*_cmd)->opt,slen) == 0) {
                if (optstarts[i] != str) { *(optstarts[i] - 1) = '\0'; }
                if (lastcmd) {
                    lastcmd->fun((lastarg == optstarts[i]) ? NULL : lastarg,
                            lastcmd->aux);
                }
                lastarg = optstarts[i+1];
                lastcmd = *_cmd;
                _cmd = &done;
                continue;
            }
            _cmd++;
        }
    }
    if (lastcmd) { lastcmd->fun((lastarg == optstarts[optstart_i]) ? NULL : lastarg, lastcmd->aux); }
}
