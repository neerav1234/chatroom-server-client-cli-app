#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#define MAX_LEN 200
#define NUM_COLORS 6

using namespace std;

struct client_identifier
{
	int cl_no;
	string name;
	int client_socket;
	thread th;
};

vector<client_identifier> clients;
string def_col="\033[0m";
string colors[]={"\033[31m", "\033[37m", "\033[33m", "\033[34m", "\033[35m","\033[36m", "\033[1;97m"};
int seed=0;
mutex cout_mtx,clients_mtx;

string color(int code);
void set_name(int cl_no, char name[]);
void printForServer(string str, bool endLine);
int brodcasting(string message, int sender_id);
int brodcasting(int num, int sender_id);
void terminate_connection(int id);
void handle_client(int client_socket, int id);

int main()
{
	int server_socket;
	if((server_socket=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket: ");
		exit(-1);
	}

	struct sockaddr_in server;
	server.sin_family=AF_INET;
	server.sin_port=htons(10000);
	server.sin_addr.s_addr=INADDR_ANY;
	bzero(&server.sin_zero,0);

	if((bind(server_socket,(struct sockaddr *)&server,sizeof(struct sockaddr_in)))==-1)
	{
		perror("bind error: ");
		exit(-1);
	}

	if((listen(server_socket,8))==-1)
	{
		perror("listen error: ");
		exit(-1);
	}

	struct sockaddr_in client;
	int client_socket;
	unsigned int len=sizeof(sockaddr_in);

	cout<<colors[NUM_COLORS]<<"\n\t  ###### Hey!, You just initiated a ChatRoom Server. ######   "<<endl<<def_col;

	while(1)
	{
		if((client_socket=accept(server_socket,(struct sockaddr *)&client,&len))==-1)
		{
			perror("accept error: ");
			exit(-1);
		}
		seed++;
		thread t(handle_client,client_socket,seed);
		lock_guard<mutex> guard(clients_mtx);
		clients.push_back({seed, string("NoName"),client_socket,(move(t))});
	}

	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].th.joinable())
			clients[i].th.join();
	}

	close(server_socket);
	return 0;
}

string color(int code)
{
	return colors[code%NUM_COLORS];
}

// Set name of client
void set_name(int id, char name[])
{
	for(int i=0; i<clients.size(); i++)
	{
			if(clients[i].cl_no==id)	
			{
				clients[i].name=string(name);
			}
	}	
}

// For synchronisation of cout statements
void printForServer(string str, bool endLine=true)
{	
	lock_guard<mutex> guard(cout_mtx);
	cout<<str;
	if(endLine)
			cout<<endl;
}

// Broadcast message to all clients except the sender
int brodcasting(string message, int sender_id)
{
	char temp[MAX_LEN];
	strcpy(temp,message.c_str());
	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].cl_no!=sender_id)
		{
			send(clients[i].client_socket,temp,sizeof(temp),0);
		}
	}		
}

// Broadcast a number to all clients except the sender
int brodcasting(int num, int sender_id)
{
	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].cl_no!=sender_id)
		{
			send(clients[i].client_socket,&num,sizeof(num),0);
		}
	}		
}

void terminate_connection(int id)
{
	for(int i=0; i<clients.size(); i++)
	{
		if(clients[i].cl_no==id)	
		{
			lock_guard<mutex> guard(clients_mtx);
			clients[i].th.detach();
			clients.erase(clients.begin()+i);
			close(clients[i].client_socket);
			break;
		}
	}				
}

void handle_client(int client_socket, int id)
{
	char name[MAX_LEN],str[MAX_LEN];
	recv(client_socket,name,sizeof(name),0);
	set_name(id,name);	

	// Display welcome message
	string welcome_message=string(name)+string(" has joined our chat");
	brodcasting("control msg",id);	
	brodcasting(id,id);								
	brodcasting(welcome_message,id);	
	printForServer("\033[1;92m"+welcome_message+def_col);
	
	while(1)
	{
		int bytes_received=recv(client_socket,str,sizeof(str),0);
		if(bytes_received<=0)
			return;
		if(strcmp(str,"##exit")==0)
		{
			// Display leaving message
			string message=string(name)+string(" has left this chat");		
			brodcasting("control msg",id);			
			brodcasting(id,id);						
			brodcasting(message,id);
			printForServer("\033[1;91m"+message+def_col);
			terminate_connection(id);							
			return;
		}
		brodcasting(string(name),id);					
		brodcasting(id,id);		
		brodcasting(string(str),id);
		printForServer(color(id)+string(name)+" : "+def_col+str);		
	}	
}