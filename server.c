/**********************************
udp_ser.c: the source file of the server in udp transmission
***********************************/

#include "headsock.h"

void str_ser(int sockfd, struct sockaddr *addr, int addrlen,float error_prob);            //transmitting and receiving function


int main(int argc, char **argv)
{
	int sockfd, ret;
	struct sockaddr_in my_addr;
	struct sockaddr_in client_addr;
	float error_prob;
  
	
        if (argc !=2){
	printf("Given parameters do not match.");
	}

	error_prob=atof(argv[1]);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);          //create socket
	
	if (sockfd<0)
	{
		printf("error in socket!");
		exit(1);
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYUDP_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;//inet_addr("172.0.0.1");
	bzero(&(my_addr.sin_zero), 8);
	ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));                //bind socket
	if (ret<0)
	{
		printf("error in binding");
		exit(1);
	}
	
	printf("waiting for data\n");
	str_ser(sockfd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_in), error_prob);                //receive packet and response
  
	close(sockfd);
	exit(0);
}

void str_ser(int sockfd, struct sockaddr *addr, int addrlen, float error_prob)
{
	char buf[BUFSIZE];
	FILE *fp;
	struct ack_so ack_send;
	struct pack_so pack_received;
	int end, n = 0;
	long lseek=0;
        end = 0;
	int frame_id = 0;
	float random_num=0;
	int damage;
	int errorn=0;
	
	srand(time(NULL));

	while(!end)
     {
	
	if(error_prob<=0.0){
		damage=0;
			    }
	else{ random_num=((float)rand()/(float)(RAND_MAX))*100;

	if (random_num<= error_prob) {
		damage=1;
		}
	else {
		damage=0;    
		 }
	}
     
		if ((n= recvfrom(sockfd, &pack_received, sizeof(pack_received), 0, addr, (socklen_t *)&addrlen))==-1)  //receive the packet
			{
				printf("error when receiving\n");
				ack_send.len = -1;
				sendto(sockfd, &ack_send, sizeof(ack_send), 0, addr, addrlen);
		}else{
		if (damage==0) {
		printf("receive packet No.%d\n", pack_received.num);
		if (pack_received.data[(int)pack_received.len - 1] == '\0') 
		//pack_received.len < DATALEN
    			{
      				end = 1;
      				n --;
    			}
		memcpy((buf+lseek), pack_received.data, n);
		lseek += pack_received.len;

		ack_send.len = 0;
		ack_send.num = pack_received.num + 1;
		n = sendto(sockfd, &ack_send, sizeof(ack_send), 0, addr, addrlen);
		printf("send ACK No.%d\n", ack_send.num);
		} 
	
  	
    	
	else { ack_send.len=-1;
	       ack_send.num= pack_received.num + 1;
	       n = sendto(sockfd, &ack_send, sizeof(ack_send), 0, addr, addrlen);
	       printf("send ACK No.%d\n", ack_send.num); 
	       errorn++;
               
		}
	     }	
	}
	 


	if ((fp = fopen ("myUDPReceive.txt","wt")) == NULL) //open file to write
		{
			printf("File doesn't exist\n");
			exit(0);
		}


	fwrite (buf , 1 , lseek , fp);	       //write data into file
	fclose(fp);

	printf("a file has been successfully received!\nthe total data received is %d bytes\n",(int)lseek);
    
	printf("Total number of errors received: %d\n", errorn);
	printf("Total number of packets received: %d\n", pack_received.num);
        printf("Total number of packets sent: %d\n", (pack_received.num+errorn));
	printf("error rate: %.5f%%\n", (errorn/(float) (pack_received.num+errorn)*100.0));

}
