/*
 *  FIFO.c
 *
 *  Created om: Mar 23, 2021
 *  	Author: Alex
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fifo.h"
void push(int btn, int state, struct node **head)
{
	if (*head == NULL)
	{
		*head = malloc(sizeof(struct node));
		(*head)->btn = btn;
		(*head)->state = state;
		(*head)->next = NULL;
	}
	else
	{
		struct node *temp;
		temp = *head;
		while(temp->next != NULL)
		{
			temp = temp->next;
		}
		struct node * node;
		node = malloc(sizeof(struct node));
		node->btn = btn;
		node->state = state;
		node->next = NULL;
	}
}

void pop(int *btn, int *state, struct node **head)
{
	if (*head == NULL)
	{
		*btn = 2;
		*state = 2;
	}
	else
	{
		struct node *temp;
		temp = *head;
		*head = (*head)->next;
		*btn = temp->btn;
		*state = temp->state;
		free(temp);
	}
}

