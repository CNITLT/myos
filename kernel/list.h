#ifndef __LIST_H
#define __LIST_H
#include "stdint.h"

#define offset(type, member) (int)(&(((type*)0) ->member))
#define elem2entry(type, member, ptr) (type*)((size_t)ptr - offset(type, member))

struct list_node{
   struct list_node* prev;
   struct list_node* next;
};

struct list{
    struct list_node head;
    struct list_node tail;
};

/* 自定义函数类型,用于在list_traversal中做回调函数 */
typedef bool list_traversal_callback(struct list_node*, int);

/*
@brief 初始化链表
@param p_list: struct list* :链表地址
*/
void list_init (struct list* p_list);

/*
@brief 在某节点前插入节点
@param before:struct list_node* : 被插入的节点
@param p_node: struct list_node* : 插入节点
*/
void list_insert_before(struct list_node* before, struct list_node* p_node);

/*
@brief 在列表头插入元素
@param p_list:struct list* : 链表地址
@param p_node: struct list_node* : 插入节点
*/
void list_push_front(struct list* p_list, struct list_node* p_node);
void list_iterate(struct list* p_list);

/*
@brief 在列表尾插入元素
@param p_list:struct list* : 链表地址
@param p_node: struct list_node* : 插入节点
*/
void list_push_back(struct list* p_list, struct list_node* p_node);  

/*
@brief 删除某个节点, 空间不会释放,仅调整链表指针
@param p_node: struct list_node* : 待删除的节点
*/
void list_remove(struct list_node* p_node);

/*
@brief 删除第一个节点
@param p_list:struct list* : 链表地址
@return list_node*:若列表为空返回NULL, 否则返回删除的节点地址
*/
struct list_node* list_pop_front(struct list* p_list);

/*
@brief 判断链表是否为空
@param p_list:struct list* : 链表地址
@return bool: 空则为true，否则false
*/
bool list_empty(struct list* p_list);

/*
@brief 获取链表长度
@param p_list:struct list* : 链表地址
@return size_t: 链表长度
*/
size_t list_len(struct list* p_list);
struct list_node* list_traversal(struct list* p_list, list_traversal_callback func, int arg);


/*
@brief 判断链表里是否存在节点
@param p_list:struct list* : 链表地址
@param P_node: struct list_node* :需要查找的node
@return bool: 若存在返回true,否则false
*/
bool find_node(struct list* p_list, struct list_node* p_node);


#endif