#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define    MAXLINE        1024

void usage(char *command)
{
	printf("usage :%s portnum\n", command);
	exit(0);
}
int main(int argc,char **argv)
{
	struct sockaddr_in     serv_addr;
	struct sockaddr_in     clie_addr;
	char                   buf[MAXLINE];
	char                   filename[255];
	int                    sock_id;
	int                    link_id;
	int                    read_len;
	int                    send_len;
	int                    clie_addr_len;
	FILE                   *fp;

	if (argc != 2) {
		usage(argv[0]);
	}

	if ((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Create socket failed\n");
		exit(0);
	}//分配socket

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));//端口存入
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 ) {
		printf("Bind socket failed\n");
		exit(0);
	}

	if (-1 == listen(sock_id, 10)) {
		printf("Listen socket failed\n");
		exit(0);
	}//监听

	while (1) {//无限循环
		memset(filename, 0, sizeof(filename));
		clie_addr_len = sizeof(clie_addr);
		link_id = accept(sock_id, (struct sockaddr *)&clie_addr, &clie_addr_len);//新socket
		if (-1 == link_id) {
			printf("Accept socket failed\n");
			exit(0);
		}
		if(recv(link_id,filename,255,0)>0) { //接收文件名
			if ((fp = fopen(filename, "r")) == NULL) {
				printf("Open file failed\n");
				exit(0);
			}//以读取方式打开文件
			bzero(buf, MAXLINE);
			while ((read_len = fread(buf, sizeof(char), MAXLINE, fp)) >0 ) {
				send_len = send(link_id, buf, read_len, 0);//发送文件
				if ( send_len < 0 ) {
					printf("Send file failed\n");
					exit(0);
				}
				bzero(buf, MAXLINE);
			}
		}
		printf("Finish\n");
		fclose(fp);
		close(link_id);
	}
	close(sock_id);
	return 0;
}