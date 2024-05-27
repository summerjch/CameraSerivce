#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iostream>
#include <cstdio>
#include "shared_memory_buffer.hpp"

using namespace boost::interprocess;

int main ()
{

   //Erase previous shared memory and schedule erasure on exit
   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("OriginalFrame"); }
      ~shm_remove(){ shared_memory_object::remove("OriginalFrame"); }
   } remover;

   //Create a shared memory object.
   shared_memory_object shm
      (create_only               //only create
      ,"OriginalFrame"           //name
      ,read_write                //read-write mode
      );
   try{
      //Set size
      shm.truncate(sizeof(BufferMemory));

      //Map the whole shared memory in this process
      mapped_region region
         (shm                       //What to map
         ,read_write //Map it as read-write
         );

      //Get the address of the mapped region
      void * addr       = region.get_address();

      //Construct the shared structure in memory
      BufferMemory * data = new (addr) BufferMemory;

      const int NumMsg = 100;

      for(int i = 0; i < NumMsg; ++i){
         scoped_lock<interprocess_mutex> lock(data->mutex);
         std::cerr << "Get the lock" << std::endl;
         if(data->message_in){
            std::cerr << "wait for lock" << std::endl;
            data->cond_full.wait(lock);
         }
         if(i == (NumMsg-1))
            std::sprintf(data->items, "%s", "last message");
         else
            std::sprintf(data->items, "%s_%d", "my_trace", i);

         //Notify to the other process that there is a message
         std::cout << data->items << std::endl;
         data->cond_empty.notify_one();

         //Mark message buffer as full
         data->message_in = true;
      }
   }
   catch(interprocess_exception &ex){
      std::cout << ex.what() << std::endl;
      return 1;
   }

   return 0;
}