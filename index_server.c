#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#define BUFLEN 100
#define MAXNUMFILES 9

char content_name_values[MAXNUMFILES][11];
char unique_content_name_values[MAXNUMFILES][11];
char peer_name_values[MAXNUMFILES][11];
char ip_values[MAXNUMFILES][10];
in_port_t client_port_values[MAXNUMFILES];
int num_times_read[MAXNUMFILES];
int numClients = 0;
int numuniqueVals=0;

// pdu strcuture to manage requests and responses, 
// type field for requests types, and data field for associated information 
struct pdu req_pdu, res_pdu;
char req_buffer[100], res_buffer[100], test_buf[50];
in_port_t receiving_port;

struct pdu {
    char type;
    char data[99];
};

// converts pdu to a bufferfor sending over the network.
void serialize(char type,char data[99] , char buffer[BUFLEN]) {
	buffer[0] = type;
	int i;
	for(i = 0 ; i < BUFLEN-1; i++) {
		buffer[i+1] = data[i];
	}
}

// Extracts information from a buffer into a pdu
void deserialize(struct pdu pdu_, char buffer[BUFLEN]) {
	pdu_.type = buffer[0];
	fprintf(stderr,"%c",buffer[0]);
	int i;
	for(i = 0; i < BUFLEN-1; i++) {
		pdu_.data[i] = buffer[i+1];
	}
}

int findIndexOfRecord(char peerName[10], char fileName[10]) {
	int i;
	for(i=0; i < numClients; i++) {
		if( strcmp(peer_name_values[i], peerName) == 0 && strcmp(content_name_values[i], fileName) == 0) {
			return i;
		}
	}
	return -1;
}

int findIndexOfFilename(char fileName[10]) {
	int i;
	for(i=0; i < numClients; i++) {
		if( strcmp(content_name_values[i], fileName) == 0) {
			return i;
		}
	}
	return -1;
}

int findIndexOfPeerName(char peerName[10]) {
	int i;
	for(i=0; i < numClients; i++) {
		if( strcmp(peer_name_values[i], peerName) == 0) {
			return i;
		}
	}
	return -1;
}

int findIndexOfUniqueFileName(char fileName[10]) {
	int i;
	for(i=0; i < numuniqueVals; i++) {
		if( strcmp(unique_content_name_values[i], fileName) == 0) {
			return i;
		}
	}
	return -1;
}

// handles client requests to register files
struct pdu register_client_server(struct pdu req, struct sockaddr_in sockadd) {
	int i;
	struct pdu resPdu;
	char peerName[11];
	char fileName[11];
	char ip_sent[10];

	// peerName from index 0 to 10, fileName from index 11 to 21, ip_sent from index 22 to 31
	strncpy(peerName, req_pdu.data, sizeof(peerName));
	strncpy(fileName, req_pdu.data + 11, sizeof(fileName));
	strncpy(ip_sent, req_pdu.data + 22, sizeof(ip_sent));
	memcpy(&receiving_port, req.data+32, sizeof(receiving_port));

	fprintf(stderr, "\n=====Registering file=====\n");
	fprintf(stderr,"Filename is: %s\nPeer is: %s\nIP is: %s\nPort is: %d\n", fileName, peerName, ip_sent,ntohs(receiving_port));
	
	// calls findIndexOfRecord to check if this peer and file combination already exists
	if(findIndexOfRecord(peerName, fileName) == -1 && numClients < MAXNUMFILES) {
		// register the file
		strncpy(peer_name_values[numClients], peerName, sizeof(peer_name_values[numClients]));
		strncpy(content_name_values[numClients], fileName, sizeof(content_name_values[numClients]));
		strncpy(ip_values[numClients], ip_sent, sizeof(ip_values[numClients]));
		client_port_values[numClients] = receiving_port;
		numClients++; // keep track of the registered entries
		
		// add file to unique_content_name_values if its not there yet
		if(findIndexOfUniqueFileName(fileName) < 0) {
			strncpy(unique_content_name_values[numuniqueVals], fileName, sizeof(unique_content_name_values[numuniqueVals]));
			numuniqueVals++;
		}
		fprintf(stderr, "File registered.. Sending back acknlowledgement\n");
		resPdu.type = 'A';
	
	// if server's file registration limit has been reached, sends a message and type to 'E'
	} else if(numClients >= MAXNUMFILES) {
		fprintf(stderr, "Server Quota reached.\n");
		resPdu.type = 'E';
		strncpy(resPdu.data, "Server Quota reached.", sizeof("Server Quota reached."));
	
	// if file already exists
	} else {
		fprintf(stderr, "File already exists.. Sending error.\n");
		resPdu.type = 'E';
		strncpy(resPdu.data, "File already exists", sizeof("File already exists"));
	}
	return resPdu;
}

