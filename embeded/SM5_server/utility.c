#include "utility.h"

int initServer(int port, int buf_size, int max_pending){
	int retval;
	int serv_sock;
	char buf[buf_size], tmp[20];
	struct sockaddr_in serveraddr;
	struct timeval timeout;

	serv_sock = socket(PF_INET, SOCK_STREAM, 0); // TCP socket
	if(serv_sock == -1) return -1;

	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
	setsockopt(serv_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);

	retval = bind(serv_sock, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
	if(retval == -1) return -2;

	retval = listen(serv_sock, max_pending);
	if(retval == -1) return -3;

	return serv_sock;
}

void err_quit(char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

int kbhit()
{
	struct termio stTerm;
	u_short usFlag;
	u_char uchMin;
	u_char uchTime;
	char szBuf[10];

	// 표준 입력 상태파악
	ioctl( 0, TCGETA, &stTerm );

	// 값 변경
	usFlag= stTerm.c_lflag;
	uchMin= stTerm.c_cc[ VMIN ];
	uchTime= stTerm.c_cc[ VTIME ];

	// low 모드로 설정
	stTerm.c_lflag &= ~ICANON;
	// read호출시 0개문자 읽어들임
	stTerm.c_cc[ VMIN ] = 0;
	// 시간 지연 없음
	stTerm.c_cc[ VTIME ]= 0;

	// 상태 변경
	ioctl( 0, TCSETA, &stTerm );

	// read() 호출
	if( read( 0, szBuf, 9 ) <= 0 )
	{
		// 원상태로 복구
		stTerm.c_lflag = usFlag;
		stTerm.c_cc[ VMIN ] = uchMin;
		stTerm.c_cc[ VTIME ]= uchTime;
		ioctl( 0, TCSETA, &stTerm );

		// 키가 안눌러졌음
		return 0;
	}
	else
	{
		// 원상태로 복구
		stTerm.c_lflag = usFlag;
		stTerm.c_cc[ VMIN ]= uchMin;
		stTerm.c_cc[ VTIME ]= uchTime;
		ioctl(0, TCSETA, &stTerm);

		// 키가 눌러졌음을 알림
		return 1;
	}
}

int getch(){
	int ch;
	struct termios oldt, newt;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

void printIP(){
	// 이더넷 데이터 구조체
	struct ifreq *ifr;
	struct sockaddr_in *sin;
	struct sockaddr *sa;

	// 이더넷 설정 구조체
	struct ifconf ifcfg;
	int fd;
	int n;
	int numreqs = 30;
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	// 이더넷 설정정보를 가지고오기 위해서
	// 설정 구조체를 초기화하고
	// ifreq데이터는 ifc_buf에 저장되며,
	// 네트워크 장치가 여러개 있을 수 있으므로 크기를 충분히 잡아주어야 한다.
	// 보통은 루프백주소와 하나의 이더넷카드, 2개의 장치를 가진다.
	memset(&ifcfg, 0, sizeof(ifcfg));
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc(ifcfg.ifc_len);

	for (;;) {
		/*        ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
		 ifcfg.ifc_buf = realloc(ifcfg.ifc_buf, ifcfg.ifc_len);*/
		if (ioctl(fd, SIOCGIFCONF, (char *) &ifcfg) < 0) {
			perror("SIOCGIFCONF ");
			exit;
		}
		break;
	}

	ifr = ifcfg.ifc_req;

	for (n = 0; n < ifcfg.ifc_len; n += sizeof(struct ifreq)) {
		sin = (struct sockaddr_in *) &ifr->ifr_addr;
		printf("IP    %s\n", inet_ntoa(sin->sin_addr));
		ifr++;
	}

	close(fd);
}
