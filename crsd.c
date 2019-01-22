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
Pairs contain the port and threadID as the first index and a vector of sockets as the second index.
PS I know this looks terrifying.
*/
std::map<std::string, std::pair<std::pair<int, bool*>, std::vector<int>*>>* chatrooms = new std::map<std::string, std::pair<std::pair<int, bool*>, std::vector<int>*>>;

// This function just splits words in a string into vector entries by space delimiter
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

void sendToAll(int clientSocket, std::string chatroomName, char* message) {
  std::vector<int>* clients;
  std::string messageString = "";
  messageString = message;
  std::cout << "Sending message: " << messageString << " to chatroom " << chatroomName << std::endl;
  for(auto i = chatrooms->begin(); i != chatrooms->end(); ++i) {
    if(i->first == chatroomName) {
      if(!*(i->second.first.second)) {
        return;
      }
      clients = i->second.second;
    }
  }
  for(auto i = clients->begin(); i != clients->end(); ++i) {
    if(*i != clientSocket) {
      send(*i, message, MAX_DATA, 0);
    }
  }
  return;
}

// Finds the chatroom specified and closes all sockets connected to it
void disconnectAll(std::string chatroomName) {
  std::vector<int>* clients;
  for(auto i = chatrooms->begin(); i != chatrooms->end(); ++i) {
    if(i->first == chatroomName) {
      clients = i->second.second;
    }
  }
  for(auto i = clients->begin(); i != clients->end(); ++i) {
    close(*i);
  }
}

void leaveChatroom(int clientSocket, std::string chatroomName) {
  std::vector<int>* clients;
  for(auto i = chatrooms->begin(); i != chatrooms->end(); ++i) {
    if(i->first == chatroomName) {
      clients = i->second.second;
    }
  }
  auto toErase = clients->begin();
  for(auto i = clients->begin(); i != clients->end(); ++i) {
    if(*i == clientSocket) {
      toErase = i;
    }
  }
  clients->erase(toErase);
  return;
}

void* chatReceiver(void* cCI) {
  std::pair<int, std::string>* clientChatInfo = (std::pair<int, std::string>*) cCI;
  int clientSocket = (*clientChatInfo).first;
  int received = 0;
  char message[MAX_DATA];
  std::string messageString = "";
  while(1) {
    memset(message, 0, MAX_DATA);
    received = recv(clientSocket, message, MAX_DATA, 0);
    if(received > 0) {
      messageString = message;
      if(messageString.substr(0,4) == "exit") {
        std::cout << clientSocket << " is leaving." << std::endl;
        leaveChatroom(clientSocket, (*clientChatInfo).second);
        return NULL;
      }
      else {
        std::cout << "Received message: " << messageString << " from " << clientSocket << std::endl;
        sendToAll(clientSocket, (*clientChatInfo).second, message);
      }
    }
  }
}

void* chatroomHandler(void* cE) {
  std::pair<std::string, std::pair<std::pair<int, bool*>, std::vector<int>*>>* chatroomEntry = (std::pair<std::string, std::pair<std::pair<int, bool*>, std::vector<int>*>>*) cE;
  // Create a receiver thread and a sender thread
  int port = (*chatroomEntry).second.first.first;
  // Create server socket
  int chatroomSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(chatroomSocket == -1) {
    printf("Could not create socket. Exiting.");
    exit(1);
  }

  // Setup sockaddr_in struct with chatrooms's info
  struct sockaddr_in chatroomInfo;
  int chatroomInfoLength = sizeof(chatroomInfo);
  memset(&chatroomInfo, 0, chatroomInfoLength);
  chatroomInfo.sin_family = AF_INET;
  chatroomInfo.sin_addr.s_addr = INADDR_ANY;
  chatroomInfo.sin_port = htons(port);

  // Bind chatroom and listen
  int bindReturn = bind(chatroomSocket, (struct sockaddr *) &chatroomInfo, (socklen_t) chatroomInfoLength);
  if(bindReturn == -1) {
    std::cout << "Could not bind" << std::endl;
    exit(1);
  }
  if(listen(chatroomSocket, 10) == -1) {
    close(chatroomSocket);
    std::cout << "Could not listen" << std::endl;
    exit(1);
  }
  std::cout << "Started chatroom on: " << inet_ntoa(chatroomInfo.sin_addr) << ":" << ntohs(chatroomInfo.sin_port) << std::endl;

  // Initialize client's sockaddr_in
  struct sockaddr_in clientInfo;
  int clientInfoLength = sizeof(clientInfo);
  memset(&clientInfo, 0, clientInfoLength);
  int clientSocket = 0;

  while(*((*chatroomEntry).second.first.second)) {
    // Accept new client's connection and print it for log
    clientSocket = accept(chatroomSocket, (struct sockaddr*) &clientInfo, (socklen_t*) &clientInfoLength);
    (*chatroomEntry).second.second->push_back(clientSocket);
    if(!*((*chatroomEntry).second.first.second)) {
      break;
    }
    printf("Client %s:%d connected.\n", inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));

    pthread_t clientThread;
    std::pair<int, std::string> clientChatInfo(clientSocket, (*chatroomEntry).first);
    if(pthread_create(&clientThread, NULL, &chatReceiver, (void*) &clientChatInfo) != 0) {
      printf("Thread couldn't be created");
      break;
    }
  }
  std::cout << "Deleting chatroom" << std::endl;
  disconnectAll((*chatroomEntry).first);
  close(chatroomSocket);
  pthread_exit(NULL);
}

