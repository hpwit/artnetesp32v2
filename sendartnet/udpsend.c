#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <time.h>
    
#define PORT     6454 
#define MAXLINE 1024 
#define BUFFER_SIZE 530
    void delay(clock_t start_time,int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds =  number_of_seconds;
 
    // Storing start time
   // clock_t start_time = clock();
 
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds)
        ;
}
 
// Driver code 
int main(int argc, char *argv[]) { 

 char addr[200];
    int delayu;
            sscanf(argv[1],"%d",&delayu);
        int fps;
            sscanf(argv[2],"%d",&fps); 
        int total_universes;
            sscanf(argv[3],"%d",&total_universes);
        sscanf(argv[4],"%s",addr);
    float timefps=(float)(1000000/fps);
    printf("time between two frames %lf us\n",timefps);
    if(delayu*total_universes>timefps)
    {
        printf("diminish delay tfor frame rate\n");
        return 0;
    }
    int delarest=(int) (timefps-delayu*total_universes);
    printf("entre deux last packet and new frame %d us\n",delarest);
    int sockfd; 
    char buffer[MAXLINE]; 
    char senfbuffer[BUFFER_SIZE];
   
    struct sockaddr_in servaddr, cliaddr; 
        
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        //perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
        
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
        
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 


       cliaddr.sin_family    = AF_INET; // IPv4 
    cliaddr.sin_addr.s_addr = inet_addr(addr);
    cliaddr.sin_port = htons(PORT); 
        
    // Bind the socket with the server address 
  if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        printf("bind failed\n"); 
        exit(EXIT_FAILURE); 
    } 
        
    int len, n; 
    
    len = sizeof(cliaddr);  //len is value/result 
   
    while(1)
    {
         clock_t start_frame=clock();
        for(uint8_t i=0;i<total_universes;i++)
        {
    senfbuffer[14]=i;    
    clock_t start_packet=clock();
    sendto(sockfd, (const char *)senfbuffer, BUFFER_SIZE,  
        0, (const struct sockaddr *) &cliaddr, 
            len); 
            
        delay(start_packet,delayu);
    }
    delay(start_frame,timefps);
    }
    printf("Hello message sent.\n");  
        
    return 0; 
}