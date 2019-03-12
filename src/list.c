#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "list.h"


void insertBehind(list *l, const char *num, int num_length) {
    list *pom;
    pom = malloc(sizeof(list));

    pom->next = l->next;
    pom->num_length = num_length;
    l->next = pom;

    pom->num = malloc((num_length + 1) * sizeof(char));
    pom->num[num_length] = '\0';
    strcpy(pom->num, num);
}

bool insertIntoSortedList(list *l, const char *num, int num_length, list **head) {
/* do posortowanej listy l wstawia x*/
    list *act, *next;

/*akt – aktualny, next – następnik akt*/
    if (l == NULL || strcmp(num, l->num) < 0) {

        size_t phwd_key = strlen(num);
        act = malloc(sizeof(list));
        act->next = l;

        act->num = malloc((phwd_key + 1) * sizeof(char));
        act->num[phwd_key] = '\0';
        strcpy(act->num, num);
        act->num_length = num_length;

        *head = act;
        return true;
    } /* wstawiliśmy x na początek listy */
    else {/* l!=NULL  && l->movie_id<x */

        *head = l;
        act = l;
        next = act->next;
        while (next != NULL && strcmp(num, next->num) >= 0) {
            act = next;
            next = act->next;
        }
        bool tmp = act->num != num && strcmp(num, act->num) != 0 &&
                   (act->next == NULL || act->next->num != num);

        if (tmp) {
            insertBehind(act, num, num_length);
            return true;
        }
    }
    return false;
}

void removeList(list **l) {
    if ((*l) == NULL) return;
    list *current = *l;
    list *next;
    while (current != NULL) {
        next = current->next;
        free(current->num);
        free(current);
        current = next;
    }
    *l = NULL;
}
