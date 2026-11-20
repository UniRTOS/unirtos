// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__


/*************************
** Datatype definitions **
**************************/

/* The node struct.  Has a prev pointer, next pointer, and data. */
/* NOTE PLEASE DEFINE THE FIELDS IN THE ORDER GIVEN! */
/* DO NOT DEFINE ANYTHING OTHER THAN WHAT I SAY HERE */
typedef struct lnode
{
    struct lnode *prev;
    struct lnode *next;
    void *data;
} node;

/* The linked list struct.  Has a head pointer. */
typedef struct llist
{
    node *head;
    unsigned int size;
} list;

typedef void (*list_op)(void *); // list_op function pointer
typedef int (*list_pred)(void *, void *); // list_pred function pointer
typedef int (*equal_op)(void *, void *); // returns zero if the data is not equal, nonzero if it is

/**************************************************
** Prototypes for linked list library functions. **
**                                               **
** For more details on their functionality,      **
** check list.c.                                 **
***************************************************/

/* Creating */
list *create_list(void);
void free_list(list *llist, list_op free_func);

/* Adding */
void push_front(list *llist, void *data);
void push_back(list *llist, void *data);

/* Removing */
int remove_front(list *llist, list_op free_func);
int remove_back(list *llist, list_op free_func);
int remove_index(list *llist, int index, list_op free_func);
int remove_data(list *llist, void *data, equal_op compare_func, list_op free_func);
int remove_if(list *llist, list_pred pred_func, void *pred_data, list_op free_func);

/* Querying List */
void *front(list *llist);
void *back(list *llist);
void *get_index(list *llist, int index);
int is_empty(list *llist);
int get_size(list *llist);

/* Searching */
int find_occurrence(list *llist, void *search, equal_op compare_func);

/* Freeing */
void empty_list(list *llist, list_op free_func);

/* Traversal */
void traverse(list *llist, list_op do_func);


/* Debugging Support */
#ifdef DEBUG
/*
   Does the following if compiled in debug mode
   When compiled in release mode does absolutely nothing.
*/
#define IF_DEBUG(call) (call)
/* Prints text (in red) if in debug mode */
#define DEBUG_PRINT(string) fprintf(stderr, "\033[31m%s:%d %s\n\033[0m", __FILE__, __LINE__, (string))
/* Asserts if the expression given is true (!0) */
/* If this fails it prints a message and terminates */
#define DEBUG_ASSERT(expr)   \
    do                           \
    {                            \
        if (!(expr))             \
        {                        \
            fprintf(stderr, "ASSERTION FAILED %s != TRUE (%d) IN %s ON line %d\n", #expr, (expr), __FILE__, __LINE__); \
            exit(0);             \
        }                        \
    } while(0)
#else
#define IF_DEBUG(call)
#define DEBUG_PRINT(string)
#define DEBUG_ASSERT(expr)
#endif

#endif
