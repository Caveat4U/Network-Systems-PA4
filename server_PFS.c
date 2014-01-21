#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> /* select() */
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>   /* memset() */
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "common_helper.h"
 
#define USAGE "./server_PFS <Port Number>" // <Private Key of Server> <Certificate of Server> <CA Cert>"
 
// Accept/Reject client names
// Register clients
// Deregister clients
// Add files to master file list
// Remove files from master file list
// Distribute master file list to all registered clients
// (Exit can be done ctrl+c)

struct file_info master_file_list[MAX_NUM_FILES];

int append_master_list(struct file_info tmp_file)
{
    int i;
    for(i=0; i < MAX_NUM_FILES; i++)
    {
        if(tmp_file.port_number == 0)
        {
            //there are no more files 
            return EXIT_FAILURE;
        }
        if(master_file_list[i].port_number == 0)
        {
            //this is an empty slot
            master_file_list[i] = tmp_file;
            return EXIT_SUCCESS; 
        }
    }
    return EXIT_SUCCESS;
}

// Should be called on disconnect.
int deregister_client(int port_number)
{
    if(port_number == 0)
    {
        return EXIT_FAILURE;
    }
    struct file_info empty_file; 
    empty_file.port_number = 0; 
    int i;
    for(i = 0; i < MAX_NUM_FILES; i++)
    {
        if(master_file_list[i].port_number == port_number)
        {
            //remove it
            master_file_list[i] = empty_file;
            //bzero(master_file_list[i], sizeof(master_file_list[i])); 
        }
    }
    // If a disconnect is detected, parse through master_file_list
    // If entry matches location, remove it from master_file_list
    // Spam out the new list
    return EXIT_SUCCESS;
}
 
