#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define    MAXLINE        1024

void usage(char *command)
{
	printf("usage :%s ipaddr portnum filename\n", command);
	exit(0);
}
int main(int argc,char **argv)
{
	struct sockaddr_in     serv_addr;
	char                   buf[MAXLINE];
	int                    sock_id;
	int                    recv_len;
	int                    write_leng;
	FILE                   *fp;
	int                    i_ret;
	char                   name[255];
	if (argc != 4) {
		usage(argv[0]);
	}

	if ((sock_id = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		printf("Create socket failed\n");
		exit(0);
	}//分配socket
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));  //端口存入
	inet_pton(AF_INET, argv[1], &serv_addr.sin_addr); //地址存入

	i_ret = connect(sock_id, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));  //建立连接
	if (-1 == i_ret) {
		printf("Connect socket failed\n");
		return -1;
	}
	send(sock_id, argv[3], strlen(argv[3]), 0);   //把文件名发送到服务端
	strcat(name,argv[3]);
	strcat(name,".test");   //给文件名加.test

	if((fp = fopen(name,"w")) == NULL){
		printf("FIle not exist\n");
		exit(0);
	}	

	bzero(buf, MAXLINE);

	while (recv_len = recv(sock_id, buf, MAXLINE, 0)) {
		//从服务端接收文件
		if(recv_len < 0) {
			printf("Recieve Data From Server Failed!\n");
			break;
		}
		printf("#");
		write_leng = fwrite(buf, sizeof(char), recv_len, fp);  //写入文件
		if (write_leng < recv_len) {
			printf("Write file failed\n");
			break;
		}
		bzero(buf,MAXLINE);
	}
	close(sock_id);//关闭socket
	printf("Finish\n");
	fclose(fp);//关闭文件
	return 0;
}