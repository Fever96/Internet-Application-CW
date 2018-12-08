#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
	struct hostent *h;
	char **aliases;
	struct in_addr **addr_list;
	int i;
	struct in_addr addr;
	
	
	if(inet_aton(argv[1],&addr)!=0)
	{

		if((h = gethostbyaddr((const char*)&addr,4,AF_INET))!=NULL)
		{	
			printf("Input address:%s\n",argv[1]);	
		}
		else{
			printf("Invalid input!\n");
			exit(1);
		}
	}
	else{
		if((h = gethostbyname(argv[1]))!=NULL){	
			printf("Input hostname:%s\n",argv[1]);
		}
		else{
			printf("Invalid input!\n");
			exit(1);
		}
	}

	printf("official hostname:%s\n",h->h_name);
 	for(aliases= h->h_aliases; *aliases!= NULL; aliases++)
		printf("alias:%s\n",*aliases);	
		
	addr_list=(struct in_addr**)h->h_addr_list;
	
	for(i=0;addr_list[i]!=NULL;i++)
		printf("IP address:%s\n",inet_ntoa(*addr_list[i]));

	return 0;
}
 	
