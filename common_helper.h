#define MAX_STRING_LENGTH 2000
#define MAX_NUM_FILES 200
#define MAX_CLIENTS 10

//enum is array of ints, key value pairing 
enum Command {EVIL, REGISTER, DEREGISTER, GET, LS};
 
struct file_info {
    char file_name[MAX_STRING_LENGTH];
    char file_size[MAX_STRING_LENGTH];
    int port_number;
    char location[MAX_STRING_LENGTH];
};
 
struct socket {
    int sd;
    int connected;
    int port_number;
};
 

 
int find_free_socket(struct socket* sockets)
{
    int i;
    for(i=0; i < MAX_CLIENTS; i++)
    {
        if(sockets[i].connected == 0)
        {
            return i;
        }
    }
    return -1;
}
 
int num_conn_clients(struct socket* sockets)
{
    int i;
    int num_conn_clients = 0;
    for(i=0; i < MAX_CLIENTS; i++)
    {
        if(sockets[i].connected == 1)
        {
            num_conn_clients++;
        }
    }
    return num_conn_clients;
}

int print_master_list(struct file_info* master_file_list)
{
    int i;
    fprintf(stdout,"Name\t\tSize\tPort\tLocation\n");
    for(i=0; i < MAX_NUM_FILES; i++)
    {
        if(master_file_list[i].port_number != 0)
        {
            fprintf(stdout, "%s\t%s\t%d\t%s\n", master_file_list[i].file_name, master_file_list[i].file_size,
             master_file_list[i].port_number, master_file_list[i].location);
        }
    }
    return EXIT_SUCCESS;
}