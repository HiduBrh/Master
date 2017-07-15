#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <stdlib.h>
#define CLIENT_PORT 50000
#define SLAVE_AUTH_PORT 50001
#define SLAVE_AUTH_SIZE 20
#define SERVER_REPLY_SIZE 1000000
#define WANT_COMMANDE "WANT "
#define NEED_COMMANDE "NEED "
#define OK "ok "
#define BAD "BAD "
#define NOSUCHFILE "NOSUCHFILE"
#define CLIENT_REQUEST_SIZE 210
#define BACKLOG_LENGTH 1000
#define BAD_COMMAND "BAD COMMAND"
#define SLAVES_MAX 2

int slaves_sockets[SLAVES_MAX];
char *slaves_indexes[SLAVES_MAX];
int next_slave_id=0;
int main(int argc , char *argv[])
{
    int socket_desc , client_sock,slave_auth_socket , c , read_size, pid;
    struct sockaddr_in server , client, slave_auth;
    char client_message[CLIENT_REQUEST_SIZE];

    //Creation des sockets
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    slave_auth_socket = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1 || slave_auth_socket==-1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(CLIENT_PORT);
    slave_auth.sin_family = AF_INET;
    slave_auth.sin_addr.s_addr = INADDR_ANY;
    slave_auth.sin_port = htons(SLAVE_AUTH_PORT);

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    if( bind(slave_auth_socket,(struct sockaddr *)&slave_auth , sizeof(slave_auth)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc, BACKLOG_LENGTH); //ecoute des requetes client sur le port 50000
    listen(slave_auth_socket, BACKLOG_LENGTH); //ecoute des requetes de demande d'authentification des slaves sur le port 50001
//accepter 5 slaves
    while(1 && next_slave_id<SLAVES_MAX) {
        //Accept and incoming connection
        puts("Waiting for incoming connections...");
        c = sizeof(struct sockaddr_in);

        //accept connection from an incoming client
        client_sock = accept(slave_auth_socket, (struct sockaddr *) &client, (socklen_t *) &c);
        if (client_sock < 0) {
            perror("accept failed");
            return 1;
        }
        puts("Connection accepted");
        //Receive a message from slave
        while ((read_size = recv(client_sock, client_message, CLIENT_REQUEST_SIZE, 0)) > 0) {
            puts(client_message);
            //recuperation du port d'coute du slave pour les prochaines communications
            char port[5];
            strncpy(port, client_message+6, 5);
            port[5] = '\0';
            puts(port);
            int numPort=atoi(port);
            //creating socket (communicate with slave)
            int sock_tmp;
            struct sockaddr_in server_tmp;
            sock_tmp = socket(AF_INET , SOCK_STREAM , 0);
            if (sock_tmp == -1)
            {
                printf("Could not create socket");
            }
            puts("Socket created");
            server_tmp.sin_addr.s_addr = inet_addr("127.0.0.1");
            server_tmp.sin_family = AF_INET;
            server_tmp.sin_port = htons(numPort);
            if (connect(sock_tmp , (struct sockaddr *)&server_tmp , sizeof(server_tmp)) < 0)
            {
                perror("connect failed. Error");
                write(slave_auth_socket, BAD, strlen(BAD));
                continue;
            }
            puts("Connected\n");
            //ajout de la socket a liste des sockets de communication avec les slaves
            slaves_sockets[next_slave_id]=sock_tmp;
            //envoie de la reponse au slave
            write(client_sock, OK, strlen(OK));
            // simulation de la liste des indexs

            if(next_slave_id==0){
                char * index[4]={"film","action","recent","0000000001"};
                slaves_indexes[next_slave_id]=index;
            }
            if(next_slave_id==1){
                char * index[4]={"code","reseau","tp","0000000002"};
                slaves_indexes[next_slave_id]=index;
            }
            //fin de la simulation

            next_slave_id++;
        }

    }

    while(1) {
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
                    //recuperation des criteres
                    char criteres[CLIENT_REQUEST_SIZE-5];
                    strncpy(criteres, client_message+5, CLIENT_REQUEST_SIZE-5);
                    criteres[CLIENT_REQUEST_SIZE-5] = '\0';
                    char *token;
                    int nbcriteres=1;
                    token=strtok(criteres,",");
                    char** tokens[3];
                    tokens[0]=token;
                    while(token!=NULL && nbcriteres<3){
                        token=strtok(NULL,",");
                        tokens[nbcriteres]=token;
                        nbcriteres++;
                    }
                    //si aucun criteres valide,redemander la saisie de nouveaux criteres
                    if(tokens[0]==NULL && tokens[1]==NULL && tokens[2]==NULL)
                        continue;
                    //recherche des fichiers dans le cache
                    char indexes_result[1000]="FILES ";
                    for(int i=0;i<SLAVES_MAX;i++){
                        if(strcmp(tokens[0],slaves_indexes[i][0])==0 && strcmp(tokens[1],slaves_indexes[i][1])==0 && strcmp(tokens[2],slaves_indexes[i][2])==0) {
                            strcat(indexes_result, slaves_indexes[i][3]);
                            strcat(indexes_result," ");
                        }
                    }
                    //renvoie de la reponse au client
                    write(client_sock, indexes_result, strlen(token));
                    memset(client_message, 0, sizeof(client_message));

                }else{
                    if(strstr(client_message,NEED_COMMANDE)) {
                        char id[CLIENT_REQUEST_SIZE-5];
                        strncpy(id, client_message+5, CLIENT_REQUEST_SIZE-5);
                        id[CLIENT_REQUEST_SIZE-5] = '\0';
                        /*
                         * to do :demande du fichier au slave
                         */
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
