
#ifndef __GLUETHREAD__
#define __GLUETHREAD__
#include<stdio.h>

typedef struct _glthread{
    struct _glthread *left;
    struct _glthread *right;
} glthread_t;

void glthread_add_next(glthread_t *current_glthread, glthread_t *new_glthread);

void glthread_add_before(glthread_t *current_glthread, glthread_t *new_glthread);

void remove_glthread(glthread_t *glthread);

void init_glthread(glthread_t *glthread);

void glthread_add_last(glthread_t *base_glthread, glthread_t *new_glthread);

#define IS_GLTHREAD_LIST_EMPTY(glthreadptr)                                                       \
    (glthreadptr->left == NULL && glthreadptr->right == NULL)

#define offsetof(struct_name,field_name)                                                          \
    (char*)&(((struct_name*)0)->field_name)
    
#define GLTHREAD_TO_STRUCT(fn_name, structure_name, field_name, glthreadptr)           \
    static inline structure_name * fn_name(glthread_t *glthreadptr){                   \
        return (structure_name *)((char *)(glthreadptr) - (char *)&(((structure_name *)0)->field_name)); \
    }
 
/* delete safe loop*/
/*Normal continue and break can be used with this loop macro*/

#define BASE(glthreadptr)   ((glthreadptr)->right)

#define ITERATE_GLTHREAD_BEGIN(glthreadptrstart, glthreadptr)                                      \
{                                                                                                  \
    glthread_t* next = NULL;                                                                       \
    for(glthreadptr = BASE(glthreadptrstart);glthreadptr != NULL;glthreadptr = next){              \
        next = glthreadptr->right;                                                                 \



#define ITERATE_GLTHREAD_END                                                                       \
        }}

#define GLTHREAD_GET_USER_DATA_FROM_OFFSET(glthreadptr, offset)                                    \
    (void*)((char*)glthreadptr - offset)
    

void delete_glthread_list(glthread_t *base_glthread);

unsigned int get_glthread_list_count(glthread_t *base_glthread);

void glthread_priority_insert(glthread_t *base_glthread,     
                         glthread_t *glthread,
                         int (*comp_fn)(void *, void *),
                         int offset);

#endif /* __GLUETHREAD__ */
