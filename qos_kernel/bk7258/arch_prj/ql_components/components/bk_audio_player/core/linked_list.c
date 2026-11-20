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


#include <stdlib.h>
#include <stdio.h>
#include "player_osal.h"
#include "linked_list.h"

static node *create_node(void *data);

/** Creates a list by allocating memory for it on the heap.
  * Also initializes size to zero and head to NULL.
  *
  * @return an empty linked list
  */
list *create_list(void)
{
    list *newlist = NULL;
    if ((newlist = player_malloc(sizeof(list))) != NULL)
    {
        newlist->head = NULL;
        newlist->size = 0;
    }
    return newlist;
}

void free_list(list *llist, list_op free_func)
{
    if (llist)
    {
        empty_list(llist, free_func);
        player_free(llist);
    }
}

/** Helper function that creates a node by allocating memory for it on the heap.
  * Sets the new node's previous and next pointers to NULL.
  *
  * @param data a void pointer to data the user wants to store in the list
  * @return a node
  */
static node *create_node(void *data)
{
    node *newnode = NULL;
    if ((newnode = player_malloc(sizeof(node))) != NULL)
    {
        newnode->prev = NULL;
        newnode->next = NULL;
        newnode->data = data;
    }
    return newnode;
}

/** Adds the data to the front of the linked list.
  *
  * @param llist a pointer to the list
  * @param data pointer to data the user wants to store in the list
  */
void push_front(list *llist, void *data)
{
    node *front;
    if ((front = create_node(data)) == NULL)
    {
        return; // malloc failed, so we can't create a new node
    }
    if (llist->head == NULL)   // empty list
    {
        front->prev = front;
        front->next = front;
    }
    else
    {
        node *head = llist->head;
        node *tail = llist->head->prev;
        front->prev = tail;
        front->next = head;
        tail->next = front;
        head->prev = front;
    }
    llist->head = front;
    llist->size += 1;
}

/** Adds the data to the back/end of the linked list.
  *
  * @param llist a pointer to the list
  * @param data pointer to data the user wants to store in the list
  */
void push_back(list *llist, void *data)
{
    node *back;
    if ((back = create_node(data)) == NULL)
    {
        return; // malloc failed
    }
    if (llist->head == NULL)   // empty list
    {
        back->prev = back;
        back->next = back;
        llist->head = back;
    }
    else
    {
        node *prev = llist->head->prev;
        prev->next = back;
        back->prev = prev;
        back->next = llist->head;
        llist->head->prev = back;
    }
    llist->size += 1;
}

/** Removes the node at the front of the linked list.
  *
  * @warning The data the node is pointing to is also freed.
  *
  * @param llist a pointer to the list
  * @param free_func pointer to a function that is responsible for freeing the node's data
  * @return -1 if the remove failed (which is only there are no elements) 0 if the remove succeeded
  */
int remove_front(list *llist, list_op free_func)
{
    if (llist->head == NULL)
    {
        return -1;
    }
    node *head = llist->head;
    node *prev = head->prev;
    node *next = head->next;
    if (head == next)   // only one item in the list
    {
        llist->head = NULL;
    }
    else
    {
        next->prev = prev;
        prev->next = next;
        llist->head = next;
    }
    llist->size -= 1;
    free_func(head->data);
    player_free(head);
    head = NULL;
    return 0;
}

/** Removes the node at the back of the linked list.
  *
  * @warning The data the node is pointing to is also freed.
  *
  * @param llist a pointer to the list
  * @param free_func pointer to a function that is responsible for freeing the node's data
  * @return -1 if the remove failed, 0 if the remove succeeded
  */
int remove_back(list *llist, list_op free_func)
{
    if (llist->head == NULL)   // empty list
    {
        return -1;
    }
    node *next = llist->head;
    node *back = llist->head->prev;
    node *prev = back->prev;
    if (llist->size == 1)   // only one item in the list
    {
        llist->head = NULL;
    }
    else
    {
        next->prev = prev;
        prev->next = next;
    }
    llist->size -= 1;
    free_func(back->data);
    player_free(back);
    back = NULL;
    return 0;
}

/** Removes the indexth node of the linked list.
  *
  * @warning The data the node is pointing to is also freed.
  *
  * @param llist a pointer to the list
  * @param index index of the node to remove
  * @param free_func pointer to a function that is responsible for freeing the node's data
  * @return -1 if the remove failed 0 if the remove succeeded
  */
int remove_index(list *llist, int index, list_op free_func)
{
    int length = get_size(llist);
    if (llist->head == NULL || index < 0 || index >= length)
    {
        return -1; // empty list or invalid index
    }
    if (index == 0)
    {
        return remove_front(llist, free_func);
    }
    if (index == length - 1)
    {
        return remove_back(llist, free_func);
    }
    int count = 0;
    node *curr = llist->head;
    while (count < index)
    {
        curr = curr->next;
        count += 1;
    }
    node *prev = curr->prev;
    node *next = curr->next;
    next->prev = prev;
    prev->next = next;
    llist->size -= 1;
    free_func(curr->data);
    player_free(curr);
    curr = NULL;
    return 0;
}

