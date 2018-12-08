#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), sendto() and recvfrom() */
#include <sys/ioctl.h> 
#include <netinet/in.h> 
#include <net/if.h> 
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h> /* for atoi() and exit() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */
#include <time.h>

#define MAX_SIZE 512 

unsigned int getIP();
int availableIPNumber(char file[]);
void offerIP(unsigned char MAC[6]);
int checkIP(unsigned int IP, unsigned char MAC[6]);
void releaseIP(unsigned int IP);
void renewIP(unsigned int IP);
int LEASETIME;

int main(int argc, char *argv[]){
    int sock; /* Socket */
    struct sockaddr_in svrAddr; /* Local address */
    struct sockaddr_in cltAddr; /* Client address */
    struct sockaddr_in broadcastAddr; /* Broadcast address */
    unsigned int cliAddrLen; /* Length of client address */
	struct ifreq if_eth1;
	
    char recvBuf[MAX_SIZE]; /* Buffer for receive */
    char sendBuf[MAX_SIZE]; /* Buffer for send */

    int recvMsgSize; /* Size of received message */
    int i; /* Counter */

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
       printf("socket() failed.\n");
	
	/*bind socket to eth1*/
    int j = 1;
	strcpy(if_eth1.ifr_name,"eth1");
	socklen_t len=sizeof(j);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &j, len);
	if (setsockopt(sock,SOL_SOCKET,SO_BINDTODEVICE,(char *)&if_eth1,sizeof(if_eth1))<0){
		printf("bind socket to eth1 error\n");
	}
    // Broadcast
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));/*Zero out structure*/
    broadcastAddr.sin_family = AF_INET; /* Internet addr family */
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");/*Server IP address*/
    broadcastAddr.sin_port = htons(68); /* Server port */

    /* Local address structure */
    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;
    svrAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    svrAddr.sin_port = htons(67);
    /* Bind to the local address */
    if ((bind(sock, (struct sockaddr *) &svrAddr, sizeof(svrAddr))) < 0)
       printf("bind() failed.\n");
    
    for(;;)
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(cltAddr);
        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, recvBuf, MAX_SIZE, 0,(struct sockaddr *) &cltAddr, &cliAddrLen)) < 0)
            printf("recvfrom() failed.\n");
        memcpy(sendBuf, recvBuf, 243);

        int sendSize = 240;

        FILE* upop;
        if ((upop=fopen("dhcp.config","r"))==NULL)  
        {     
            printf("Cannot open dhcp.config\n");  
            return 0;  
        }  
        char string[16];

        // Subnet Mask
        fscanf(upop,"%s",string);
        unsigned int MASK = (inet_addr(string));

        // Router
        fscanf(upop,"%s",string);
        unsigned int ROUTER = (inet_addr(string));

        // DNS
        fscanf(upop,"%s",string);
        unsigned int DNS = (inet_addr(string));

        // IP address lease time
        fscanf(upop,"%s",string);
        LEASETIME = (atoi(string));

        // IP (prepared)
        fscanf(upop,"%s",string);
        unsigned int IP = (inet_addr(string));

        fclose(upop);  
                    
        switch(recvBuf[242])
        {
            case 0x01: // DHCP Discover
            {
                printf("Received: Discover\n");
                if(availableIPNumber("dhcp.config") <= 0)
                {
                    sendSize = 0;
                    printf("No available IP Address.\n");
                }
                else
                {
                    printf("Send: Offer\n");
                    
                    sendBuf[0]=2;//From server to client
					
                   	unsigned int yourIPAddress = IP;
                    memcpy(&sendBuf[16], &yourIPAddress, sizeof(yourIPAddress));

                    // Option 53
                    sendBuf[242]=2;

                    // Option 54
                    sendBuf[243]=54;
					sendBuf[244]=4;//54 Length
					unsigned int ServerIdentifier = getIP();
                   	memcpy(&sendBuf[245], &ServerIdentifier, 4);
						
	                // Option 51
					sendBuf[249]=51;
	                sendBuf[250]=4;// 51 Length
	                unsigned int LeaseTime = htonl(LEASETIME);
                	memcpy(&sendBuf[251], &LeaseTime, 4);
                 
                    // Option 1
                    sendBuf[255]=1;
                    sendBuf[256]=4;//1 Length
                    memcpy(&sendBuf[257], &MASK, 4);

                    // Option 3
                    sendBuf[261]=3;
                    sendBuf[262]=4;//3 length
                    memcpy(&sendBuf[263], &ROUTER, 4);//Router

                    // Option 6
                    sendBuf[267]=6;
                    sendBuf[268]=4;//6 length
					memcpy(&sendBuf[269], &DNS, 4);//DNS

                    // Option 58
                    sendBuf[273]=58;
                    sendBuf[274]=4;//58 length
                    unsigned int T1=htonl(LEASETIME / 2);
                    memcpy(&sendBuf[275], &T1, 4);

                    // Option 59
                    sendBuf[279]=59;
                    sendBuf[280]=4;//59 length
                    unsigned int T2=htonl(LEASETIME*7 / 8);
                    memcpy(&sendBuf[281], &T2, 4);

                    // Option 255
					sendBuf[285]=255;//end

                    // Padding
                    unsigned char padding[312 - 286];
                    for(i = 286; i < 312 ; i++){
                        sendBuf[i] = 0;
                    }
                    sendSize = 312;
                }
                break;
            }
            case 0x03: // DHCP Request (Response ACK)
            {
                unsigned int clientIP;
                memcpy(&clientIP, &recvBuf[12], sizeof(unsigned int));
                if(clientIP == 0x00000000) // For Discover-Offer-Request-ACK loop
                {
                        printf("Received: Request (address aquisition)\n");
						printf("Send:ACK\n");
                        
						sendBuf[0]=2;

                        unsigned int yourIPAddress = IP;
                   		memcpy(&sendBuf[16], &yourIPAddress, sizeof(yourIPAddress));

                        // Option 53
                        sendBuf[242]=5;

	                    // Option 54
	                    sendBuf[243]=54;
						sendBuf[244]=4;//54 Length
						unsigned int ServerIdentifier = getIP();
                   		memcpy(&sendBuf[245], &ServerIdentifier, 4);
						
	                    // Option 51
						sendBuf[249]=51;
	                    sendBuf[250]=4;// 51 Length
	                    unsigned int LeaseTime = htonl(LEASETIME);
                    	memcpy(&sendBuf[251], &LeaseTime, 4);
	                    
	                    // Option 1
	                    sendBuf[255]=1;
	                    sendBuf[256]=4;//1 Length
	                    memcpy(&sendBuf[257], &MASK, 4);
	
	                    // Option 3
	                    sendBuf[261]=3;
	                    sendBuf[262]=4;//3 length
	                    memcpy(&sendBuf[263], &ROUTER, 4);//Router
	
	                    // Option 6
	                    sendBuf[267]=6;
	                    sendBuf[268]=4;//6 length
						memcpy(&sendBuf[269], &DNS, 4);//DNS
	
	                    // Option 58
	                    sendBuf[273]=58;
	                    sendBuf[274]=4;//58 length
	                    unsigned int T1=htonl(LEASETIME / 2);
	                    memcpy(&sendBuf[275], &T1, 4);
	
	                    // Option 59
	                    sendBuf[279]=59;
	                    sendBuf[280]=4;//59 length
	                    unsigned int T2=htonl(LEASETIME*7 / 8);
	                    memcpy(&sendBuf[281], &T2, 4);
	
	                    // Option 255
						sendBuf[285]=255;//end

	                    // Padding
	                    unsigned char padding[312 - 286];
	                    for(i = 286; i < 312 ; i++){
	                        sendBuf[i] = 0;
	                    }
                        sendSize = 312;

                        // Modify IP pool & add the client into the log
                        unsigned char MAC[6];
                        memcpy(&MAC, &recvBuf[28], sizeof(unsigned char) * 6);

                        offerIP(MAC);
                    }
                else // For renew the IP address
                {
                    // check in the dhcp.lease
                    unsigned short bootFlags;
                    memcpy(&bootFlags, &sendBuf[10], sizeof(unsigned short));

                    if(bootFlags == 0x00)
                    {
                        printf("Send: (Unicast) ");
                    }
                    else
                    {
                        printf("Send: (Broadcast) ");
                    }

                    sendBuf[0]=2;

                    unsigned int requestedIP;
                    memcpy(&requestedIP, &recvBuf[12], sizeof(unsigned int));

                    unsigned char requestedMAC[6];
                    memcpy(&requestedMAC, &recvBuf[28], sizeof(unsigned char) * 6);

                    int IPAddressLegal = checkIP(requestedIP, requestedMAC);

                    if(IPAddressLegal == 1) // Allow the client to renew the IP (ACK)
                    {
                        struct sockaddr_in sin;
                        sin.sin_addr.s_addr = requestedIP;
 
                        printf("ACK\nRenew: %s\n", inet_ntoa(sin.sin_addr));
                        // Renew the recording in the dhcp.lease file
                        renewIP(requestedIP);

                        // Release the IP
                        memcpy(&sendBuf[16], &clientIP, sizeof(clientIP));
						
                        // Option 53
                        sendBuf[242]=5;

                         // Option 54
	                    sendBuf[243]=54;
						sendBuf[244]=4;//54 Length
						unsigned int ServerIdentifier = getIP();
                   		memcpy(&sendBuf[245], &ServerIdentifier, 4);
						
	                    // Option 51
						sendBuf[249]=51;
	                    sendBuf[250]=4;// 51 Length
	                    unsigned int LeaseTime = htonl(LEASETIME);
                    	memcpy(&sendBuf[251], &LeaseTime, 4);
	                    
	                    // Option 1
	                    sendBuf[255]=1;
	                    sendBuf[256]=4;//1 Length
	                    memcpy(&sendBuf[257], &MASK, 4);
	
	                    // Option 3
	                    sendBuf[261]=3;
	                    sendBuf[262]=4;//3 length
	                   memcpy(&sendBuf[263], &ROUTER, 4);//Router
	
	                    // Option 6
	                    sendBuf[267]=6;
	                    sendBuf[268]=4;//6 length
						memcpy(&sendBuf[269], &DNS, 4);//DNS
	
	                    // Option 58
	                    sendBuf[273]=58;
	                    sendBuf[274]=4;//58 length
	                    unsigned int T1=htonl(LEASETIME / 2);
	                    memcpy(&sendBuf[275], &T1, 4);
	
	                    // Option 59
	                    sendBuf[279]=59;
	                    sendBuf[280]=4;//59 length
	                    unsigned int T2=htonl(LEASETIME*7 / 8);
	                    memcpy(&sendBuf[281], &T2, 4);
	
	                    // Option 255
						sendBuf[285]=255;//end

	                    // Padding
	                    for(i = 286; i < 312 ; i++){
	                        sendBuf[i] = 0;
	                    }
                        
                        sendSize = 312;
                    }
                    else // Does not allow the client renew the IP (NAK)
                    {
                        printf("NAK\n");
                        // Option 53
                        sendBuf[242]=6;

                        // Option 54
                        sendBuf[243]=54;
						sendBuf[244]=4;//54 Length
						unsigned int ServerIdentifier = getIP();
                   		memcpy(&sendBuf[245], &ServerIdentifier, 4);
                   		
                        // Option 255
						sendBuf[249]=255;//end

	                    // Padding
	                    for(i = 250; i < 312 ; i++){
	                        sendBuf[i] = 0;
	                    }
                        sendSize = 312;
                    }
                }
                break;
            }
            case 0x07: // DHCP Release (Retrive the IP address without sending the ACK)
            {
                printf("Receive: Release\n");
                unsigned int releasedIP;
                memcpy(&releasedIP, &recvBuf[12], sizeof(unsigned int));

                struct sockaddr_in sin;
                sin.sin_addr.s_addr = releasedIP;
                printf("Release: %s\n", inet_ntoa(sin.sin_addr));

                releaseIP(releasedIP);
                break;
            }
            case 0x08: // DHCP Inform (Response ACK without Option 51 IP address Lease Time)
            {
                printf("Receive: Inform\n");
                printf("Send: ACK\n");

                sendBuf[0]=2;

                // Option 53
                        sendBuf[242]=5;

                         // Option 54
	                    sendBuf[243]=54;
						sendBuf[244]=4;//54 Length
						unsigned int ServerIdentifier = getIP();
                   		memcpy(&sendBuf[245], &ServerIdentifier, 4);
	                    
	                    // Option 1
	                    sendBuf[249]=1;
	                    sendBuf[250]=4;//1 Length
	                    memcpy(&sendBuf[251], &MASK, 4);
	
	                    // Option 3
	                    sendBuf[255]=3;
	                    sendBuf[256]=4;//3 length
	                    memcpy(&sendBuf[257], &ROUTER, 4);//Router
	
	                    // Option 6
	                    sendBuf[261]=6;
	                    sendBuf[262]=4;//6 length
						memcpy(&sendBuf[263], &DNS, 4);//DNS
	
	                    // Option 58
	                    sendBuf[267]=58;
	                    sendBuf[268]=4;//58 length
	                    unsigned int T1=htonl(LEASETIME / 2);
	                    memcpy(&sendBuf[269], &T1, 4);
	
	                    // Option 59
	                    sendBuf[273]=59;
	                    sendBuf[274]=4;//59 length
	                    unsigned int T2=htonl(LEASETIME*7 / 8);
	                    memcpy(&sendBuf[275], &T2, 4);
	
	                    // Option 255
						sendBuf[279]=255;//end

	                    // Padding
	                    for(i = 280; i < 312 ; i++){
	                        sendBuf[i] = 0;
	                    }
			sendSize=312;
                break;
            }
            default:{
                break;
            }
        }
        
        /* Send received datagram back to the client */
        if((recvBuf[242] == 0x01 || recvBuf[242] == 0x03 || recvBuf[242] == 0x08))
        {
            int length = 0;
            unsigned short q;
            memcpy(&q, &sendBuf[10], sizeof(unsigned short));
            if(q == 0x00){;
                length = sendto(sock, sendBuf, sendSize, 0, (struct sockaddr *) &cltAddr, sizeof(cltAddr));
               if (length<0) printf("Send failed");
            }
            else if(q == 0x80){
                length = sendto(sock, sendBuf, sendSize, 0, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr));
            }
        }
    }
}

