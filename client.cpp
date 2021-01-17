#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <bits/stdc++.h>
#define bufferSize 5000

using namespace std;

//connection file descriptor integer
int socketfd;
/****************** error handler *******************/
void error(const char *msg)
{
    perror(msg);
    exit(0);
}
/****************************************************/

/**************** request parser ********************/
void parseRequest(string request, vector<string> &parsedReq)
{
    if(request[0] == 'G')
    {
        parsedReq.push_back("GET");
        parsedReq.push_back(request.substr(4, request.length() - 4));
    }
    else
    {
        parsedReq.push_back("POST");
        parsedReq.push_back(request.substr(5, request.length() - 5));
    }


}

/****************************************************/

/************ read and write file class *************/
class FileReaderAndWriter
{
public:
    size_t readFile(string filePath, char* file)
    {
        ifstream fin(filePath, std::ios::binary | ios::in);
        if(!fin.good())
            return -1;
        fin.seekg(0, std::ios::end);
        size_t filesize = (int)fin.tellg();
        fin.seekg(0);
        if(fin.read((char*)file, filesize))
        {
            fin.close();
            return filesize;
        }
        return -1;;
    }
    bool writeFile(string filePath, char* file, size_t fileSize)
    {
        std::ofstream fout(filePath, std::ios::binary | ios::out);
        if(!fout.good())
            return false;
        fout.write((char*)file, fileSize);
        fout.close();
        return true;
    }
};
/****************************************************/

/*************** GET request handler ****************/
void getRequestHandler(string request)
{
    char* buffer = new char[bufferSize];
    bzero(buffer, bufferSize);
    vector<string> parsedRequest;
    parseRequest(request, parsedRequest);
    request += "\r\n\r\n";
    //send request
    if(write(socketfd, request.c_str(), bufferSize) == -1)
    {
        error("Cannot write to socket");
    }
    //receive status
    bzero(buffer, request.length());
    read(socketfd, buffer, 30);
    //check if 200 OK or 404 Not Found
    cout << "server: " << string(buffer)<<endl;
    if(buffer[9] == '4') // 404 not found
    {
        return;
    }
    size_t fileSize ;
//receive size of the file
    read(socketfd, &fileSize, sizeof(size_t));
//receive file
    bzero(buffer,fileSize);
    read(socketfd, buffer, fileSize);
    FileReaderAndWriter frw;
    frw.writeFile(parsedRequest[1], buffer, fileSize);
    delete[] buffer;
}
/****************************************************/

/************** POST request handler ****************/
void postRequestHandler(string request)
{
    char* buffer = new char[bufferSize];
    bzero(buffer, bufferSize);
    //parse request to get file's path
    vector<string> parsedRequest;
    parseRequest(request, parsedRequest);
    //read file
    FileReaderAndWriter frw;
    size_t fileSize;
    string filePath = parsedRequest.at(1);
    fileSize = frw.readFile(filePath, buffer);
    if(fileSize == -1)
    {
        cout << "client: no file with that path" << endl;
        return;
    }
    request += "\r\n\r\n";
    //send request
    if(write(socketfd, request.c_str(), bufferSize) == -1)
        error("Cannot send request");
    //send size of file
    if(write(socketfd,(void *)&fileSize, sizeof(size_t)) == -1)
        error("Cannot write to socket");
    //send file
    if(write(socketfd, buffer, fileSize) == -1)
        error("Cannot write to socket");
    //receive response
    bzero(buffer, bufferSize);
    if(read(socketfd, buffer, 20) == -1)
        error("Cannot read from socket");
    cout << "server: " << string(buffer);
    delete[] buffer;
}
/****************************************************/

/**************** read requests' file ***************/
void readRequestFile(vector<string> &requests, string filePath)
{
    ifstream fin;
    fin.open(filePath);
    string line;
    while(fin)
    {
        getline(fin, line);
        requests.push_back(line);
    }
    // an empty line is read so we need to pop the last line
    requests.pop_back();
}
/****************************************************/


int main(int argc, char *argv[])
{
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    string requestsFilePath = "requestsFile.txt";
    vector<string> requests;

    if (argc < 3)
    {
        cout << "missing arguments";
        exit(0);
    }
    portno = atoi(argv[2]);
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(socketfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting to the server");
    else
        cout << "connected to server on port: " << portno << endl;

    //reading request from file
    readRequestFile(requests, requestsFilePath);
    // handle request
    for(int i = 0; i < requests.size(); i++)
    {
        string request = requests.at(i);
        cout << "client: " << request << endl;
        if(request[0] == 'G')
        {
            getRequestHandler(request);
        }
        else
        {
            postRequestHandler(request);
        }
        //usleep(1000000);
    }
    close(socketfd);
    cout << "connection closed" << endl;
    return 0;
}
