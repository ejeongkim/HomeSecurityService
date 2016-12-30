/*
 * utility.h
 *
 *  Created on: 2014. 11. 12.
 *      Author: woosung
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <termios.h>

int initServer(int port, int buf_size, int max_pending);
void err_quit(char *msg);
int kbhit();
int getch();
void printIP();

#endif /* UTILITY_H_ */
