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
#define USAGE "./client_PFS <Port Number> <Name>" // <Private Key of Server> <Certificate of Server> <CA Cert>"

// You MUST pass files by reference.
int get_file_list(struct file_info* files, int this_port_number, int* num_files, char* location)
{
    FILE* proc_pointer;
    char file_name[MAX_STRING_LENGTH];
    char file_size[MAX_STRING_LENGTH];
    char* command = "ls -alh | awk {'print $5\" \"$9'}";
    int count;
    count = 0;
     
    /* Open the command for reading. */
    proc_pointer = popen(command, "r");
    if( proc_pointer == NULL )
    {
        fprintf(stderr, "Failed to run command 'ls'\n");
        return EXIT_FAILURE;
    }
     
    /* Read the output a line at a time. */
    while( fscanf(proc_pointer, "%s %s", file_size, file_name) != EOF ) 
    {
        if( strcmp(file_name, ".") != 0 && strcmp(file_name, "..") != 0 )
        {
            strcpy(files[count].location, location);
            strcpy(files[count].file_name, file_name);
            strcpy(files[count].file_size, file_size);
            files[count].port_number = this_port_number;
            count++;
        }
    }
     
    // Close the process out.
    pclose(proc_pointer);
    *num_files = count;
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 3)
    {
        fprintf(stderr, "Incorrect number of arguments. Usage should be:\n %s\n", USAGE);
        return EXIT_FAILURE;
    }
    struct sockaddr_in local_addr, remote_addr;
    int sd, peer_sd, remote_sd; //socket descriptors
    char buffer[MAX_STRING_LENGTH];
    int num_files, i, port_number, temp_socket; 
    socklen_t addr_length;

    struct file_info my_files[MAX_NUM_FILES];

    // Clear socket buffers
    bzero(&local_addr, sizeof(local_addr));
    bzero(&remote_addr, sizeof(remote_addr));
     
    // htons() sets the port # to network byte order
    // Setup for local
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = 0; // Find any available port.
    
    // Setup for remote 
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    remote_addr.sin_port = htons(atoi(argv[1]));
     
     
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "error opening socket\n");
        return EXIT_FAILURE;
    }

    /* Get the peer socket bound and listening. */
    if((peer_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "error opening socket\n");
        return EXIT_FAILURE;
    }

    /* Get the peer socket bound and listening. */
    /*if((remote_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "error opening socket\n");
        return EXIT_FAILURE;
    }*/

    if(bind(peer_sd, (struct sockaddr *)&local_addr, sizeof(local_addr))<0)
    {
        return EXIT_FAILURE;
    }

    /*if(fcntl(peer_sd, F_SETFL, O_NDELAY) < 0)
    {
        fprintf(stderr, "Could not un-block socket.\n");
        return EXIT_FAILURE;    
    }*/

    if (listen(peer_sd, 20) == -1)
    {
        fprintf(stderr, "Listen Error!\n");
        return EXIT_FAILURE;
    }

    addr_length = sizeof(local_addr);
    /*retrieves port to localaddr*/
    getsockname(peer_sd, (struct sockaddr *)&local_addr, &addr_length);
    port_number = (int) ntohs(local_addr.sin_port);
    fprintf(stderr, "This client is now listening on %d for peer connections.\n", port_number);


    /* Connect to server. */
	if(connect(sd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) < 0)
	{
		int errsv = errno;
        //printf("somecall() failed\n");
		fprintf(stderr, "Connect error of %s\n", strerror(errsv));
		return EXIT_FAILURE;
	}
    send(sd, &port_number, sizeof(port_number),0);

    if(get_file_list(my_files, port_number, &num_files, argv[2]) == EXIT_FAILURE)
    {
        fprintf(stderr, "There was a problem retreiving the file list. Whoops.\n");
        return EXIT_FAILURE;
    }
	
    enum Command this_command = REGISTER;
    send(sd, &this_command, sizeof(this_command), 0);

    // This will send the whole damn file list. Hooray!
    for(i=0; i < num_files; i++)
    {
        send(sd, &my_files[i], sizeof(my_files[i]), 0);
    }
    //fprintf(stderr, "Sent file list. Size: %d\n", send(sd, my_files, sizeof(my_files), 0));
    // Enter our accept loop. Wait for an incoming command.
    // TODO - multithread?
    char command[MAX_STRING_LENGTH];
    char file_name[MAX_STRING_LENGTH]; 
    char get_file_name[MAX_STRING_LENGTH];
    char input[MAX_STRING_LENGTH];
    int remote_port_number;
    char location[MAX_STRING_LENGTH];
    FILE *file_pointer;
    struct file_info master_file_list[MAX_NUM_FILES];
    pid_t child_pid;
    child_pid = fork();
    // DON'T PUT SCHTUFF HERE
    if(child_pid == 0) // child process
    { // I listen to the command line for commands - then I do schtuff with them
        while(1)
        {
            bzero(command, MAX_STRING_LENGTH);
            bzero(get_file_name, MAX_STRING_LENGTH);
            bzero(input, MAX_STRING_LENGTH);
            this_command = EVIL;
            gets(input);
            sscanf(input, "%s %s", command, get_file_name);
            fprintf(stdout, "Command: %s\n", command);
            if(strcmp("ls", command) == 0)
            {
                bzero(master_file_list, sizeof(master_file_list));
                this_command = LS; 
                send(sd, &this_command, sizeof(this_command), 0);
                for(i=0; i < MAX_NUM_FILES; i++)
                {
                    recv(sd, &master_file_list[i], sizeof(master_file_list[i]), 0);
                }
                print_master_list(master_file_list); 
            }
            if(strcmp("get", command) == 0)
            {
                // Run to the server, ask it the location and port of the file
                this_command = GET;
                send(sd, &this_command, sizeof(this_command), 0);
                send(sd, get_file_name, sizeof(get_file_name), 0);

                recv(sd, &remote_port_number, sizeof(remote_port_number), 0);
                recv(sd, location, sizeof(location), 0);

                fprintf(stdout, "Retrieving file %s from %s on %d.\n", get_file_name, location, remote_port_number);
                    /* Get the peer socket bound and listening. */
                if((remote_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                    fprintf(stderr, "error opening socket\n");
                    return EXIT_FAILURE;
                }
                // Connect to location and port
                remote_addr.sin_port = htons(remote_port_number);

                if(connect(remote_sd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) < 0)
                {
                    int errsv = errno;
                    //printf("somecall() failed\n");
                    fprintf(stderr, "Connect error of %s\n", strerror(errsv));
                    //return EXIT_FAILURE;
                }
                else
                {
                    // send it get command
                    send(remote_sd, get_file_name, sizeof(get_file_name), 0);

                    file_pointer = fopen(get_file_name, "w");
                    fprintf(stdout, "Opened file %s\n", get_file_name);        
                    // write out result to file
                    while(recv(remote_sd, buffer, sizeof(buffer), 0) != 0 )
                    {
                        //printf("Buffer: %s", buffer);
                        fputs(buffer, file_pointer);
                    }
                    bzero(&buffer, sizeof(buffer));
                    fclose(file_pointer);
                    close(remote_sd);    
                }
                
                
            }
            else
            {
                fprintf(stderr, "Command not recognized\n");
            }
        }

    }
    else //parent process
    { // I wait for peers to connect, then all I'm responsible for is sending a file
        while(1)
        {
            // Note, temp socket is blocking.
            //returned as connection to peer: temp_socket
            if((temp_socket = accept(peer_sd, (struct sockaddr *)&local_addr, &addr_length)) < 0) 
            {
                int errsv = errno;
                fprintf(stderr, "Accept error of %s\n", strerror(errsv));
            }
            fprintf(stdout, "Peer client now connected.\n");
            bzero(&file_name, sizeof(file_name));
            recv(temp_socket, &file_name, sizeof(file_name), 0);

            file_pointer = fopen(file_name, "r");
        
            if (file_pointer == NULL)
            {
              fprintf(stderr, "Cannot open input file %s!\n", file_name);
              return EXIT_FAILURE;
            }
            while(fgets(buffer, MAX_STRING_LENGTH, file_pointer) != NULL)
            {
                //fread(buffer, MAX_STRING_LENGTH, 1, file_pointer);
                send(temp_socket, &buffer, sizeof(buffer), 0);
                bzero(&buffer, sizeof(buffer));
            }
            fclose(file_pointer);

            //gets rid of tmp socket, 2- if it receives or anything left in buffer, get rid of it
            close(temp_socket);
            fprintf(stderr, "Connection torn down.\n");
        }    
    }


    
    
	return EXIT_SUCCESS;
}
