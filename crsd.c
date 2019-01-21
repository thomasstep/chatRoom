#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include "interface.h"

int serverSocket = 0;
/*
Keys are the names of chatrooms.
Pairs contain the port as the first index and a vector of sockets as the second index.
*/
std::map<std::string, std::pair<int, std::vector<int>>> chatrooms;

std::vector<std::string> splitOnSpace(std::string input) {
    std::vector<std::string> splitInput;
    std::string param = "";
    for (int i = 0; i < input.length(); i++) {
        if (!isspace(input[i])) {
            param += input[i];
        } else {
            splitInput.push_back(param);
            param = "";
        }
    }
    // Get the last parameter
    splitInput.push_back(param);
    return splitInput;
}

void* clientReceiver(void* cS) {
  int* clientSocket = (int*) cS;
  int received = 0;
  char message[MAX_DATA];
  std::string messageString = "";
  std::string toSend = "";
  while(1) {
    received = recv(*clientSocket, message, MAX_DATA, 0);
    if(received > 0) {
      messageString = message;
      std::cout << "Message from " << *clientSocket << ": " << messageString;
      std::vector<std::string> messageVect = splitOnSpace(messageString);
      if(messageVect[0] == "LIST") {
        // List chat rooms
        toSend = "";
        for(auto i = chatrooms.begin(); i != chatrooms.end(); ++i) {
          if(i != chatrooms.begin()) {
            toSend += ", ";
          }
          toSend += i->first;
        }
        toSend += "\n"
        send(*clientSocket, toSend.c_str(), toSend.length(), 0);
      }
      else if(messageVect[0] == "CREATE") {
        // Create chatroom and spin up new thread for that room
        chatrooms[messageVect[1]];
        toSend = "Creating chat room " + messageVect[1] + ".\n";
        send(*clientSocket, toSend.c_str(), toSend.length(), 0);
      }
      else if(messageVect[0] == "DELETE") {
        // Delete chatroom and disconnect everyone
        toSend = "Deleting chat room " + messageVect[1] + ".\n";
        send(*clientSocket, toSend.c_str(), toSend.length(), 0);
      }
      else if(messageVect[0] == "JOIN") {
        // Join a chatroom
        toSend = "Connecting you to chat room " + messageVect[1] + ".\n";
        send(*clientSocket, toSend.c_str(), toSend.length(), 0);
      }
      else {
        toSend = "Could not interpret command.\n";
        send(*clientSocket, toSend.c_str(), toSend.length(), 0);
      }
    }
  }
  pthread_exit(NULL);
}

int main() {
  // Create server socket
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(serverSocket == -1) {
    printf("Could not create socket. Exiting.");
    exit(1);
  }

  // Setup sockaddr_in struct with server's info
  struct sockaddr_in serverInfo;
  int serverInfoLength = sizeof(serverInfo);
  memset(&serverInfo, 0, serverInfoLength);
  serverInfo.sin_family = AF_INET;
  serverInfo.sin_addr.s_addr = INADDR_ANY;
  // inet_pton(AF_INET, "127.0.0.1", &serverInfo.sin_addr.s_addr);
  serverInfo.sin_port = htons(8888);

  // Bind server and listen
  int bindReturn = bind(serverSocket, (struct sockaddr *) &serverInfo, (socklen_t) serverInfoLength);
  if(bindReturn == -1) {
    std::cout << "Could not bind" << std::endl;
    exit(1);
  }
  if(listen(serverSocket, 10) == -1) {
    close(serverSocket);
    std::cout << "Could not listen" << std::endl;
    exit(1);
  }
  std::cout << "Started server on: " << inet_ntoa(serverInfo.sin_addr) << ":" << ntohs(serverInfo.sin_port) << std::endl;

  // Initialize client's sockaddr_in
  struct sockaddr_in clientInfo;
  int clientInfoLength = sizeof(clientInfo);
  memset(&clientInfo, 0, clientInfoLength);
  int clientSocket = 0;

  while(1) {
    // Accept new client's connection and print it for log
    clientSocket = accept(serverSocket, (struct sockaddr*) &clientInfo, (socklen_t*) &clientInfoLength);
    printf("Client %s:%d connected.\n", inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));

    pthread_t clientThread;
    if(pthread_create(&clientThread, NULL, &clientReceiver, (void*) &clientSocket) != 0) {
      printf("Thread couldn't be created");
      break;
    }
  }

  return 0;
}
