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

struct Physics
{
	double Gain;
	double st;
	double ed;
	double delta_t;
	double Dir;
	double gravity;
	double mass;
	double length;
	double xmin;
	double xmax;
	double theta;
	double h_velocity;
	double v_velocity;
	double h_acceleration;
	double v_acceleration;
	double hb_force;
	double vb_force;
	double hc_force;
	double vc_force;
	double v_force;
	double h_force;
	double h_position;
	double v_position;
	double hc_position;
	int gm;
};

struct lcd_info
{
	double x_position;
	double y_position;
	double c_postion;
	int gm;
};

void pop(int *btn, int *state, struct node **head);

void push(int btn, int state, struct node **head);
#endif /* SRC_HEADER_FILES_FIFO_H_ */
