README
Programming Assignment 4
Peer to Peer File Server
Chris Sterling & Becca Seigel

Server can successfully accept multiple connections from multiple different clients- either simultaneously or asynchronously. The server can register and deregister clients- each time successfully updating the master file list. 

Once a client connects, it sends it's file list to the server and the server adds the names of the files into it's master file list. Whenever a new client is added, it's list also gets appended to the master client list.
The master client list lists the file name, it's size, what port it is from, and which client it's from. 

Clients can successfully request 'ls' and receive the master file list from the server- we decided not to push the master file list automatically to the clients because we thought that if each client already had the master client list, there would be no point to implement ls. So we did implement ls and didn't push the master file list out on client connection.

Clients can also successfully request 'get <filename>' from the server. The server finds the file, its port and its location- it tells the client that owns that file to send it to the requesting client. The client can receive the file successfully and it gets added to its directory. 

The client can do an ls and then do a get <filname>  and then will receive the file. 

We created test files test_loc_1 and test_loc_2 for the clients, where foo example files from PA1 live. 
Ctrl+C exits the connections

We chose not to implement SSL. 