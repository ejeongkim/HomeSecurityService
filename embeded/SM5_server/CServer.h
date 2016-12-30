/*
 * CServer.h
 *
 *  Created on: 2014. 11. 4.
 *      Author: woosung
 */

#ifndef CSERVER_H_
#define CSERVER_H_

#include <pthread.h>
#include "utility.h"

#define BUFSIZE 1024 // recvBuffer¿« ≈©±‚
unsigned char SENSOR_REQ_PACKET[] = {0xA0, 0x0A, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0A, 0xA0};

void printPacketInfo(unsigned char* packet);
void sendPacket(int clientSockfd, unsigned char* data);
void * recvThread(void * vpData);

#endif /* CSERVER_H_ */
