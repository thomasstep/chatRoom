#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"

using namespace std;

/*
 * TODO: IMPLEMENT BELOW THREE FUNCTIONS
 */
int connect_to(const char *host, const int port);
struct Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const int port);

void* recv_msg_handler(void* cr_fd) {
    int chatroom_fd =  *((int *)cr_fd);
    char receiveMessage[201] = {};
    int one = 1;
    while (one != 0) {
        int receive = recv(chatroom_fd, receiveMessage, 201, 0);
        int message_len = strlen(receiveMessage);


        //printf("length: %d\n",message_len);
        //printf("AFTER\n");
        
        if (message_len > 0) {
            if (strcmp(receiveMessage, "Warning: the chatroom is going to be closed...\n") == 0){
              printf("WE IN");
              one = 0;
            } 
            display_message(receiveMessage);
        } else if (message_len == 0) {
            continue;
        } else { 
            // -1 
        }
        //printf("one: %d\n", one);
    }
    printf("NULL");
    pthread_exit(NULL);
    return NULL;
}

void* send_msg_handler(void* cr_fd) {
    int chatroom_fd =  *((int *)cr_fd);
    char message[101] = {};
    while (1) {
        get_message(message, 101);

        // printf("\r%s", "> ");
        // fflush(stdout);

        send(chatroom_fd, message, 101, 0);
        if (strcmp(message, "exit") == 0) {
            break;
        }
    }
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char** argv)
{
  if (argc != 3) {
    fprintf(stderr,
        "usage: enter host address and port number\n");
    exit(1);
  }

    display_title();

  while (1) {

    int sockfd = connect_to(argv[1], atoi(argv[2]));

    char command[MAX_DATA];
    get_command(command, MAX_DATA);

    struct Reply reply = process_command(sockfd, command);
    display_reply(command, reply);

    touppercase(command, strlen(command) - 1);
    if (strncmp(command, "JOIN", 4) == 0 && reply.status == SUCCESS) {
      printf("Now you are in the chatmode (Press 'Q' to exit chatmode)\n");
      process_chatmode(argv[1], reply.port);
      printf("TITLE");
      display_title();
    }

    close(sockfd);
    }

    return 0;
}

/*
 * Connect to the server using given host and port information
 *
 * @parameter host    host address given by command line argument
 * @parameter port    port given by command line argument
 *
 * @return socket fildescriptor
 */
int connect_to(const char *host, const int port)
{
  // ------------------------------------------------------------
  // GUIDE :
  // In this function, you are suppose to connect to the server.
  // After connection is established, you are ready to send or
  // receive the message to/from the server.
  //
  // Finally, you should return the socket fildescriptor
  // so that other functions such as "process_command" can use it
  // ------------------------------------------------------------

  int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(clientSocket == -1) {
    printf("Could not create socket. Exiting.");
    exit(1);
  }

  // Setup sockaddr_in struct with server's info
  struct sockaddr_in serverInfo;
  int serverInfoLength = sizeof(serverInfo);
  memset(&serverInfo, 0, serverInfoLength);
  serverInfo.sin_family = AF_INET;
  serverInfo.sin_addr.s_addr = inet_addr(host);
  // inet_pton(AF_INET, "0.0.0.0", &serverInfo.sin_addr.s_addr);
  serverInfo.sin_port = htons(port);

  cout << "Client connecting to: " << inet_ntoa(serverInfo.sin_addr) << ":" << ntohs(serverInfo.sin_port) << endl;

  // Connect to server
  int connection = connect(clientSocket, (struct sockaddr*)&serverInfo, serverInfoLength);
  if(connection == -1) {
    cout << "There was an error connecting to the server" << endl;
    exit(1);
  }
  return clientSocket;
}

/*
 * Send an input command to the server and return the result
 *
 * @parameter sockfd   socket file descriptor to commnunicate
 *                     with the server
 * @parameter command  command will be sent to the server
 *
 * @return    Reply
 */
struct Reply process_command(const int sockfd, char* command)
{
  // ------------------------------------------------------------
  // GUIDE 1:
  // In this function, you are supposed to parse a given command
  // and create your own message in order to communicate with
  // the server. Surely, you can use the input command without
  // any changes if your server understand it. The given command
    // will be one of the followings:
  //
  // CREATE <name>
  // DELETE <name>
  // JOIN <name>
    // LIST
  //
  // -  "<name>" is a chatroom name that you want to create, delete,
  // or join.
  //
  // - CREATE/DELETE/JOIN and "<name>" are separated by one space.
  // ------------------------------------------------------------
    char delim[] = " ";
    int inputLength = strlen(command);
    char *temp = (char*) calloc(inputLength + 1, sizeof(char));
    strncpy (temp,command, inputLength);
    char* ptr = strtok(temp, delim);
    char buf[1000];
    int numbytes = 0;
    int received = 0;
    memset(buf, 0, 1000);

