#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <algorithm>

using namespace Sync;

// This thread handles the socket operations
class SocketThread : public Thread
{
private:
   int endThread; 

   // reference to our connected sockets
   Socket &socket;
   Socket &connectedSocket;

public:
   SocketThread(Socket &socket1, Socket &socket2, bool &endThread)
       : socket(socket1), connectedSocket(socket2), endThread(endThread) {}

   ~SocketThread()
   {
   }

   Socket &GetSocket()
   {
      return socket;
   }

   virtual long ThreadMain()
   {
      ByteArray data;
      while ( !endThread )
      {
         try
         {
            // read from socket
            if ( socket.Read(data) == 0 ) {
               // socket is closed, shutdown thread
               std::cout << "Socket closed on READ" << std::endl;
               break; 
            };

            // send to other
            if ( connectedSocket.Write(data) == 0 ) {
               // socket is closed, shutdown thread
               std::cout << "Socket closed on WRITE" << std::endl;
               break; 
            }
         }
         catch (...)
         {
            // unknown error occurred, shutting down thread.    
            break;
         }
      }

      std::cout << "Shutting down thread" << std::endl;

      // flag to partner that the chat should end
      endThread = true; 

      // ensure both sockets are closed to trigger the clients apps to close
      socket.Close(); 
      connectedSocket.Close(); 

      return 0;
   }
};

// handles server operations
class ServerThread : public Thread
{
private:
   SocketServer &server;
   std::vector<SocketThread *> socketThreads;
   bool terminate = false;

public:
   ServerThread(SocketServer &server, std::vector<SocketThread *> socketThreads)
       : server(server), socketThreads(socketThreads)
   {
   }

   ~ServerThread()
   {
      for (SocketThread *thread : socketThreads)
      {
         try
         {
            // Close the sockets
            Socket &toClose = thread->GetSocket();
            std::cout << "Closing Client Socket - " << toClose.GetFD() << "...\n" ;
            toClose.Close();
            std::cout << "Deleting Thread \n";
            delete thread;
         }
         catch (...)
         {
            terminate = true;
         }
      }

      terminate = true;
   }

   virtual long ThreadMain()
   {
      while (!terminate)
      {
         try
         {
            std::cout << "Server awaiting connections" << std::endl;
            // Wait for client one socket connection
            Socket *connectionOne = new Socket(server.Accept());
            Socket &socketReferenceOne = *connectionOne;
            socketReferenceOne.Write(ByteArray("Success!"));
            std::cout << "1st Connection received. Awaiting second connection." << std::endl;

            // Wait for client two socket connection
            Socket *connectionTwo = new Socket(server.Accept());
            Socket &socketReferenceTwo = *connectionTwo;
            socketReferenceTwo.Write(ByteArray("Success!\n"));
            std::cout << "2nd Connection received. Initializing chat." << std::endl;

            // send both clients connected
            socketReferenceOne.Write(ByteArray("Connected."));
            sleep(1);
            socketReferenceTwo.Write(ByteArray("Connected.\n"));

            // Pass a reference to this pointer into a new socket thread
            socketThreads.push_back(new SocketThread(socketReferenceOne, socketReferenceTwo, terminate));
            socketThreads.push_back(new SocketThread(socketReferenceTwo, socketReferenceOne, terminate));
         }
         catch (...)
         {
            terminate = true;
         }
      }
      return 1;
   }
};

int main(void)
{
   std::cout << "SE3313 Chat Room Server" << std::endl;
   std::cout << "Press enter to terminate the server...";
   std::cout.flush();

   // Create our server
   SocketServer server(3000);
   std::vector<SocketThread *> socketThreads;

   // Need a thread to perform server operations
   ServerThread serverThread(server, socketThreads);

   // This will wait for input to shutdown the server
   FlexWait cinWaiter(1, stdin);
   cinWaiter.Wait();

   // Shut down and clean up the server
   server.Shutdown();
   std::cout << "Server Exiting..." << std::endl;
}