// Get IP
unsigned int getIP()
{
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;

    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        perror("socket");

    strncpy(ifr.ifr_name, "eth1", IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
        perror("ioctl"); 

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));

    unsigned int IP = sin.sin_addr.s_addr;
    
    return IP;
}

// Change the IP address in the dhcp.config
void offerIP(unsigned char MAC[6])
{
    FILE *file1;
    FILE *file2;
    char string[16];
    char IP[16];

    file1 = fopen("dhcp.config", "r");
    file2 = fopen("dhcp.config2", "w");

    // Subnet Mask
    fscanf(file1, "%s", string);
    fprintf(file2, "%s\n", string);

    // Router
    fscanf(file1, "%s", string);
    fprintf(file2, "%s\n", string);

    // DNS
    fscanf(file1, "%s", string);
    fprintf(file2, "%s\n", string);

    // IP release time
    fscanf(file1, "%s", string);
    fprintf(file2, "%s\n", string);

    // Remove IP
    fscanf(file1, "%s", IP);//skip ip

    while (fscanf(file1, "%s", string) != EOF)
        fprintf(file2, "%s\n", string);

    fclose(file1);
    fclose(file2);
    if ((remove("dhcp.config") == 0) && (rename("dhcp.config2", "dhcp.config") != 0))
        printf("File Operation Failed!");

    // Write the info into dhcp.lease
    file2 = fopen("dhcp.lease", "a");
    time_t t;  
    t = time(NULL);  
    struct tm *lt;  
    int timestamp = time(&t);  
    
    fprintf(file2, "%d %02x%02x%02x%02x%02x%02x %s\n", timestamp, MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5], IP);
    fclose(file2);
}

