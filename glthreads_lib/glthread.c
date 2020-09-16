#include<stdio.h>
#include "glthread.h"


void init_glthread(glthread_t *glthread){
    glthread->left = NULL;
    glthread->right = NULL;
}
void glthread_add_next(glthread_t *current_glthread, glthread_t *new_glthread){
    new_glthread->right = current_glthread->right;
    new_glthread->left = current_glthread;
    if(current_glthread->right){
        current_glthread->right->left = new_glthread;
    }
    current_glthread->right = new_glthread;
}
void glthread_add_before(glthread_t *current_glthread, glthread_t *new_glthread){
    new_glthread->right = current_glthread;
    new_glthread->left = current_glthread->left;
    if(current_glthread->left){
        current_glthread->left->right = new_glthread;
    }
    current_glthread->left = new_glthread;
}
void glthread_priority_insert(glthread_t *base_glthread,     
                         glthread_t *glthread,
                         int (*comp_fn)(void *, void *),
                         int offset){
    // If glthread is empty
    if(IS_GLTHREAD_LIST_EMPTY(base_glthread)){
        glthread_add_next(base_glthread,glthread);
        return;
    }
    glthread_t *current = NULL,*previous = NULL;
    ITERATE_GLTHREAD_BEGIN(base_glthread,current){
        previous = current;
        if(comp_fn(GLTHREAD_GET_USER_DATA_FROM_OFFSET(glthread,offset),
            GLTHREAD_GET_USER_DATA_FROM_OFFSET(current,offset))<0){
            
            glthread_add_before(current,glthread);
            return;
        }
    }ITERATE_GLTHREAD_END;
    glthread_add_next(previous,glthread);

}
void remove_glthread(glthread_t *glthread){
    if(glthread->right){
        glthread->right->left = glthread->left;
        if(glthread->left){
            glthread->left->right = glthread->right;
        }
        return;
    }
    if(glthread->left){
        glthread->left->right = NULL;
    }
    return;
}