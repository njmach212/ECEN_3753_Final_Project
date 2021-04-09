/*
 * fifo.h
 *
 *  Created on: Mar 23, 2021
 *      Author: Alex
 */

#ifndef SRC_HEADER_FILES_FIFO_H_
#define SRC_HEADER_FILES_FIFO_H_


struct node
{
	bool btn;
	bool state;
	struct node *next;
};

struct Speed
{
	int speed;
	int incriment;
	int decriment;
};

struct Direction
{
	int direction;
	int left;
	int right;
	int time;
};

void pop(int *btn, int *state, struct node **head);

void push(int btn, int state, struct node **head);
#endif /* SRC_HEADER_FILES_FIFO_H_ */
