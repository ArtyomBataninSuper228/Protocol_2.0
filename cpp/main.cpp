//
//  main.cpp
//  Protocol_UDP
//
//  Created by Артем Батанин on 07.05.2026.
//


#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#define PORT     8080
#define MAXLINE 1500

#ifndef MSG_CONFIRM
#define MSG_CONFIRM 0
#endif
  

#include <vector>
#include <string>
#include <cstdint>

#include "stuff.hpp"






// Driver code
int main() {
    int sockfd;
    unsigned char buffer[MAXLINE];
    const char *hello = "Hello from server";
    struct sockaddr_in servaddr, cliaddr;
      
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        std::cout<<"socket creation failed"<<std::endl;
        exit(EXIT_FAILURE);
    }
      
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
      
    // Filling server information
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
      
    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,
            sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
      
    socklen_t len;
    int n;
    uint32_t hash;
    bool ok;
    unsigned char bytes[4];
    len = sizeof(cliaddr);  //len is value/result
    while(true){
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                    &len);
        auto t_st = std::chrono::high_resolution_clock::now();
        buffer[n] = '\0';
        //printf("Client : %s\n", buffer);
        sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
        hash = crc32(buffer, 4, n);
        
        // Big-Endian
        bytes[3] = (hash >> 0) & 0xFF;
        bytes[2] = (hash >> 8) & 0xFF;
        bytes[1] = (hash >> 16) & 0xFF;
        bytes[0] = (hash >> 24) & 0xFF;
        if (not  ((bytes[0] == buffer[0]) and (bytes[1] == buffer[1]) and (bytes[2] == buffer[2]) and (bytes[3] == buffer[3]))){
            continue;
        }
        auto t_end = std::chrono::high_resolution_clock::now();
        
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end-t_st).count();
        std::cout<<ns<<std::endl;
        //std::cout<<"SUCCESFUL HASH"<<std::endl;
        
        
        
          
        
        
        
        
    }
    return 0;
    
}
