/*
 * mb_mqueue.cpp
 *
 *  Created on: Jun 25, 2013
 *      Author: adderleym
 */

#include <errno.h>
#include "mb_mqclient.hpp"
#include "MBDebugHelper.h"

namespace NsMessageBroker
{

   MqClient::MqClient()
   :m_sndHandle(-1),
    m_rcvHandle(-1),
    m_send(std::string("")),
    m_recv(std::string(""))
   {
   }

   MqClient::MqClient(const std::string& queue_name_snd, const std::string& queue_name_rcv)
   :m_sndHandle(-1),
    m_rcvHandle(-1),
    m_send(queue_name_snd),
    m_recv(queue_name_rcv)
   {
   }


   MqClient::~MqClient()
   {
      if(m_sndHandle != -1)
      {
          ::mq_close(m_sndHandle);
          m_sndHandle = -1;
      }

      if(m_rcvHandle != -1)
      {
          ::mq_close(m_rcvHandle);
          m_rcvHandle = -1;
      }
   }

   bool MqClient::MqOpen()
   {
     struct mq_attr attributes;
     attributes.mq_maxmsg = MSGQ_MAX_MESSAGES;
     attributes.mq_msgsize = MAX_QUEUE_MSG_SIZE;
     attributes.mq_flags = 0;

     m_sndHandle = ::mq_open(m_send.c_str(), O_WRONLY|O_CREAT, 0666, &attributes);
     m_rcvHandle = ::mq_open(m_recv.c_str(), O_RDONLY|O_CREAT, 0666, &attributes);

     return ((m_sndHandle != -1) && (m_rcvHandle != -1)) ? true : false;
   }

   ssize_t MqClient::Send(const std::string& data)
   {
     std::string rep = data;
     int retVal = 0;
     const char* ptrBuffer = rep.c_str();
     static int i=1;

     if (rep.empty())
     {
       return -1;
     }

     if (rep.length() > MAX_QUEUE_MSG_SIZE)
     {
       int retVal = 0;
       std::string shm_data="";

       if(i%2)
       {
         i = 0;
         shm_data="SDL_TO_APPLINK_SHM_1";
         int len = rep.length();
         ptr2->size = len;
         memset(ptr2->text, 0, sizeof(ptr2->size));
         memcpy(ptr2->text, rep.c_str(), len); /* write to the shared memory */
         DBG_MSG(("Written to shared memmory:Buffer %s\n", ptrBuffer));
       }
       else
       {
         i = 1;
         shm_data="SDL_TO_APPLINK_SHM_2";
         int len = rep.length();
         ptr3->size = len;
         memset(ptr3->text, 0, sizeof(ptr3->size));
         memcpy(ptr3->text, rep.c_str(), len); /* write to the shared memory */
         DBG_MSG(("Written to shared memmory:Buffer %s\n", ptrBuffer));
       }

       retVal = ::mq_send(m_sndHandle, &shm_data[0], shm_data.length(), 0);
       if(retVal == -1) {
         DBG_MSG(("Error mq_send %s, id of error = %d\n", strerror(errno), errno));
       }
       return (retVal == -1) ? retVal :(shm_data.length());
     }
     else
     {
       int retVal = ::mq_send(m_sndHandle, ptrBuffer, rep.length(), 0);
       if(retVal == -1) {
         DBG_MSG(("Error mq_send %s, id of error = %d\n", strerror(errno), errno));
       } else {
         DBG_MSG(("MqClient::Send:Buffer %s\n", ptrBuffer));
       }
       return (retVal == -1) ? retVal : rep.length();
     }
   }

   ssize_t MqClient::Recv(std::string& data)
   {
     char buf[MAX_QUEUE_MSG_SIZE] = {'\0'};
     ssize_t length=0;

     if ((length = ::mq_receive(m_rcvHandle, buf, sizeof(buf), 0)) == -1)
     {
       return -1;
     }

     if (strcmp(buf+1,"SHARED_MEMORY") != 0)
     {
       data = std::string(&buf[1], length);
       DBG_MSG(("MqClient::Received Data at mq_client %s",data.c_str()));
       return length;
     } else {
       DBG_MSG(("Shared Memory::Received Length %d",ptr->size));
       data = std::string(ptr->text  ,ptr->size);
       length = ptr->size;

       return length;
     }
   }

} /* namespace NsMessageBroker */
