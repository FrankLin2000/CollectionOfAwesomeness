#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include "mandelbrot.h"
#include "pixelColor.h"

int waitForConnection (int serverSocket);
int makeServerSocket (int portno);
void serveBMP (int socket,double centerX,double centerY,double z);
char analyzeFileName(char* request,double* x,double* y,double* z);
void gen(double centerX,double centerY,unsigned char zoom,unsigned char* result);
void serveHTML (int socket);

#define SIMPLE_SERVER_VERSION 1.0
#define REQUEST_BUFFER_SIZE 1000
#define DEFAULT_PORT 7191
#define NUMBER_OF_PAGES_TO_SERVE 10

#define HEAD_OFFSET 54
#define SIZE 512

int main (int argc, char *argv[]) {
      
   printf ("************************************\n");
   printf ("Starting simple server %f\n", SIMPLE_SERVER_VERSION);
   printf ("Serving bmps since 2012\n");   
   
   int serverSocket = makeServerSocket (DEFAULT_PORT);   
   printf ("Access this server at http://localhost:%d/\n", DEFAULT_PORT);
   printf ("************************************\n");
   
   char request[REQUEST_BUFFER_SIZE];
   int numberServed = 0;
   while (numberServed < NUMBER_OF_PAGES_TO_SERVE) {
      
      printf ("*** So far served %d pages ***\n", numberServed);
      
      int connectionSocket = waitForConnection (serverSocket);
      // wait for a request to be sent from a web browser, open a new
      // connection for this conversation
      
      // read the first line of the request sent by the browser  
      int bytesRead;
      bytesRead = read (connectionSocket, request, (sizeof request)-1);
      assert (bytesRead >= 0); 
      // were we able to read any data from the connection?


      // print entire request to the console 
      printf (" *** Received http request ***\n%s\n",request);
      
      //Get the parameters for geneate the bmp file
      double x,y,z;
      char r=analyzeFileName(request,&x,&y,&z);
      printf("x=%f,y=%f,z=%f\n",x,y,z);   

      //send the browser a simple html page using http
      printf (" *** Sending http response ***\n");
      serveBMP(connectionSocket,x,y,z);
      if(r==0){
          serveHTML(connectionSocket);
      }    

      // close the connection after sending the page- keep aust beautiful
      close(connectionSocket);
      
      numberServed++;
   } 
   
   // close the server connection after we are done- keep aust beautiful
   printf ("** shutting down the server **\n");
   close (serverSocket);
   
   return EXIT_SUCCESS; 
}

int escapeSteps (double x, double y){
    int n=0;
    double zx=0,zy=0,new_zx=0;
    while ((zx*zx + zy*zy < 4.0) && (n < 256)) {
        new_zx = zx*zx - zy*zy + x;
        zy = 2.0*zx*zy + y;
        zx = new_zx;
        n++;
    }
    return n;
}

void serveHTML (int socket) {
   char* message;
 
   // first send the http response header
   message =
      "HTTP/1.0 200 Found\n"
      "Content-Type: text/html\n"
      "\n";
   printf ("about to send=> %s\n", message);
   int re = write (socket, message, strlen (message));

   message =
      "<!DOCTYPE html>\n"
      "<script src=\"https://almondbread.cse.unsw.edu.au/tiles.js\"></script>"
      "\n";      
   re = write (socket, message, strlen (message));
   printf("%d",re);
}


