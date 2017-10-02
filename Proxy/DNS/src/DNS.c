/*
 ============================================================================
 Name        : DNS.c
 Author      : Andrii Polianytsia
 Version     :
 Copyright   : Free to use
 Description : Simple DNS Proxy Server
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define START 12 // start index of Domain Address in UDP package

char * dnsServer; // Upper DNS server
char * errorMsg; // Error message for Domain Names in Black List
char blackList[100][100]; // Black List of Domain Names
char buff[512]; // Request message buffer
char retBuf[512]; // Buffer for DNS response

const size_t bufflen = sizeof(buff); // Size of buffers

FILE * config; // Config file descriptor
int readConfigFile();
int extractDomainName(char * , int, int);
int compare(char * , int);
int sendToDNS(char * , int);

int main(void) {

  struct sockaddr_in addr;
  struct sockaddr_in src_addr;
  int blackListCounter;
  blackListCounter = readConfigFile();
  int recSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (recSock < 0) {
    printf("Failed to create listening socket");
    exit(1);
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(53);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(recSock, (struct sockaddr * ) & addr, sizeof(addr)) != 0) {
    printf("\n Error : Connect Failed \n");
    return 1;
  }
  listen(recSock, 100);
  int connfd = accept(recSock, (struct sockaddr * ) NULL, NULL);
  while (1) {
    int src_addr_len = sizeof(src_addr);
    ssize_t count = recvfrom(recSock, buff, bufflen, 0, (struct sockaddr * ) & src_addr, & src_addr_len);
    if (count == -1) {
      printf("%s", "Error with recvfrom function \n");
    } else if (count == sizeof(buff)) {
      warn("datagram too large for buffer: truncated");
    } else {
      char * pChar;
      buff[count] = '\0';
      pChar = (char * ) buff;
      int result = extractDomainName(buff, blackListCounter, count);
      if (result == 0) {
        sendto(recSock, retBuf, bufflen, 0, (struct sockaddr * ) & src_addr, sizeof(src_addr));
      } else {
        sendto(recSock, errorMsg, bufflen, 0, (struct sockaddr * ) & src_addr, sizeof(src_addr));
      }
    }
  }
  free(blackList);
  free(buff);
  free(retBuf);
  free(dnsServer);
  free(errorMsg);
  return EXIT_SUCCESS;
}

// Sending request to DNS server
int sendToDNS(char sendBuff[], int dnsCounter) {
    struct sockaddr_in dest;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = inet_addr(dnsServer); //dns servers
    int n = sendto(s, sendBuff, dnsCounter, 0, (struct sockaddr * ) & dest, sizeof(dest));

    int dest_len = sizeof(dest);
    int r;
    if ((r = recvfrom(s, retBuf, 512, 0, (struct sockaddr * ) & dest, & dest_len)) < 0) {
      printf("Error receiving message\n");
    }
    return 0;
  }
  //Function to compare Domain Name and Black List string
int compare(char ipAddr[], int BlackList) {
  int i;
  for (i = 0; i < BlackList; i++) {
    int result = strcmp(ipAddr, blackList[i]);
    if (result == 0) {
      return 0;
    } else {
      return 1;
    }
  }
}
int extractDomainName(char buff[], int BlackList, int messageSize) {
    int amount; // the amount symbols to read
    int positionBuff; // position in Buffer
    int positionString; // position in New String
    int dnsCounter = 0; // The amount of symbols from the START point to the end of Domain name in packet
    int count = START;
    int result; // Resulting value
    while (buff[count] != 0) {
      count++;
      dnsCounter++;
    }
    char requestName[100]; // String to carry domain address
    amount = buff[START];
    strncpy(requestName, & buff[START + 1], dnsCounter);
    positionBuff = START + amount + 1 + 1;
    positionString = amount + 1;
    amount = amount + requestName[positionString] + 1;
    requestName[positionString - 1] = '.'; // Placing dots where they should be in actual domain name;
    if (amount < (dnsCounter - 1)) // One extra symbol for NULL
    {
      positionString = amount + 1;
      requestName[positionString] = '.';
    }
    requestName[dnsCounter] = 0;
    result = compare(requestName, BlackList);
    if (result == 0) {
      return 1;
    } else {
      sendToDNS(buff, messageSize);
      return 0;
    }
  }
  //Function to read configuration file
int readConfigFile() {
  config = fopen("proxy.conf", "r+");
  if (config == NULL) {
    fprintf(stderr, "Config file error\n");
    exit(1);
  }
  size_t buffLen = 100;
  // reading upper DNS server address
  size_t length = 16;
  dnsServer = (char * ) malloc(16);
  getline( & dnsServer, & length, config);
  //reding error message
  size_t errlength = 20;
  errorMsg = (char * ) malloc(20);
  getline( & errorMsg, & errlength, config);
  //reading blacklist
  char * tempString = (char * ) malloc(100);
  int counter = 0;
  int capacity = 0;
  while ((capacity = getline( & tempString, & buffLen, config)) > 0) {
    strncpy(blackList[counter], tempString, capacity);
    blackList[counter][strcspn(blackList[counter], "\r\n")] = 0;
    counter++;
  }
  free(tempString);
  fclose(config);
  return counter;

}
