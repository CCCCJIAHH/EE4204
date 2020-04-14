/*******************************
udp_client.c: the source file of the client in udp transmission
********************************/

#include "headsock.h"

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len); //transmission function
void tv_sub(struct  timeval *out, struct timeval *in);	    //calcu the time interval between out and in

int main(int argc, char **argv)
{
	int sockfd;
	float ti, rt;
	long len;
	struct sockaddr_in ser_addr;
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;

	if (argc != 2) {
		printf("parameters not match");
	}

	sh = gethostbyname(argv[1]);	                      //get host's information
	if (sh == NULL) {
		printf("error when gethostby name");
		exit(0);
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);            //create the socket
	if (sockfd<0)
	{
		printf("error in socket");
		exit(1);
	}
  
	addrs = (struct in_addr **)sh->h_addr_list;
	printf("canonical name: %s\n", sh->h_name);					//print the remote host's information
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);
	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}
  
	ser_addr.sin_family = AF_INET;                                                      
	ser_addr.sin_port = htons(MYUDP_PORT);
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);
	
	if((fp = fopen ("myfile.txt","r+t")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}

  //perform the transmission and receiving
	ti = str_cli(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len);
	rt = (len/(float)ti);           //caculate the average transmission rate
	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti, (int)len, rt);

	close(sockfd);
	printf("sockfd close\n");
	fclose(fp);
  	printf("file close\n");
	exit(0);
}

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len)
{
	char *buf;
	long lsize, ci;   //lsize=entire file size; ci=curr index of buf    
	struct ack_so ack_received;
	struct pack_so pack_send;
	pack_send.num = 0;
	int n, slen;      // slen=len of string to send 
	float time_inv = 0.0;
	struct timeval sendt, recvt; //the timer for the whole process
  	int status = 1;
	int frame_id = 0;
	ci = 0;

	struct timeval tv; //the timer for timeout
	tv.tv_sec = 0;
    tv.tv_usec = 5000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval)); //set socket option -- timeout is 100000 microseconds

	fseek (fp , 0 , SEEK_END);
	lsize = ftell (fp);
	rewind (fp);
	//printf("The file length is %d bytes\n", (int)lsize);
	//printf("the packet length is %d bytes\n",DATALEN);

  // allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) exit (2);

  // copy the file into the buffer.
	fread (buf,1,lsize,fp);

  /*** the whole file is loaded in the buffer. ***/
	buf[lsize] ='\0'; //append the end byte (extra byte sent to server)
	gettimeofday(&sendt, NULL);		//get the current time
	
        
    while(ci<= lsize)
	{
		if ((lsize+1-ci) <= DATALEN)
			slen = lsize+1-ci;
		else 
			slen = DATALEN;
		memcpy(pack_send.data, (buf+ci), slen);


		do{
			pack_send.num = frame_id;
			pack_send.len = slen;
			n = sendto(sockfd, &pack_send, sizeof(pack_send), 0, addr, addrlen);
			printf("send packet No.%d\n",pack_send.num);

			n = recvfrom(sockfd, &ack_received, sizeof(ack_received), 0, addr, (socklen_t *)&addrlen);

   			if (n < 0 || ack_received.num != (frame_id + 1) || ack_received.len != 0) {
     			printf("error in transmission, packet will be retransmitted \n");
   			}else{
				   printf("receive ACK No.%d\n", ack_received.num);
			   }
		}while((n < 0) || (ack_received.num != (frame_id + 1)) || (ack_received.len != 0));
		
		ci += slen;	
		frame_id++;	
	}

	*len= ci;
   	printf("final total size %d bytes\n", (int)*len);
	printf("the packet length is %d bytes\n",DATALEN);

	
  
  // calculating time taken for transfer
	gettimeofday(&recvt, NULL);           //get current time
	tv_sub(&recvt, &sendt);               // get the whole trans time
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	printf("time_inv: %f\n",time_inv);
	return(time_inv);
}

void tv_sub(struct  timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}