void* clientReceiver(void* cS) {
  int clientSocket = *(int*) cS;
  int received = 0;
  char message[MAX_DATA];
  std::string messageString = "";
  std::string toSend = "";
  while(1) {
    memset(message, 0, MAX_DATA);
    received = recv(clientSocket, message, MAX_DATA, 0);
    if(received > 0) {
      messageString = message;
      // Logging message and who sent it
      std::cout << "Message from " << clientSocket << ": " << messageString;
      std::vector<std::string> messageVect = splitOnSpace(messageString);
      if(messageVect[0] == "LIST") {
        // List chat rooms
        toSend = "";
        // Loop through each chatroom and add its name to the string sent to client
        for(auto i = chatrooms->begin(); i != chatrooms->end(); ++i) {
          if(i != chatrooms->begin()) {
            toSend += ", ";
          }
          toSend += i->first;
        }
        toSend += "\n";
        send(clientSocket, toSend.c_str(), toSend.length(), 0);
      }
      else if(messageVect[0] == "CREATE") {
        // Create chatroom and spin up new thread for that room
        // Start from port 8889 and find the closest unused port
        int port = 8889;
        bool taken = false;
        // Loop through existing chatrooms; choose port that isn't taken
        while(port < 9145 && chatrooms->begin() != chatrooms->end()) {
          for(auto i = chatrooms->begin(); i != chatrooms->end(); ++i) {
            if(i->second.first.first == port) {
              taken = true;
            }
            else {
              taken = false;
            }
          }
          if(taken) {
            port++;
          }
          else {
            break;
          }
        }
        // Creating the necessary components for a chatroom and adding entry to map
        std::vector<int>* emptyVect = new std::vector<int>();
        pthread_t chatroomThread;
        bool keepRunning = true;
        std::pair<int, bool*> ids(port, &keepRunning);
        std::pair<std::pair<int, bool*>, std::vector<int>*> second(ids, emptyVect);
        std::pair<std::string, std::pair<std::pair<int, bool*>, std::vector<int>*>> chatroomEntry(messageVect[1], second);
        chatrooms->insert(chatroomEntry);
        toSend = "Creating chat room \'" + messageVect[1] + "\' on port " + std::to_string(port) + ".\n";
        pthread_create(&chatroomThread, NULL, &chatroomHandler, (void*) &chatroomEntry);
        send(clientSocket, toSend.c_str(), toSend.length(), 0);
      }
      else if(messageVect[0] == "DELETE") {
        // Delete chatroom and disconnect everyone
        toSend = "Deleting chat room " + messageVect[1] + ".\n";
        auto toErase = chatrooms->end();
        for(auto i = chatrooms->begin(); i != chatrooms->end(); ++i) {
          if(i->first == messageVect[1]) {
            *(i->second.first.second) = false;
            toErase = i;
          }
        }
        if(toErase != chatrooms->end()) {
          chatrooms->erase(toErase);
        }
        else {
          toSend = "Chatroom does not exist.\n";
        }
        send(clientSocket, toSend.c_str(), toSend.length(), 0);
      }
      else if(messageVect[0] == "JOIN") {
        // Join a chatroom
        int port = 0;
        std::vector<int>* chatroom;
        for(auto i = chatrooms->begin(); i != chatrooms->end(); ++i) {
          if(i->first == messageVect[1]) {
            port = i->second.first.first;
            chatroom = i->second.second;
          }
        }
        if(port == 0) {
          toSend = "That chatroom does not exist.";
        }
        else {
          toSend = "Chat room " + messageVect[1] + " is on port " + std::to_string(port) + ".\n";
          toSend = toSend + "There are currently " + std::to_string(chatroom->size()) + " others in the chatroom.\n";
        }
        send(clientSocket, toSend.c_str(), toSend.length(), 0);
      }
      else {
        toSend = "Could not interpret command.\n";
        send(clientSocket, toSend.c_str(), toSend.length(), 0);
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