// Get IP address number
int availableIPNumber(char file[])
{
    char string[16];
    int h = 0;
    FILE *fp;

    if ((fp = fopen(file, "r")) == NULL)  
        return 0;  

    while(!feof(fp))  
    {  
        fscanf(fp, "%s", string);  
        h++;
    }
    return h - 5;
}

// Check whether the IP is in the lease
int checkIP(unsigned int IP, unsigned char MAC[6])
{
    FILE *file1;
    char string[50];

    file1 = fopen("dhcp.lease", "r");
    
    while(!feof(file1))
    {
        fgets(string, 50, file1);

        if(strlen(string) >= 1)
            string[strlen(string) - 1] = '\0';
        else
           break;

        char* timeStampStr = NULL;
        char* IPStr = NULL;
        char* MACStr = NULL;

        timeStampStr = strtok(string, " ");//split line
        int timeStamp = atoi(timeStampStr);
        timeStamp += LEASETIME;//time it leases

        time_t t;  
        t = time(NULL);  
        struct tm *lt;  
        int timeStampNow = time(&t);  
        MACStr = strtok(NULL, " ");
        IPStr = strtok(NULL, " ");

        if(IP == inet_addr(IPStr))
        {
            char MACASCII[12];
            sprintf(MACASCII, "%02x%02x%02x%02x%02x%02x", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

            if(strcmp(MACASCII, MACStr) == 0)
            {
                if(timeStamp >= timeStampNow)
                {
                    return 1;
                }
                else
                    return -1;
            }
            else
                return -1;
        }
        else
        {
            int i = 0;
            for(i = 0; i < 50; i++)
                string[i] = '\0';
            continue;
        }
    }
    fclose(file1);
    return -1;
}

// Release the IP when received release
void releaseIP(unsigned int IP)
{
    FILE *file1;
    FILE *file2;
    int i = 0;
    int flag = 0;
    char string[50];

    struct sockaddr_in sin;
    sin.sin_addr.s_addr = IP;

    file1 = fopen("dhcp.lease", "r");
    file2 = fopen("dhcp.lease2", "w");
    //delete lease
    while(!feof(file2))  
    {
        fgets(string, 50, file1);

        if(strlen(string) >= 1)
            string[strlen(string) - 1] = '\0';
        else
           break;

        if(strstr(string, inet_ntoa(sin.sin_addr)) != NULL)
        {
            for(i = 0; i < 50; i++)//init string
                string[i] = '\0';
            flag = 1;
            continue;
        }
        else
        {
            fprintf(file2, "%s\n", string);//init string
            for(i = 0; i < 50; i++)
                string[i] = '\0';
            continue;
        }
    }

    if ((remove("dhcp.lease") == 0) && (rename("dhcp.lease2", "dhcp.lease") != 0))
        printf("File Operation Failed!");
    fclose(file1);
    fclose(file2);
    //append
    if(flag == 1)
    {
        file2 = fopen("dhcp.config", "a");
        
        fprintf(file2, "%s\n", inet_ntoa(sin.sin_addr));
        fclose(file2);
    }
}

// Renew the IP address
void renewIP(unsigned int IP)
{
    FILE *file1;
    FILE *file2;

    char string[50];

    file1 = fopen("dhcp.lease", "r");
    file2 = fopen("dhcp.lease2", "w");
    
    while(!feof(file1))
    {
        fgets(string, 50, file1);

        char stringCpy[50];
        strcpy(stringCpy, string);

        if(strlen(string) >= 1)
            string[strlen(string) - 1] = '\0';
        else
           break;

        char* timeStampStr = NULL;
        char* IPStr = NULL;
        char* MACStr = NULL;

        timeStampStr = strtok(string, " ");
        MACStr = strtok(NULL, " ");
        IPStr = strtok(NULL, " ");
        //renew to now
        if(IP == inet_addr(IPStr))
        {
            time_t t;  
            t = time(NULL);  
            struct tm *lt;  
            int Now = time(&t);

            fprintf(file2, "%d %s %s\n", Now, MACStr, IPStr);
        }
        else
        {
            fprintf(file2, "%s", stringCpy);//keep
        }
        int i = 0;
        for(i = 0; i < 50; i++)
            string[i] = '\0';
    }
    
    fclose(file1);
    fclose(file2);

    if ((remove("dhcp.lease") == 0) && (rename("dhcp.lease2", "dhcp.lease") != 0))
        printf("File Operation Failed!");
}
