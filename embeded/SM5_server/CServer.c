#include "CServer.h"
 
int program_run = 1; // 프로그램 종료 시 모든 스레드를 종료하기 위한 플래그
int client_cnt = 0; // 클라이언트의 갯수 카운트
 
int main(int argc, char * argv[]) {
	pthread_t thread_id;
	struct sockaddr_in client_addr;
	int server_sock, client_sock;
	int addr_len;
 
	printIP();
 
	//initServer(port, buffer_size, max_pending)
	server_sock = initServer(7777, BUFSIZE, 5);
 
	if(server_sock==-1) err_quit("socket() error");
	else if(server_sock==-2) err_quit("bind() error");
	else if(server_sock==-3) err_quit("listen() error");
 
	printf("Server Started\n");
 
	while(1)
	{
		//아무키나 누르면 서버 종료
		if(kbhit()) break;
 
		//클라이언트 접속 기다림 (1초마다 체크)
		addr_len = sizeof(client_addr);
		client_sock = accept(server_sock, (struct sockaddr*) &client_addr, &addr_len);
 
		// 클라이언트가 접속했을 경우 통신 스레드 생성
		if(client_sock>0){
			pthread_create(&thread_id, 0, recvThread, (void*)client_sock);
		}
	}
 
	printf("Server Closed\n");
 
	program_run = 0;
 
	close(server_sock);
}
 
void printPacketInfo(unsigned char* packet){
	int id = packet[3];
 
	int aa = packet[4];//(packet[4]<<8)|packet[5];
	int ba = packet[5];
	int ca = packet[6];
	int da = packet[7];
	int ea = packet[8];
	int fa = packet[9];
	int ga = packet[10];
	int ha = packet[11];
	int ia = packet[12];
	int ja = packet[13];
 
	int i = 0;
	unsigned char csc = packet[14];
	unsigned char sum = 0;
 
	for(i=2;i<14;i++){
		sum += packet[i];
	}
 
	if(sum==csc){
		printf("[%03d] aa:%d, ba:%d, ca:%d, da:%d, ea:%d\nfa:%d, ga:%d, ha:%d, ia:%d, ja:%d\n", aa, ba, ca, da, ea,fa, ga, ha, ia, ja);
	} else {
		printf("[%03d] Checksum error\n", id);
	}
}
 
void sendPacket(int clientSockfd, unsigned char* data){
 
	unsigned char packet[] = {0xA0, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xA0};
	unsigned char csc = 0;
	int i=0;
 
	if(sizeof(data)!=10) return;
 
	for(i=0;i<10;i++){
		packet[4+i] = data[i];
		csc += data[i];
	}
 
	packet[14] = csc;
 
	send(clientSockfd, packet, sizeof(packet), 0);
}
 
void * recvThread(void * vpData)
{
	int clientSockfd = (int)vpData;
	int recvLen;
	int i;
	int packet_pt = 0;
	unsigned char recvBuffer[BUFSIZE];
	unsigned char packet_buffer[20];
	unsigned char packet_started = 0;
 
	printf("Client connected (Total:%d)\n", ++client_cnt);
 
	sleep(1);
 
	// 연결된 모듈에게 센서값 전송을 시작하라는 패킷을 보낸다.
	if ((send(clientSockfd, SENSOR_REQ_PACKET, sizeof(SENSOR_REQ_PACKET), 0))<0) {
		printf("send() ERROR\n");
		printf("Client disconnected\n");
		close(clientSockfd);
		client_cnt--;
		return 0;
	}
 
	while (program_run) {
		// clientSockfd로 들어오는 데이터를 받아 recvBuffer에 저장
		if ((recvLen = recv(clientSockfd, recvBuffer, BUFSIZE - 1, 0)) < 0) continue;
 
		// 클라이언트가 연결을 끊는다면 while문 종료
		if (recvLen == 0) break;
 
		// 패킷 데이터 저장
		for(i=0;i<recvLen;i++){
			packet_buffer[packet_pt++] = recvBuffer[i];
 
			if(packet_started){ //패킷의 헤더를 받은 상태라면 받은 데이터가 마지막인지 또는 버퍼 크기가 초과되었는지 검사
				// 마지막 패킷 데이터라면 printPacketInfo 함수 호출 후 초기화
				if(packet_buffer[packet_pt-2]==0x0A&&packet_buffer[packet_pt-1]==0xA0&&packet_pt==17){
					printPacketInfo(packet_buffer);
					//unsigned char data[10] = {0,0,0,0,0,0,0,0,0,0};
					//sendPacket(clientSockfd, data);
					packet_pt = 0;
					packet_started = 0;
					continue;
				}
 
				// 버퍼 크기가 초과되었다면 초기화
				if(packet_pt>=20){
					packet_pt = 0;
					packet_started = 0;
					printf("BUFFER OVERFLOW ERROR\n");
				}
			} else { //패킷의 헤더를 받지 않은 상태라면 받은 데이터가 헤더인지 검사
				// 받은 2개의 데이터가 헤더가 아니라면 초기화
				// 헤더라면 packet_started를 1로 변경
				if(packet_pt>=2){
					if(packet_buffer[0]==0xA0 && packet_buffer[1]==0x0A){
						packet_started = 1;
					} else {
						packet_pt = 0;
						printf("START PACKET ERROR\n");
					}
				}
			}
		}
	}
 
	client_cnt--;
	close(clientSockfd);
	printf("Client disconnected\n");
 
	return 0;
}
