#ifndef STR_CMD_PARSE_H
#define STR_CMD_PARSE_H 

typedef struct sgetopt_t {
    /* NULL terminated string representing opt, canonically starting with - */
    char *opt;
    /* Function to be called on string coming after opt, using auxiliary data in
       aux. Function should be prepared to accept NULL arg */
    void (*fun) (char *arg, void *aux);
    /* Auxiliary data */
    void *aux;
} sgetopt_t;

void str_cmd_parse (char *str, sgetopt_t **cmds);

#endif /* STR_CMD_PARSE_H */
