#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"

using namespace std;

int serverSocket = 0;
int clientSocket = 0;

int main() {
  // Create server socket
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(serverSocket == -1) {
    printf("Could not create socket. Exiting.");
    exit(1);
  }

  // Setup sockaddr_in struct with server's info
  struct sockaddr_in serverInfo;
  serverInfo.sin_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", &serverInfo.sin_addr);
  serverInfo.sin_port = htons(8888);

  // Bind server and listen
  bind(serverSocket, (struct sockaddr*) &serverInfo, sizeof(serverInfo));
  listen(serverSocket, 5);
  printf("Started server on: %s:%d\n", inet_ntoa(serverInfo.sin_addr), ntohs(serverInfo.sin_port));

  // Initialize client's sockaddr_in
  struct sockaddr_in clientInfo;
  int received = 0;
  char message[MAX_DATA];
  while(true) {
    // Accept new client's connection and print it for log
    clientSocket = accept(serverSocket, (struct sockaddr*) &clientInfo, (socklen_t*)sizeof(clientInfo));
    printf("Client %s:%d connected.\n", inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));
    while(true) {
      recv(clientSocket, message, MAX_DATA, 0);
      if(received > 0) {
        printf(message);
      }
      cout << "Waiting" << endl;
    }
    cout << "Done waiting" << endl;
    close(clientSocket);
  }

  return 0;
}