// removes any specific file registered by a peer combinations
struct pdu deregister_client_server(struct pdu req) {
	int i, unique_indx, j;
	struct pdu resPdu;
	char peerName[10];
	char fileName[10];
	
	strncpy(fileName, req.data, sizeof(fileName)); // fileName from index 0 to 10
	strncpy(peerName, req.data + 11, sizeof(peerName));	// index 11 and up
	fprintf(stderr, "\n=====Deregistering file=====\n");
	fprintf(stderr, "Filename is: %s\nPeer is: %s\n", fileName, peerName);
	
	int index = findIndexOfRecord(peerName, fileName);	// search peerName and fileName from the list
	if(index > -1) {
		// this loop shifts each record in the arrays one position up from the index to remove the targeted file
		for(i = index; i < numClients -1; i++) {
			strncpy(content_name_values[i], content_name_values[i+1], sizeof(content_name_values[i+1]));
			strncpy(peer_name_values[i], peer_name_values[i+1], sizeof(peer_name_values[i+1]));
			strncpy(ip_values[i], ip_values[i+1], sizeof(ip_values[i+1]));
			client_port_values[i] = client_port_values[i+1];
			num_times_read[i] = num_times_read[i+1];
		}

		//clear last value of array and decrement numclients
		memset(content_name_values[numClients-1], '\0', sizeof(content_name_values[numClients-1]));
		memset(peer_name_values[numClients-1], '\0', sizeof(peer_name_values[numClients-1]));
		memset(ip_values[numClients-1], '\0', sizeof(ip_values[numClients-1]));
		client_port_values[numClients-1] = 0;
		num_times_read[numClients-1] = 0;
		numClients--;
		
		// if no more entries for fileName exist, it is removed from unique_content_name_values
		if(findIndexOfFilename(fileName) < 0) {
			unique_indx = findIndexOfUniqueFileName(fileName);	// locates the index of fileName in unique_content_name_values
			if(unique_indx >= 0) {
				// if it exists, this loop shifts entries down, removing fileName from the unique list
				for(j=unique_indx; j < numuniqueVals; j++) {
					strncpy(unique_content_name_values[j], unique_content_name_values[j+1], sizeof(unique_content_name_values[j+1]));
				}
				memset(unique_content_name_values[unique_indx], '\0', sizeof(unique_content_name_values[unique_indx]));
				numuniqueVals--;
			}
		}

		fprintf(stderr,"File deregistered.. Sending Acknowledgment \n");
		resPdu.type = 'A';
	} else {
		fprintf(stderr,"File could not be found. Sending error\n");
		resPdu.type = 'E';
	}

	return resPdu;
}

// removes all files registered by a peer
struct pdu deregister_all_client_server(struct pdu req) {
	int i, unique_indx, j, index;
	struct pdu resPdu;
	char peerName[11], fileName[11];
	strncpy(peerName, req.data, sizeof(peerName));
	fprintf(stderr, "\n=====Deregistering file=====\n");
	fprintf(stderr, "Peer is: %s\n", peerName);
	while((index = findIndexOfPeerName(peerName)) > -1){
		//if non-last value is deregistered move all values back one index.
		strncpy(fileName, content_name_values[i], sizeof(content_name_values[i]));
		for(i = index; i < numClients -1; i++) {
			strncpy(content_name_values[i], content_name_values[i+1], sizeof(content_name_values[i+1]));
			strncpy(peer_name_values[i], peer_name_values[i+1], sizeof(peer_name_values[i+1]));
			strncpy(ip_values[i], ip_values[i+1], sizeof(ip_values[i+1]));
			client_port_values[i] = client_port_values[i+1];
			num_times_read[i] = num_times_read[i+1];
		}
		//clear last value of array and decrement numclients
		memset(content_name_values[numClients-1], '\0', sizeof(content_name_values[numClients-1]));
		memset(peer_name_values[numClients-1], '\0', sizeof(peer_name_values[numClients-1]));
		memset(ip_values[numClients-1], '\0', sizeof(ip_values[numClients-1]));
		client_port_values[numClients-1] = 0;
		num_times_read[numClients-1] = 0;
		numClients--;
		
		//ammends unique name array if no more copies of file exist in index records.
		if(findIndexOfFilename(fileName) < 0) {
			unique_indx = findIndexOfUniqueFileName(fileName);
			if(unique_indx >= 0) {
				for(j=unique_indx; j < numuniqueVals; j++) {
					strncpy(unique_content_name_values[j], unique_content_name_values[j+1], sizeof(unique_content_name_values[j+1]));
				}
				memset(unique_content_name_values[unique_indx], '\0', sizeof(unique_content_name_values[unique_indx]));
				numuniqueVals--;
			}
		}
	}
		fprintf(stderr,"Client deregistered.. Sending Acknowledgment \n");
		resPdu.type = 'A';
	return resPdu;
}

