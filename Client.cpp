#include "thread.h"
#include "socket.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

using namespace Sync;

std::string name;
std::vector<std::string> clientHistory;

int chatEnded = false; 

// refreshes  chat with clint history of messages
void refreshMessages()
{
   system("clear");
   std::cout << "------------ CHAT ROOM HISTORY -----------------\n";
   for (std::string line : clientHistory)
   {
      std::cout << line + "\n";
   }
   std::cout << "\n----------------------------------------------\n";
   std::cout << "Message: (or done)\n";
   std::cout << std::flush;
}

class ReadThread : public Thread
{
private:
   // socket
   Socket *socket;

public:
   // constructor - initalize socket
   ReadThread(Socket *socketInput)
       : socket(socketInput)
   {
   }

   // deconstructor - delete is handled in main function
   ~ReadThread() {
      // indicate the chat has ended
      chatEnded = true; 

   }

   // main thread method
   virtual long ThreadMain()
   {
      // response variable for socket
      ByteArray response;

      // loop until exited
      while ( ! chatEnded )
      {
         if (socket->Read(response) == 0)
         {
            std::cout << "\nConnection lost, press enter to close chat.";
            break;
         }
         std::string responseString = response.ToString();

         if (responseString == "done")
         {
            std::cout << "\nPartner ended chat, press enter to close chat.";
            break;
         }
         // add responseString to local histroy and refresh
         clientHistory.push_back(responseString);
         refreshMessages();
      }
      // indicate the chat has ended
      chatEnded = true; 

      return 1;
   }
};

int main(void)
{
   // Welcome the user
   std::cout << "SE3313 Chat Room Client" << std::endl;

   // Create our socket
   Socket socket("34.220.178.108", 3000);

   // Initially we need a connection
   while (true)
   {
      try
      {
         std::cout << "Connecting...";
         std::cout.flush();
         socket.Open();
         std::cout << "OK\n"<< std::endl;
         break;
      }
      catch (...)
      {
      }

      // Try again every 5 seconds
      std::cout << "Trying again in 5 seconds" << std::endl;
      sleep(5);
   }

   std::cout << "Enter Name: ";
   std::getline(std::cin, name);

   // start read thread for client
   ReadThread *readThread = new ReadThread(&socket);

   // continuely ask for input
   std::string input;
   
   // Send first message to indicate who is connected 
   if ( socket.Write(name + " is connected!") == 0) {
      chatEnded = true; 
   }
   sleep(1);

   // continue to prompt the user for next message and send to partner         
   while ( !chatEnded )
   {
      // get input
      std::getline(std::cin, input);
 
      // check to see if the chat session has ended
      if ( chatEnded ) {
         break;
      } 
      // check is user is asking for chat session to end 
      else if (input == "done")
      {
         socket.Write(input);
         std::cout << "\n Ending chat session...\n";
         break;
      } 
      else
      {
         // send input to server as user:message
         if (socket.Write(name + ": " + input) == 0)
         {
            std::cout << "Connection lost, closing chat.\n";
            break;
         }
         clientHistory.push_back("You: " + input);
         refreshMessages();
      }

   }

   // close socket
   socket.Close();
   delete readThread;
   std::cout << "\nChat Ended.\n";
   return 0;
}