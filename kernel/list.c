#include "list.h"
#include "stddef.h"
#include "debug.h"
#include "stdint.h"
#include "interrupt.h"
void list_init (struct list* p_list){
    p_list->head.prev = NULL;
    p_list->head.next = &(p_list->tail);
    p_list->tail.prev = &(p_list->head);
    p_list->tail.next = NULL;
}

void list_insert_before(struct list_node* before, struct list_node* p_node){
    interrupt_state old = close_interrupt();
    struct list_node* bprev = before->prev;
    bprev->next = p_node;
    p_node->prev = bprev;
    p_node->next = before;
    before->prev = p_node;
    set_interrupt_state(old);
}

void list_push_front(struct list* p_list, struct list_node* p_node){
    list_insert_before(p_list->head.next, p_node);
}

void list_push_back(struct list* p_list, struct list_node* p_node){
    list_insert_before(&(p_list->tail), p_node);
}


void list_remove(struct list_node* p_node){
    interrupt_state old = close_interrupt();
    p_node->prev->next = p_node->next;
    p_node->next->prev = p_node->prev;
    set_interrupt_state(old);
}


struct list_node* list_pop_front(struct list* p_list){
   if(!list_empty(p_list)) {
    struct list_node* res = p_list->head.next;
    list_remove(p_list->head.next);
    return res;
   }    
   return NULL;
}

bool list_empty(struct list* p_list){
    return p_list->head.next == &(p_list->tail);
}


size_t list_len(struct list* p_list){
    int length = 0;
    struct list_node* iter = p_list->head.next;
    while(iter != &(p_list->tail)){
        length++;
        iter = iter->next;
    }
    return length;
}


bool find_node(struct list* p_list, struct list_node* p_node){
    struct list_node* iter = p_list->head.next;
    while(iter != &(p_list->tail)){
        if(iter == p_node){
            return true;
        }
        iter = iter->next;
    }
    return false;
}


struct list_node* list_traversal(struct list* p_list, list_traversal_callback func, int arg){
    struct list_node* iter = p_list->head.next;
    struct list_node* res = NULL;
    while(iter != &(p_list->tail)){
        if(func(iter,arg)){
            res = iter;
            break;
        }
        iter = iter->next;
    }
    return res;
}