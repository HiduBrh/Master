#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#define CLIENT_PORT 50000
#define SERVER_REPLY_SIZE 1000000
#define WANT_COMMANDE "WANT "
#define NEED_COMMANDE "NEED "
#define NOSUCHFILE "NOSUCHFILE"
#define CLIENT_REQUEST_SIZE 210
#define BACKLOG_LENGTH 1000
#define BAD_COMMAND "BAD COMMAND"


int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c , read_size, pid;
    struct sockaddr_in server , client;
    char client_message[CLIENT_REQUEST_SIZE];

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(CLIENT_PORT);

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc, BACKLOG_LENGTH);
    while(1) {
        //Accept and incoming connection
        puts("Waiting for incoming connections...");
        c = sizeof(struct sockaddr_in);

        //accept connection from an incoming client
        client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t *) &c);
        if (client_sock < 0) {
            perror("accept failed");
            return 1;
        }
        puts("Connection accepted");
        //deleguer a un sous processus
        if((pid=fork())==0) {
            printf(" process %d \n",pid);
            //Receive a message from client
            while ((read_size = recv(client_sock, client_message, 2000, 0)) > 0) {
                puts(client_message);
                if(strstr(client_message,WANT_COMMANDE)) {
                    char criteres[CLIENT_REQUEST_SIZE-5];
                    strncpy(criteres, client_message+5, CLIENT_REQUEST_SIZE-5);
                    criteres[CLIENT_REQUEST_SIZE-5] = '\0';
                    char *token;
                    //puts(criteres);
                    int nbcriteres=1;
                    token=strtok(criteres,",");
                    char** tokens[3];
                    tokens[0]=token;
                    while(token!=NULL && nbcriteres<3){
                        token=strtok(NULL,",");
                        tokens[nbcriteres]=token;
                        nbcriteres++;
                    }
                    if(tokens[0]==NULL && tokens[1]==NULL && tokens[2]==NULL)
                        continue;
                    //Send the message back to client
                    write(client_sock, token, strlen(token));
                    memset(client_message, 0, sizeof(client_message));

                }else{
                    if(strstr(client_message,NEED_COMMANDE)) {
                        char id[CLIENT_REQUEST_SIZE-5];
                        strncpy(id, client_message+5, CLIENT_REQUEST_SIZE-5);
                        id[CLIENT_REQUEST_SIZE-5] = '\0';
                        //Send the message back to client
                        write(client_sock, id, strlen(id));
                        memset(client_message, 0, sizeof(client_message));
                    } else{
                        write(client_sock, BAD_COMMAND, strlen(BAD_COMMAND));
                    }
                }

            }

            if (read_size == -1) {
                perror("recv failed");
            }
            close(socket_desc);
            return 0;
        }
        if(pid!=0)
            printf(" service execute par le processus %d \n",pid);


    }
    return 0;
}
