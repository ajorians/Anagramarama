/*
  Anagramarama - A word game.  Like anagrams?  You'll love anagramarama!
  Copyright (C) 2003  Colm Gallagher

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Contact Details: colm@coralquest.com
  12 Weston Terrace, West Kilbride, KA23 9JX.  Scotland.
*/

//#include <stdlib.h>
//#include <string.h>
#include <os.h>
#include "linked.h"

/*
 * Count the length of the linked list.
 */

int
Length(struct node *head)
{
    struct node *current = head;
    int count = 0;
    for ( ; current; current = current->next) {
        ++count;
    }
    return count;
}

/*
 * swap the data content of two linked list nodes without changing
 * the position of the node within the list
 */

void
swap(struct node **from, struct node **to)
{
    char *word = (*from)->anagram;
    int len = (*from)->length;

    (*from)->anagram = (*to)->anagram;
    (*from)->length = (*to)->length;
    (*to)->anagram = word;
    (*to)->length = len;
}

/*
 * sort the list first alphabetically and then by increasing word length
 */

void
sort(struct node **headRef)
{
    struct node* left, *right;
    int completed = 0;

    while (!completed) {
        left = *headRef;
        right = left->next;
        completed = 1;
        for (; left && right; left = left->next, right = right->next) {
            if (strcmp(left->anagram, right->anagram) > 0) {
                swap(&left, &right);
                completed = 0;
            }
        }
    }

    completed = 0;
    while (!completed) {
        left = *headRef;
        right = left->next;
        completed = 1;
        for (; left && right; left = left->next, right = right->next) {
            if (left->length > right->length) {
                swap(&left, &right);
                completed = 0;
            }
        }
    }
}    

/*
 * Free the list and the allocated data in each node
 */

void
destroyAnswers(struct node** headRef)
{
    struct node* current = *headRef;
    struct node* previous = *headRef;
    
    while (current != NULL){
        free(current->anagram);
        previous = current;
        current = current->next;
        free(previous);
        previous = NULL;
    }
    
    *headRef = NULL;
}

char* Copy(const char* pstr)
{
   char *d = malloc(strlen(pstr) + 1);
   if( d == NULL )return NULL;
   strcpy(d, pstr);
   return d;
}

/***********************************************************/
void
push(struct node **headRef, const char *anagram)
{
    struct node * current = *headRef;
    int len;
    int duplicate = 0;

    /* walk the list first, so we can ignore duplicates...
     * this is probably slower than clearing duplicates at the end 
     * but simpler to write in the first instance
     */
    while (current != NULL) {
        if (!strcmp(anagram, current->anagram)){
            duplicate = 1;
            break;
        }
        current = current->next;
    }

    if (!duplicate) {
        struct node *newNode = malloc(sizeof(struct node));
        len = strlen(anagram);
        newNode->anagram = Copy(anagram);//strdup(anagram);
        newNode->length = len;
        newNode->found = 0;
        newNode->guessed = 0;
        newNode->next = *headRef;

        *headRef = newNode;
    }
}

/*
 * Local variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