// searches for a registered file and returns the client informtion if found
struct pdu find_client_server_for_file(char fileName[10]) {
	int i;
	struct pdu resPdu;
	in_port_t testport;
	int lastIndx= -1;
	int lastNumTimesRead = -1;
	fprintf(stderr,"\n=====Searching for file=====\n");
	fprintf(stderr, "Filename is: %s\n", fileName);
	for(i = 0; i < numClients; i++) {
		if(strcmp(content_name_values[i], fileName) == 0) {
			if(lastNumTimesRead == -1 || lastNumTimesRead > num_times_read[i]) {
			lastIndx = i;
			lastNumTimesRead = num_times_read[i];
			}
		}
	}
	if(lastIndx > -1) {
		fprintf(stderr,"File found and chosen for server: %s\nSending Details...\n", peer_name_values[lastIndx]);
		resPdu.type = 'S';
		memcpy(resPdu.data, ip_values[lastIndx] , sizeof(ip_values[lastIndx]));
		memcpy(resPdu.data + 10, &client_port_values[lastIndx] , sizeof(client_port_values[lastIndx]));
		memcpy(&testport, resPdu.data + 10, sizeof(testport));
		fprintf(stderr, "IP of server is %s\nPort is: %d\n",resPdu.data, ntohs(testport));
		num_times_read[lastIndx] ++;
	} else {
		fprintf(stderr,"File not found\n");
		resPdu.type = 'E';
		strcpy(resPdu.data, "File not found");
	}
	return resPdu;
}

// lists all unique files currently registered on the server
struct pdu list_files_in_library() {
	int i, j,h=0, total_indx=0, str_len;
	size_t offset = 0;
	struct pdu tmpPdu;
	tmpPdu.type = 'O';
	fprintf(stderr, "\n====Listing Files====\n");
	fprintf(stderr, "Files in library: %d\n", numuniqueVals);
    for (i = 0; i < numuniqueVals; ++i) {
		str_len = strlen(unique_content_name_values[i]);
		for(j = 0; j < str_len; j++) {
			tmpPdu.data[h++] = unique_content_name_values[i][j];
		}
		if(i < numuniqueVals-1) tmpPdu.data[h++] = ':';
		fprintf(stderr, "%d. %s\n", i+1, unique_content_name_values[i]);
    }
	tmpPdu.data[h] = '\0';
	return tmpPdu;
}

int main(int argc, char *argv[]) {
	struct  sockaddr_in fsin;	/* the from address of a client	*/
	int	sock;					/* server socket */
	int	alen;					/* from-address length */
	struct	sockaddr_in sin; 	/* an Internet endpoint address */
    int	s, type;        		/* socket descriptor and socket type */
	int port=3000;
	
	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
																								
	/* Allocate a socket */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	fprintf(stderr, "can't creat socket\n");
													
	/* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	fprintf(stderr, "can't bind to %d port\n",port);;
	listen(s, 5);	
	alen = sizeof(fsin);
	fprintf(stderr, "======Index server======\n");
	while (1) {
		if (recvfrom(s, req_buffer, sizeof(req_buffer), 0, (struct sockaddr *)&fsin, (unsigned int *) &alen) < 0)
			fprintf(stderr, "recvfrom error\n");
		
			req_pdu.type = req_buffer[0];
			int i;
			for(i = 0; i < BUFLEN-1; i++) req_pdu.data[i] = req_buffer[i+1];
		
		switch(req_pdu.type) {
			case 'R':
				res_pdu = register_client_server(req_pdu, fsin);
				break;
			case 'T':
				res_pdu = deregister_client_server(req_pdu);
				break;
			case 'S':
				res_pdu = find_client_server_for_file(req_pdu.data);
				break;
			case 'O':
				res_pdu = list_files_in_library();
				break;
			case 'Q':
				res_pdu = deregister_all_client_server(res_pdu);
			case 'E':
			default:
				fprintf(stderr,"\n=====Unsupported request received %s=====\n", req_pdu.data);
				res_pdu.type = 'E';
				strcpy(res_pdu.data, "Unsupported request");	
		}
		fprintf(stderr, "\n===== Sending request type: %c with data %s =====\n", res_pdu.type, res_pdu.data);
		serialize(res_pdu.type,res_pdu.data, res_buffer);
		(void) sendto(s, res_buffer, sizeof(res_buffer), 0, (struct sockaddr *)&fsin, sizeof(fsin));
	}
}