int main(int argc, char *argv[]) {
    if (argc != 2)
    {
        fprintf(stderr, "Incorrect number of arguments. Usage should be:\n %s\n", USAGE);
        return EXIT_FAILURE;
    }
    struct sockaddr_in local_addr, remote_addr;
    int sd; //file descriptor
    char file_name[MAX_STRING_LENGTH];
    int nbytes; 
    // Clear socket buffers
    bzero(&local_addr, sizeof(local_addr));
    bzero(&remote_addr, sizeof(remote_addr));
    bzero(&master_file_list, sizeof(master_file_list));
     
    // htons() sets the port # to network byte order
    // Setup for local
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    local_addr.sin_port = htons(atoi(argv[1]));
    // Setup for remote 
    //remote_addr.sin_family = AF_INET;
    //remote_addr.sin_addr.s_addr = INADDR_ANY;
    //remote_addr.sin_port = htons(router.links[i].dest_tcp_port);
     
     
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "error opening socket\n");
        return EXIT_FAILURE;
    }
         
    if(bind(sd, (struct sockaddr *)&local_addr, sizeof(local_addr))<0)
    {
        fprintf(stderr, "%s: cannot to bind port number %d \n",argv[0], atoi(argv[1]));
        return EXIT_FAILURE;
    }
    //non blocking socket conversion    
    if(fcntl(sd, F_SETFL, O_NDELAY) < 0)
    {
        fprintf(stderr, "Could not un-block socket.\n");
    }
     
    /* Listen */
    if (listen(sd, 20) == -1)
    {
        fprintf(stderr, "Listen Error\n");
        return EXIT_FAILURE;
    }
            
    struct socket client_sockets[MAX_CLIENTS];
    int i, j;
    /*todo remove ?
    for(i =0; i < MAX_CLIENTS; i++)
    {
        if((client_sockets[i].sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        {
            fprintf(stderr, "error opening socket\n");
            return EXIT_FAILURE;
        }
        client_sockets[i].connected = 0;

    }*/
    fprintf(stdout, "Sleeping for 10 seconds while awaiting an initial connection.\n");
    sleep(10);
    int free_socket = -1;
    int invalid = -1;
    int found_file = 0;
    struct file_info tmp_file;
    enum Command this_command;
    while(1)
    {
        this_command = EVIL;
        bzero(&remote_addr,sizeof(remote_addr));  //zero the struct 
        socklen_t sin_size = sizeof(remote_addr);
        if((free_socket = find_free_socket(client_sockets)) < 0)
        {
            fprintf(stderr, "No sockets available.\n");
            return EXIT_FAILURE;
        }

        if((client_sockets[free_socket].sd = accept(sd, (struct sockaddr *)&remote_addr, &sin_size)) < 0) 
        {
            //perror("Could not accept");
        }
        else
        {
            recv(client_sockets[free_socket].sd, &client_sockets[free_socket].port_number, 
                sizeof(int), 0);

            if(fcntl(client_sockets[free_socket].sd, F_SETFL, O_NDELAY) < 0)
            {
                fprintf(stderr, "Could not un-block socket.\n");
                return EXIT_FAILURE;    
            }
            
            fprintf(stdout, "Client %d connected\n", client_sockets[free_socket].port_number);
            //return EXIT_FAILURE;
            client_sockets[free_socket].connected = 1;    
        }   
        
        //wont enter into receive state at all if there's nothing to accept
        for(i=0; i < MAX_CLIENTS; i++)
        {
            if(client_sockets[i].connected)
            {
                this_command = EVIL;
                if((nbytes = recv(client_sockets[i].sd, &this_command, sizeof(this_command), 0)) > 1)
                {
                    switch(this_command)
                    {
                        case REGISTER:
                            bzero(&tmp_file, sizeof(tmp_file));
                            while((nbytes = recv(client_sockets[i].sd, &tmp_file, sizeof(tmp_file), 0)) > -1)
                            {
                                fprintf(stderr, "Files: %d\n", tmp_file.port_number);
                                //success case
                                //fprintf(stdout, "received file list from %d\n", tmp_file_list[0].port_number);
                                append_master_list(tmp_file);
                                //return EXIT_SUCCESS;
                                bzero(&tmp_file, sizeof(tmp_file));
                            } 
                            print_master_list(master_file_list);
                            break;

                        case DEREGISTER:
                            fprintf(stderr, "DEREGISTER is called elsewhere\n");
                            break;
                        case LS:
                            fprintf(stderr, "Sending file list.\n");
                            for(j=0; j < MAX_NUM_FILES; j++)
                            {
                                send(client_sockets[i].sd, &master_file_list[j], sizeof(master_file_list[j]), 0);
                            }
                            break;
                        case GET:
                            bzero(&file_name, sizeof(file_name));
                            found_file = 0;
                            // Make it blocking
                            int opts;
                            opts ^= O_NDELAY;
                            opts ^= O_NONBLOCK;
                            fcntl(client_sockets[i].sd, F_SETFL, opts);
                            
                            //sleep(2);
                            
                            recv(client_sockets[i].sd, file_name, sizeof(file_name), 0);
                            fprintf(stdout, "Retrieving file location for %s.\n", file_name);
                            for(j = 0; j < MAX_NUM_FILES; j++)
                            {
                                //fprintf(stderr, "Looping: %d\n", j);
                                //print_master_list(master_file_list);
                                if(strcmp(master_file_list[j].file_name, file_name) == 0)
                                {
                                    //fprintf(stderr, "SENDING %d - %s", master_file_list[j].port_number, master_file_list[j].location);
                                    send(client_sockets[i].sd, &master_file_list[j].port_number, sizeof(master_file_list[j].port_number), 0);
                                    send(client_sockets[i].sd, master_file_list[j].location, sizeof(master_file_list[j].location), 0);
                                    found_file = 1;
                                    break;
                                }
                            }
                            if(found_file == 0)
                            {
                                send(client_sockets[i].sd, &invalid, sizeof(invalid), 0);
                                send(client_sockets[i].sd, "NONE", sizeof("NONE"), 0);    
                            }
                            found_file = 0;
                            fcntl(client_sockets[i].sd, F_SETFL, O_NONBLOCK);
                            break;
                        default:
                            break; 
                    }
                }
                else if(nbytes == 0)
                {
                    sin_size = sizeof(remote_addr);
                    getsockname(client_sockets[i].sd, (struct sockaddr *)&remote_addr, &sin_size);
                    
                    deregister_client(client_sockets[i].port_number);
                    print_master_list(master_file_list);
                    close(client_sockets[i].sd);
                    client_sockets[i].connected = 0; 
                    client_sockets[i].port_number = 0;

                }
            }
        }
    }
    return EXIT_SUCCESS;
}