void serveBMP (int socket,double centerX,double centerY,double z) {
   int re=1;
   char* message;
   
   // first send the http response header
   
   // (if you write stings one after another like this on separate
   // lines the c compiler kindly joins them togther for you into
   // one long string)
   message = "HTTP/1.0 200 OK\r\n"
            "Content-Type: image/bmp\r\n"
            "\r\n";
   printf ("About to send: %s\n", message);
   re=write (socket, message, strlen (message));
   
   
   unsigned char head[] = {
     0x42,0x4d,0x36,0x00,0x0C,0x00,0x00,0x00,0x00,
     0x00,0x36,0x00,0x00,0x00,0x28,0x00,0x00,0x00,
     0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x01,
     0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x0C,0x00,0x13,0x0B,0x00,0x00,0x13,0x0B,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

   unsigned char result[SIZE*SIZE];
   unsigned char colored[SIZE*SIZE*3];
   gen(centerX,centerY,z,result);

   int y=0,x=0,t=0;
   for (y=0;y<SIZE;y++){
      for(x=0;x<SIZE;x++){
        colored[t]=stepsToRed(result[y*SIZE+x]);
        colored[t+1]=stepsToBlue(result[y*SIZE+x]);
        colored[t+2]=stepsToGreen(result[y*SIZE+x]);
        t+=3;
      }
   }

   //Write the head of BMP
   re=write (socket, head, sizeof(head));
   //Write the content of BMP
   re=write (socket, colored, sizeof(colored)); 
   printf("%d",re);
}

void gen(double centerX,double centerY,unsigned char zoom,unsigned char* result){
   long double x,y;  
   long double interval=1;
   long double width=0,height=0;
   long i=0;
   
   interval=(int)pow(2.0,(float)interval);   

   interval=1/interval;
   width=511*interval;
   height=511*interval;   
   
   i=0;
   for(y=centerY-width/2-interval;y<centerY+width/2;y+=interval){
      for(x=centerX-height/2-interval;x<centerX+height/2;x+=interval){
         long re=escapeSteps(x,y);
         re=re%256;
          //printf("%f,%f=%i\n",x,y,*(result+i));
         *(result+i)=(unsigned char)re;
         i++;
      }
   }
}

char analyzeFileName(char* request,double* x,double* y,double* z){
  char result;   
  char* name=strtok(request,"/");
  name=strtok(NULL,"/");
  name=strtok(name," ");
  name=strtok(name,"_");
  
    if(name[0]!='t'||name[1]!='i'||name[2]!='l'||name[3]!='e'){
      result=0;
    }else{
      char* xc=strtok(NULL,"_");
      char* yc=strtok(NULL,"_");
      char* zc=strtok(NULL,"_");
      zc=strtok(zc,".");

      *x=atof(strtok(xc,"x"));
      *y=atof(strtok(yc,"y"));
      *z=atof(strtok(zc,"z"));
      result=1;
    }
    return result;
}

// start the server listening on the specified port number
int makeServerSocket (int portNumber) { 
   
   // create socket
   int serverSocket = socket (AF_INET, SOCK_STREAM, 0);
   assert (serverSocket >= 0);   
   // error opening socket
   
   // bind socket to listening port
   struct sockaddr_in serverAddress;
   bzero ((char *) &serverAddress, sizeof (serverAddress));
   
   serverAddress.sin_family      = AF_INET;
   serverAddress.sin_addr.s_addr = INADDR_ANY;
   serverAddress.sin_port        = htons (portNumber);
   
   // let the server start immediately after a previous shutdown
   int optionValue = 1;
   setsockopt (
      serverSocket,
      SOL_SOCKET,
      SO_REUSEADDR,
      &optionValue, 
      sizeof(int)
   );

   int bindSuccess = 
      bind (
         serverSocket, 
         (struct sockaddr *) &serverAddress,
         sizeof (serverAddress)
      );
   
   assert (bindSuccess >= 0);
   // if this assert fails wait a short while to let the operating 
   // system clear the port before trying again
   
   return serverSocket;
}

// wait for a browser to request a connection,
// returns the socket on which the conversation will take place
int waitForConnection (int serverSocket) {
   // listen for a connection
   const int serverMaxBacklog = 10;
   listen (serverSocket, serverMaxBacklog);
   
   // accept the connection
   struct sockaddr_in clientAddress;
   socklen_t clientLen = sizeof (clientAddress);
   int connectionSocket = 
      accept (
         serverSocket, 
         (struct sockaddr *) &clientAddress, 
         &clientLen
      );
   
   assert (connectionSocket >= 0);
   // error on accept
   
   return (connectionSocket);
}