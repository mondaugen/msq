#ifndef DEFS_H
#define DEFS_H 

#include <stdlib.h> 
#include <string.h> 

/* Defines for convenience */
#define _M(t,s) (t*)malloc(sizeof(t)*s)
#define _C(t,s) (t*)calloc(s,sizeof(t)) 
#define _MZ(x,t,s) memset(x,0,sizeof(t)*s) 
#define _F(x) free(x) 

/* forward define structure */
#define FDS(t) struct t; typedef struct t t 

/* print errors */
#define _PE(format,...) fprintf(stderr,"%s:%d " format "\n", __func__, __LINE__, __VA_ARGS__)

#endif /* DEFS_H */
