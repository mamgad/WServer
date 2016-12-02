/*
    WServer -An HTTP over TCP Web Server
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>

//Constants
#define BUFFER_MAX 4096
#define GET 0
#define POST 1
#define MAX_FILE_SIZE 2147483648

//Methods declarations
void printLogo();
void error(char *msg);
void parseRequest();
void serveRequest();

//Connection variables
int fileDescriptorServer, fileDescriptorClient, portNumber, clientLength, requestResult, statusValue=1;

int main(int argc, char *argv[])
{
    printLogo();

    //Buffer declaration and initialization - Alter the BUFFER_MAX definition when needed
    char buffer[BUFFER_MAX];
    struct sockaddr_in serverAddress, clientAddress;

    //Check and handle default port number.
    if (argc < 2)
    {
        fprintf(stderr, "Default port 2030 will be used\n\e[39m");
        portNumber = 2030;
    }

    //Initialize the socket file descriptor and handle failure
    fileDescriptorServer = socket(AF_INET, SOCK_STREAM, 0);

    //Set address as reusable
    setsockopt(fileDescriptorServer, SOL_SOCKET, SO_REUSEADDR, &statusValue, sizeof(statusValue));

    printf("[*]Attempting to open socket\n\e[39m");
    if (fileDescriptorServer < 0)
    {
        error("\e[31m[!]ERROR opening socket\e[39m");
    }
    printf("\e[1m\e[32mDone \e[39m\n");

    //Clear the server address
    bzero((char *) &serverAddress, sizeof (serverAddress));

    //portNumber = atoi(argv[1]);
    portNumber = 2030;

    //Set famile to internet connection
    serverAddress.sin_family = AF_INET;

    //Any IP Address can connect
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    //Set listening port number
    serverAddress.sin_port = htons(portNumber);
    printf("\e[0m[*]Attempting to bind socket\n\e[39m");
    if (bind(fileDescriptorServer, (struct sockaddr *) &serverAddress, sizeof (serverAddress)) < 0)
    {
        error("\e[31m[!]ERROR on binding\e[39m");
    }
    printf("\e[1m\e[32mDone \e[39m\n");

    //Handle requests indefinitely
    while (1)
    {
        listen(fileDescriptorServer, 5);
        clientLength = sizeof (clientAddress);
        fileDescriptorClient = accept(fileDescriptorServer, (struct sockaddr *) &clientAddress, &clientLength);
        printf("[*]Attempting to accept incoming connecion..\n");
        if (fileDescriptorClient < 0)
        {
            error("[!]ERROR on accept");
        }
        printf("\e[1m\e[32mDone \e[39m\n");
        bzero(buffer, BUFFER_MAX);
        requestResult = read(fileDescriptorClient, buffer, BUFFER_MAX-1);
        if (requestResult < 0)
        {
            error("ERROR reading from socket");
        }
        else
        {
            //Parse the request - Request server procedure is called from within the parser
            parseRequest(buffer);
        }

        close(fileDescriptorClient);
    }
    return 0;
}

//Print the logo
void printLogo()
{
    printf("\e[1m\e[32m __        ______                           \n \\ \\      / / ___|  ___ _ ____   _____ _ __ \n  \\ \\ /\\ / /\\___ \\ / _ \\ '__\\ \\ / / _ \\ '__|\n   \\ V  V /  ___) |  __/ |   \\ V /  __/ |   \n    \\_/\\_/  |____/ \\___|_|    \\_/ \\___|_|   \n\n\n\n\e[0m");
}

//Error message display
void error(char *msg)
{
    perror(msg);
    exit(1);
}

//Parse incoming request
void parseRequest(char * requestMessage)
{

    int i=0;
    //printf("Request:\n%s\n", requestMessage);
    char * delimeter = "\r\n";
    char * parsedLine =  strtok(requestMessage, delimeter);
    char lines[100][100];
    bzero(lines,sizeof(lines));
    //Parse lines
    while(parsedLine)
    {
        strcpy(lines[i],parsedLine);
        printf("Line #%d => %s\n", i+1, lines[i]);
        i++;
        parsedLine = strtok(NULL, delimeter);
    }

    //Parse first line
    int requestType;
    char * requestedFileName;
    char * firstLineToken =  strtok(lines[0], " ");

    //Parse request type
    if(!strcmp(firstLineToken, "GET")) requestType=GET;
    else if(!strcmp(firstLineToken, "POST")) requestType=POST;

    //Parse reqested file name
    firstLineToken=strtok(NULL," ");
    serveRequest(firstLineToken, requestType);
}

//Serve the request
void serveRequest(char * requestedFileName, int requestType)
{

    char * contentBuffer=0;
    if(requestType==GET)
    {
        //Trim first character "/"
        requestedFileName++;
        if(!strcmp(requestedFileName,"")) strcpy(requestedFileName,"index.html");
        printf("Requested file: %s\n",requestedFileName);
        printf("Client address: %d\n", fileDescriptorClient);
        //Send file
        FILE * file = fopen(requestedFileName,"rb");
        printf("FILE = %d\n", file);
        //404 handling
        if(file==NULL || file<0)
        {
            char error404[] = "<html><head><title>WServer - Page not found</title></head><body><h1 style=\"color:#2abada\">WServer</h1><h2>Oops! The requested page could not be found!</h2></body></html>";
            send(fileDescriptorClient,"HTTP/1.1 404 Not Found\r\n", 23, 0);
            send(fileDescriptorClient,"Content-Length: 2048",19, 0);
            send(fileDescriptorClient,"Content-Type: text/html\r\n",25,0);
            send(fileDescriptorClient,"\r\n",2, 0);
            send(fileDescriptorClient,error404,sizeof(error404), 0);
            puts("Failed to open the requested file, replied with 404.");
        }
        else
        {
            //Find file size
            size_t fileSize = lseek(file, 0, SEEK_END)+1;
            printf("File size = %d bytes\n",fileSize);
            //Fetch content
            fseek (file, 0, SEEK_SET);
            contentBuffer = malloc (fileSize);
            if (contentBuffer)
            {
                fread (contentBuffer, 1, fileSize, file);
            }
            printf("\nFile content:\n%s",contentBuffer);
            //Start transfer
            send(fileDescriptorClient,"HTTP/1.1 200 OK\r\n",18,0);
            send(fileDescriptorClient,"Server: WServer v1\r\n", 21, 0);
            send(fileDescriptorClient,"Content-Type: text/html\r\n",25,0);
            send(fileDescriptorClient,"\r\n", 2,0);
            send(fileDescriptorClient,contentBuffer,fileSize,0);
            puts("\e[1m\e[32mServed \e[39m\n");

            //Close file on completion

        }
    fclose(file);
    }

    else if(requestType==POST)
    {
        printf("Recieved file: %s\n",requestedFileName);
        //Recieve file - to be implemented
    }
}