    if(strcmp(ptr, "CREATE") == 0){
      send(sockfd, command, inputLength, 0);
      numbytes = recv(sockfd, buf, 999, 0);
      //printf("%s\n", buf);

      struct Reply reply; 
      received = atoi(buf);
      if(received == 0){
        reply.status = SUCCESS;
      } 
      else {
        reply.status = FAILURE_ALREADY_EXISTS;
      }
      return reply;
    } 
    else if(strcmp(ptr, "DELETE") == 0){
      send(sockfd, command, inputLength, 0);
      numbytes = recv(sockfd, buf, 999, 0);

      struct Reply reply; 
      received = atoi(buf);
      //printf("%d\n", received);

      if(received == 0){
        reply.status = SUCCESS;
      } 
      else {
        reply.status = FAILURE_NOT_EXISTS;
      }

      return reply;
    } 
    else if(strcmp(ptr, "JOIN") == 0){
      send(sockfd, command, inputLength, 0);
      numbytes = recv(sockfd, buf, 999, 0);
      //printf("%s\n", buf);

      struct Reply reply;
      received = atoi(buf);
      if(received != 2){
        int buflength = strlen(buf);
        char *copy_buf = (char*) calloc(inputLength + 1, sizeof(char));
        strncpy (copy_buf, buf, buflength);
        char* token = strtok(copy_buf, delim);
        int port_num = atoi(token);
        token = strtok(NULL, delim);
        int member_num = atoi(token);
        reply.status = SUCCESS;
        reply.num_member = member_num; 
        reply.port = port_num; 
      } 
      else {
        reply.status = FAILURE_NOT_EXISTS;
      }
      
      return reply;
    } 
    else if(strcmp(ptr, "LIST") == 0){
      send(sockfd, command, inputLength, 0);
      numbytes = recv(sockfd, buf, 999, 0);
      //printf("%s\n", buf);

      struct Reply reply; 
      
      strcpy(reply.list_room, buf);
      reply.status = SUCCESS;
      return reply;
    } 
    else{
      struct Reply reply;
      reply.status = FAILURE_INVALID;
    }


  // ------------------------------------------------------------
  // GUIDE 2:
  // After you create the message, you need to send it to the
  // server and receive a result from the server.
  // ------------------------------------------------------------


  // ------------------------------------------------------------
  // GUIDE 3:
  // Then, you should create a variable of Reply structure
  // provided by the interface and initialize it according to
  // the result.
  //
  // For example, if a given command is "JOIN room1"
  // and the server successfully created the chatroom,
  // the server will reply a message including information about
  // success/failure, the number of members and port number.
  // By using this information, you should set the Reply variable.
  // the variable will be set as following:
  //
  // Reply reply;
  // reply.status = SUCCESS;
  // reply.num_member = number;
  // reply.port = port;
  //
  // "number" and "port" variables are just an integer variable
  // and can be initialized using the message fomr the server.
  //
  // For another example, if a given command is "CREATE room1"
  // and the server failed to create the chatroom becuase it
  // already exists, the Reply varible will be set as following:
  //
  // Reply reply;
  // reply.status = FAILURE_ALREADY_EXISTS;
    //
    // For the "LIST" command,
    // You are suppose to copy the list of chatroom to the list_room
    // variable. Each room name should be seperated by comma ','.
    // For example, if given command is "LIST", the Reply variable
    // will be set as following.
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // strcpy(reply.list_room, list);
    //
    // "list" is a string that contains a list of chat rooms such
    // as "r1,r2,r3,"
  // ------------------------------------------------------------

  // REMOVE below code and write your own Reply.
}

/*
 * Get into the chat mode
 *
 * @parameter host     host address
 * @parameter port     port
 */
void process_chatmode(const char* host, const int port)
{
  // ------------------------------------------------------------
  // GUIDE 1:
  // In order to join the chatroom, you are supposed to connect
  // to the server using host and port.
  // You may re-use the function "connect_to".
  // ------------------------------------------------------------
  int chatroom_fd = connect_to(host, port);
  // ------------------------------------------------------------
  // GUIDE 2:
  // Once the client have been connected to the server, we need
  // to get a message from the user and send it to server.
  // At the same time, the client should wait for a message from
  // the server.
  // ------------------------------------------------------------
  pthread_t send_msg_thread;
  pthread_t recv_msg_thread;

  while(1){
    //printf("BEFORE JOIN\n");
  
    (void) pthread_join(send_msg_thread, NULL);
    (void) pthread_join(recv_msg_thread, NULL);
    
    //printf("BEFORE SEND\n");
    if (pthread_create(&send_msg_thread, NULL, &send_msg_handler, (void*) &chatroom_fd) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    //printf("BEFORE RECEIVE\n");
    if (pthread_create(&recv_msg_thread, NULL, &recv_msg_handler, (void*) &chatroom_fd) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }
    printf("DONE");
    pthread_exit(NULL);


  }
  // ------------------------------------------------------------
  // IMPORTANT NOTICE:
  // 1. To get a message from a user, you should use a function
  // "void get_message(char*, int);" in the interface.h file
  //
  // 2. To print the messages from other members, you should use
  // the function "void display_message(char*)" in the interface.h
  //
  // 3. Once a user entered to one of chatrooms, there is no way
  //    to command mode where the user  enter other commands
  //    such as CREATE,DELETE,LIST.
  //    Don't have to worry about this situation, and you can
  //    terminate the client program by pressing CTRL-C (SIGINT)
  // ------------------------------------------------------------
}