/** Removes ALL nodes whose data is EQUAL to the data passed in, or rather, when the comparison function returns true.
  *
  * @warning The data the node is pointing to is also freed.
  *
  * @param llist a pointer to the list
  * @param data the data to compare to
  * @param compare_func a pointer to a function to compare the data
  * @param free_func a pointer to a function that is responsible for freeing the nodes
  * @return the number of nodes that were removed
  */
int remove_data(list *llist, void *data, equal_op compare_func, list_op free_func)
{
    if (llist->head == NULL)
    {
        return 0; // empty list
    }
    int total, count, result, length;
    node *curr;
    void *more_data;
    total = count = result = 0;
    length = get_size(llist);
    curr = llist->head;
    while (count < length)
    {
        more_data = curr->data;
        curr = curr->next;
        if (compare_func(data, more_data))
        {
            if (count == 0)
            {
                result = remove_front(llist, free_func);
            }
            else if (count == length - 1)
            {
                result = remove_back(llist, free_func);
            }
            else
            {
                result = remove_index(llist, count, free_func);
            }
            if (result == 0)
            {
                total += 1;
            }
        }
        count += 1;
    }
    return total;
}

/** Removes all nodes whose data, when passed into the predicate function, returns true.
  *
  * @param llist a pointer to the list
  * @param data the data to compare to
  * @param compare_func a pointer to a function that determines whether to remove the node
  * @return the number of nodes that were removed
  */
int remove_if(list *llist, list_pred pred_func, void *pred_data, list_op free_func)
{
    if (llist->head == NULL)
    {
        return 0;
    }
    int total, count, result, length;
    node *curr;
    void *data;
    total = count = result = 0;
    length = get_size(llist);
    curr = llist->head;
    while (count < length)
    {
        data = curr->data;
        curr = curr->next;
        if (pred_func(pred_data, data))
        {
            if (count == 0)
            {
                result = remove_front(llist, free_func);
            }
            else if (count == length - 1)
            {
                result = remove_back(llist, free_func);
            }
            else
            {
                result = remove_index(llist, count, free_func);
            }
            if (result == 0)
            {
                total += 1;
            }
        }
        count += 1;
    }
    return total;
}

/** Gets the data at the front of the linked list.
  * Returns NULL if the list is empty.
  *
  * @param llist a pointer to the list
  * @return The data at the first node in the linked list
  */
void *front(list *llist)
{
    return (llist->head == NULL ? NULL : llist->head->data);
}

/** Gets the data at the indexth node of the linked list.
  * If the list is empty or if the index is invalid, returns NULL.
  *
  * @param llist a pointer to the list
  * @return The data at the indexth node in the linked list
  */
void *get_index(list *llist, int index)
{
    if (llist->head == NULL || index < 0 || index >= get_size(llist))
    {
        return NULL;
    }
    int count = 0;
    node *curr = llist->head;
    while (count < index)
    {
        curr = curr->next;
        count += 1;
    }
    return curr->data;
}

/** Gets the data at the "end" of the linked list.
  * Returns NULL if the list is empty.
  *
  * @param llist a pointer to the list
  * @return The data at the last node in the linked list
  */
void *back(list *llist)
{
    return (llist->head == NULL ? NULL : llist->head->prev->data);
}

/** Checks to see if the list is empty.
  *
  * @param llist a pointer to the list
  * @return 1 if the list is indeed empty, 0 otherwise
  */
int is_empty(list *llist)
{
    return (llist->head == NULL && llist->size == 0) ? 1 : 0;
}

/** Gets the size of the linked list.
  *
  * @param llist a pointer to the list
  * @return The size of the linked list
  */
int get_size(list *llist)
{
    return llist->size;
}

/** Tests if the search data passed in is in the linked list.
  *
  * @param llist a pointer to a linked list.
  * @param search data to search for the occurence
  * @param compare comparison function used to compare two DATA items
  * @return 1 if the data is indeed in the linked list 0 otherwise
  */
int find_occurrence(list *llist, void *search, equal_op compare_func)
{
    if (llist->head == NULL)
    {
        return 0; // empty list
    }
    int count, length;
    node *curr;
    count = 0;
    length = get_size(llist);
    curr = llist->head;
    while (count < length)
    {
        if (compare_func(search, curr->data))
        {
            return 1;
        }
        curr = curr->next;
        count += 1;
    }
    return 0;
}

/** Empties the list; after this function is called, the list should be empty.
  *
  * @param llist a pointer to a linked list
  * @param free_func function used to free the nodes' data
  */
void empty_list(list *llist, list_op free_func)
{
    while (llist->size > 0)
    {
        remove_front(llist, free_func);
    }
}

/** Traverses the linked list, calling a function on each node's data.
  *
  * @param llist a pointer to a linked list
  * @param do_func a function that does something to each node's data
  */
void traverse(list *llist, list_op do_func)
{
    if (llist->head == NULL)
    {
        return;
    }
    int count, length;
    node *curr;
    count = 0;
    length = get_size(llist);
    curr = llist->head;
    while (count < length)
    {
        do_func(curr->data);
        curr = curr->next;
        count += 1;
    }
}
