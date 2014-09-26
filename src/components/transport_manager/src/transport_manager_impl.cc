/*
 * Copyright (c) 2014, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "transport_manager/transport_manager_impl.h"

#include <pthread.h>
#include <stdint.h>
#include <cstring>
#include <queue>
#include <set>
#include <algorithm>
#include <limits>
#include <functional>
#include <sstream>

#include "utils/macro.h"
#include "utils/logger.h"
#include "transport_manager/common.h"
#include "transport_manager/transport_manager_listener.h"
#include "transport_manager/transport_manager_listener_empty.h"
#include "transport_manager/transport_adapter/transport_adapter.h"
#include "transport_manager/transport_adapter/transport_adapter_event.h"
#include "config_profile/profile.h"

using ::transport_manager::transport_adapter::TransportAdapter;

namespace transport_manager {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

TransportManagerImpl::Connection TransportManagerImpl::convert(
  const TransportManagerImpl::ConnectionInternal& p) {
  LOG4CXX_TRACE(logger_, "enter. ConnectionInternal: " << &p);
  TransportManagerImpl::Connection c;
  c.application = p.application;
  c.device = p.device;
  c.id = p.id;
  LOG4CXX_TRACE(logger_, "exit with TransportManagerImpl::Connection. It's ConnectionUID = "
                << c.id);
  return c;
}

TransportManagerImpl::TransportManagerImpl()
  : message_queue_mutex_(),
    all_thread_active_(false),
    message_queue_thread_(),
    event_queue_thread_(),
    device_listener_thread_wakeup_(),
    is_initialized_(false),
#ifdef TIME_TESTER
    metric_observer_(NULL),
#endif  // TIME_TESTER
    connection_id_counter_(0) {
  LOG4CXX_INFO(logger_, "==============================================");
  pthread_mutex_init(&message_queue_mutex_, NULL);
  pthread_cond_init(&message_queue_cond_, NULL);
  pthread_mutex_init(&event_queue_mutex_, 0);
  pthread_cond_init(&device_listener_thread_wakeup_, NULL);
  LOG4CXX_DEBUG(logger_, "TransportManager object created");
}

TransportManagerImpl::~TransportManagerImpl() {
  LOG4CXX_DEBUG(logger_, "TransportManager object destroying");

  for (std::vector<TransportAdapter*>::iterator it =
         transport_adapters_.begin();
       it != transport_adapters_.end(); ++it) {
    delete *it;
  }

  for (std::map<TransportAdapter*, TransportAdapterListenerImpl*>::iterator it =
         transport_adapter_listeners_.begin();
       it != transport_adapter_listeners_.end(); ++it) {
    delete it->second;
  }

  pthread_mutex_destroy(&message_queue_mutex_);
  pthread_cond_destroy(&message_queue_cond_);
  pthread_mutex_destroy(&event_queue_mutex_);
  pthread_cond_destroy(&device_listener_thread_wakeup_);
  LOG4CXX_INFO(logger_, "TransportManager object destroyed");
}

int TransportManagerImpl::ConnectDevice(const DeviceHandle& device_handle) {
  LOG4CXX_TRACE(logger_, "enter. DeviceHandle: " << &device_handle);
  if (!this->is_initialized_) {
    LOG4CXX_ERROR(logger_, "TransportManager is not initialized.");
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: !this->is_initialized_");
    return E_TM_IS_NOT_INITIALIZED;
  }

  DeviceUID device_id = converter_.HandleToUid(device_handle);
  LOG4CXX_DEBUG(logger_, "Convert handle to id " << device_id);

  transport_adapter::TransportAdapter* ta = device_to_adapter_map_[device_id];
  if (NULL == ta) {
    LOG4CXX_ERROR(logger_, "No device adapter found by id " << device_id);
    LOG4CXX_TRACE(logger_, "exit with E_INVALID_HANDLE. Condition: NULL == ta");
    return E_INVALID_HANDLE;
  }

  TransportAdapter::Error ta_error = ta->ConnectDevice(device_id);
  int err = (TransportAdapter::OK == ta_error) ? E_SUCCESS : E_INTERNAL_ERROR;
  LOG4CXX_TRACE(logger_, "exit with error: " << err);
  return err;
}

int TransportManagerImpl::DisconnectDevice(const DeviceHandle& device_handle) {
  LOG4CXX_TRACE(logger_, "enter. DeviceHandle: " << &device_handle);
  if (!this->is_initialized_) {
    LOG4CXX_ERROR(logger_, "TransportManager is not initialized.");
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: !this->is_initialized_");
    return E_TM_IS_NOT_INITIALIZED;
  }
  DeviceUID device_id = converter_.HandleToUid(device_handle);
  LOG4CXX_DEBUG(logger_, "Convert handle to id" << device_id);

  transport_adapter::TransportAdapter* ta = device_to_adapter_map_[device_id];
  if (NULL == ta) {
    LOG4CXX_WARN(logger_, "No device adapter found by id " << device_id);
    LOG4CXX_TRACE(logger_, "exit with E_INVALID_HANDLE. Condition: NULL == ta");
    return E_INVALID_HANDLE;
  }
  ta->DisconnectDevice(device_id);
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::Disconnect(const ConnectionUID& cid) {
  LOG4CXX_TRACE(logger_, "enter. ConnectionUID: " << &cid);
  if (!this->is_initialized_) {
    LOG4CXX_ERROR(logger_, "TransportManager is not initialized.");
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: !this->is_initialized_");
    return E_TM_IS_NOT_INITIALIZED;
  }

  ConnectionInternal* connection = GetConnection(cid);
  if (NULL == connection) {
    LOG4CXX_ERROR(logger_, "TransportManagerImpl::Disconnect: Connection does not exist.");
    LOG4CXX_TRACE(logger_, "exit with E_INVALID_HANDLE. Condition: NULL == connection");
    return E_INVALID_HANDLE;
  }

  pthread_mutex_lock(&event_queue_mutex_);
  int messages_count = 0;
  for (EventQueue::const_iterator it = event_queue_.begin();
       it != event_queue_.end();
       ++it) {
    if (it->application_id == static_cast<ApplicationHandle>(cid)) {
      ++messages_count;
    }
  }
  pthread_mutex_unlock(&event_queue_mutex_);

  if (messages_count > 0) {
    connection->messages_count = messages_count;
    connection->shutDown = true;

    const uint32_t disconnect_timeout =
      profile::Profile::instance()->transport_manager_disconnect_timeout();
    if (disconnect_timeout > 0) {
      connection->timer->start(disconnect_timeout);
    }
  } else {
    connection->transport_adapter->Disconnect(connection->device,
        connection->application);
  }
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::DisconnectForce(const ConnectionUID& cid) {
  LOG4CXX_TRACE(logger_, "enter ConnectionUID: " << &cid);
  if (false == this->is_initialized_) {
    LOG4CXX_ERROR(logger_, "TransportManager is not initialized.");
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: false == this->is_initialized_");
    return E_TM_IS_NOT_INITIALIZED;
  }
  pthread_mutex_lock(&event_queue_mutex_);
  // Clear messages for this connection
  // Note that MessageQueue typedef is assumed to be std::list,
  // or there is a problem here. One more point versus typedefs-everywhere
  MessageQueue::iterator e = message_queue_.begin();
  while (e != message_queue_.end()) {
    if (static_cast<ConnectionUID>((*e)->connection_key()) == cid) {
      RaiseEvent(&TransportManagerListener::OnTMMessageSendFailed,
                 DataSendTimeoutError(), *e);
      e = message_queue_.erase(e);
    } else {
      ++e;
    }
  }
  pthread_mutex_unlock(&event_queue_mutex_);
  const ConnectionInternal* connection = GetConnection(cid);
  if (NULL == connection) {
    LOG4CXX_ERROR(
      logger_,
      "TransportManagerImpl::DisconnectForce: Connection does not exist.");
    LOG4CXX_TRACE(logger_, "exit with E_INVALID_HANDLE. Condition: NULL == connection");
    return E_INVALID_HANDLE;
  }
  connection->transport_adapter->Disconnect(connection->device,
      connection->application);
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::AddEventListener(TransportManagerListener* listener) {
  LOG4CXX_TRACE(logger_, "enter. TransportManagerListener: " << listener);
  transport_manager_listener_.push_back(listener);
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::Stop() {
  LOG4CXX_TRACE(logger_, "enter");
  if (!all_thread_active_) {
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: !all_thread_active_");
    return E_TM_IS_NOT_INITIALIZED;
  }
  all_thread_active_ = false;

  pthread_mutex_lock(&event_queue_mutex_);
  pthread_cond_signal(&device_listener_thread_wakeup_);
  pthread_mutex_unlock(&event_queue_mutex_);

  pthread_mutex_lock(&message_queue_mutex_);
  pthread_cond_signal(&message_queue_cond_);
  pthread_mutex_unlock(&message_queue_mutex_);

  pthread_join(message_queue_thread_, 0);
  pthread_join(event_queue_thread_, 0);

  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::SendMessageToDevice(const RawMessagePtr message) {
  LOG4CXX_TRACE(logger_, "enter. RawMessageSptr: " << message);
  LOG4CXX_INFO(logger_, "Send message to device called with arguments "
               << message.get());
  if (false == this->is_initialized_) {
    LOG4CXX_ERROR(logger_, "TM is not initialized.");
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: false == this->is_initialized_");
    return E_TM_IS_NOT_INITIALIZED;
  }

  const ConnectionInternal* connection =
    GetConnection(message->connection_key());
  if (NULL == connection) {
    LOG4CXX_ERROR(logger_, "Connection with id " << message->connection_key()
                  << " does not exist.");
    LOG4CXX_TRACE(logger_, "exit with E_INVALID_HANDLE. Condition: NULL == connection");
    return E_INVALID_HANDLE;
  }

  if (connection->shutDown) {
    LOG4CXX_ERROR(logger_, "TransportManagerImpl::Disconnect: Connection is to shut down.");
    LOG4CXX_TRACE(logger_,
                  "exit with E_CONNECTION_IS_TO_SHUTDOWN. Condition: connection->shutDown");
    return E_CONNECTION_IS_TO_SHUTDOWN;
  }
#ifdef TIME_TESTER
  if (metric_observer_) {
    metric_observer_->StartRawMsg(message.get());
  }
#endif  // TIME_TESTER
  this->PostMessage(message);
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::ReceiveEventFromDevice(const TransportAdapterEvent& event) {
  LOG4CXX_TRACE(logger_, "enter. TransportAdapterEvent: " << &event);
  if (!is_initialized_) {
    LOG4CXX_ERROR(logger_, "TM is not initialized.");
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: false == this->is_initialized_");
    return E_TM_IS_NOT_INITIALIZED;
  }
  this->PostEvent(event);
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::RemoveDevice(const DeviceHandle& device_handle) {
  LOG4CXX_TRACE(logger_, "enter. DeviceHandle: " << &device_handle);
  DeviceUID device_id = converter_.HandleToUid(device_handle);
  if (false == this->is_initialized_) {
    LOG4CXX_ERROR(logger_, "TM is not initialized.");
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: false == this->is_initialized_");
    return E_TM_IS_NOT_INITIALIZED;
  }
  device_to_adapter_map_.erase(device_id);
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::AddTransportAdapter(
  transport_adapter::TransportAdapter* transport_adapter) {
  LOG4CXX_TRACE(logger_, "enter. TransportAdapter: " << transport_adapter);

  if (transport_adapter_listeners_.find(transport_adapter) !=
      transport_adapter_listeners_.end()) {
    LOG4CXX_ERROR(logger_, "Adapter already exists.");
    LOG4CXX_TRACE(logger_,
                  "exit with E_ADAPTER_EXISTS. Condition: transport_adapter_listeners_.find(transport_adapter) != transport_adapter_listeners_.end()");
    return E_ADAPTER_EXISTS;
  }
  transport_adapter_listeners_[transport_adapter] =
    new TransportAdapterListenerImpl(this, transport_adapter);
  transport_adapter->AddListener(
    transport_adapter_listeners_[transport_adapter]);

  if (transport_adapter->IsInitialised() ||
      transport_adapter->Init() == TransportAdapter::OK) {
    transport_adapters_.push_back(transport_adapter);
  }
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::SearchDevices() {
  LOG4CXX_TRACE(logger_, "enter");
  if (!this->is_initialized_) {
    LOG4CXX_ERROR(logger_, "TM is not initialized");
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: !this->is_initialized_");
    return E_TM_IS_NOT_INITIALIZED;
  }

  LOG4CXX_INFO(logger_, "Search device called");

  bool success_occurred = false;

  for (std::vector<TransportAdapter*>::iterator it =
         transport_adapters_.begin();
       it != transport_adapters_.end(); ++it) {
    LOG4CXX_DEBUG(logger_, "Iterating over transport adapters");
    TransportAdapter::Error scanResult = (*it)->SearchDevices();
    if (transport_adapter::TransportAdapter::OK == scanResult) {
      success_occurred = true;
    } else {
      LOG4CXX_ERROR(logger_, "Transport Adapter search failed "
                    << *it << "[" << (*it)->GetDeviceType()
                    << "]");
      switch (scanResult) {
        case transport_adapter::TransportAdapter::NOT_SUPPORTED: {
          LOG4CXX_ERROR(logger_, "Search feature is not supported "
                        << *it << "[" << (*it)->GetDeviceType()
                        << "]");
          LOG4CXX_DEBUG(logger_, "scanResult = TransportAdapter::NOT_SUPPORTED");
          break;
        }
        case transport_adapter::TransportAdapter::BAD_STATE: {
          LOG4CXX_ERROR(logger_, "Transport Adapter has bad state " << *it << "[" <<
                        (*it)->GetDeviceType()
                        << "]");
          LOG4CXX_DEBUG(logger_, "scanResult = TransportAdapter::BAD_STATE");
          break;
        }
        default: {
          LOG4CXX_ERROR(logger_, "Invalid scan result");
          LOG4CXX_DEBUG(logger_, "scanResult = default switch case");
          return E_ADAPTERS_FAIL;
        }
      }
    }
  }
  int transport_adapter_search = (success_occurred || transport_adapters_.empty())
                                 ? E_SUCCESS
                                 : E_ADAPTERS_FAIL;
  if (transport_adapter_search == E_SUCCESS) {
    LOG4CXX_TRACE(logger_,
                  "exit with E_SUCCESS. Condition: success_occured || transport_adapters_.empty()");
  } else {
    LOG4CXX_TRACE(logger_,
                  "exit with E_ADAPTERS_FAIL. Condition: success_occured || transport_adapters_.empty()");
  }
  return transport_adapter_search;
}

int TransportManagerImpl::Init() {
  LOG4CXX_TRACE(logger_, "enter");
  all_thread_active_ = true;

  int error_code = pthread_create(&message_queue_thread_, 0,
                                  &MessageQueueStartThread, this);
  if (0 != error_code) {
    LOG4CXX_ERROR(logger_,
                  "Message queue thread is not created exit with error code "
                  << error_code);
    LOG4CXX_TRACE(logger_, "exit with E_TM_IS_NOT_INITIALIZED. Condition: 0 != error_code");
    return E_TM_IS_NOT_INITIALIZED;
  }
  pthread_setname_np(message_queue_thread_, "TM MessageQueue");

  error_code = pthread_create(&event_queue_thread_, 0,
                              &EventListenerStartThread, this);
  if (0 != error_code) {
    LOG4CXX_ERROR(logger_,
                  "Event queue thread is not created exit with error code "
                  << error_code);
    LOG4CXX_TRACE(logger_, "exit with E_TM_IS_NOT_INITIALIZED. Condition: 0 != error_code");
    return E_TM_IS_NOT_INITIALIZED;
  }
  pthread_setname_np(event_queue_thread_, "TM EventListener");

  is_initialized_ = true;
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

int TransportManagerImpl::Visibility(const bool& on_off) const {
  LOG4CXX_TRACE(logger_, "enter. On_off: " << &on_off);
  TransportAdapter::Error ret;

  LOG4CXX_DEBUG(logger_, "Visibility change requested to " << on_off);
  if (false == this->is_initialized_) {
    LOG4CXX_ERROR(logger_, "TM is not initialized");
    LOG4CXX_TRACE(logger_,
                  "exit with E_TM_IS_NOT_INITIALIZED. Condition: false == this->is_initialized_");
    return E_TM_IS_NOT_INITIALIZED;
  }

  for (std::vector<TransportAdapter*>::const_iterator it =
         transport_adapters_.begin();
       it != transport_adapters_.end(); ++it) {
    if (on_off) {
      ret = (*it)->StartClientListening();
    } else {
      ret = (*it)->StopClientListening();
    }
    if (TransportAdapter::Error::NOT_SUPPORTED == ret) {
      LOG4CXX_DEBUG(logger_, "Visibility change is not supported for adapter "
                    << *it << "[" << (*it)->GetDeviceType() << "]");
    }
  }
  LOG4CXX_TRACE(logger_, "exit with E_SUCCESS");
  return E_SUCCESS;
}

void TransportManagerImpl::UpdateDeviceList(TransportAdapter* ta) {
  LOG4CXX_TRACE(logger_, "enter. TransportAdapter: " << ta);
  std::set<DeviceInfo> old_devices;
  for (DeviceInfoList::iterator it = device_list_.begin();
       it != device_list_.end();) {
    if (it->first == ta) {
      old_devices.insert(it->second);
      it = device_list_.erase(it);
    } else {
      ++it;
    }
  }

  std::set<DeviceInfo> new_devices;
  const DeviceList dev_list = ta->GetDeviceList();
  for (DeviceList::const_iterator it = dev_list.begin();
       it != dev_list.end(); ++it) {
    DeviceHandle device_handle = converter_.UidToHandle(*it);
    DeviceInfo info(device_handle, *it, ta->DeviceName(*it), ta->GetConnectionType());
    device_list_.push_back(std::make_pair(ta, info));
    new_devices.insert(info);
  }

  std::set<DeviceInfo> added_devices;
  std::set_difference(new_devices.begin(), new_devices.end(),
                      old_devices.begin(), old_devices.end(),
                      std::inserter(added_devices, added_devices.begin()));
  for (std::set<DeviceInfo>::const_iterator it = added_devices.begin();
       it != added_devices.end();
       ++it) {
    RaiseEvent(&TransportManagerListener::OnDeviceAdded, *it);
  }

  std::set<DeviceInfo> removed_devices;
  std::set_difference(old_devices.begin(), old_devices.end(),
                      new_devices.begin(), new_devices.end(),
                      std::inserter(removed_devices, removed_devices.begin()));

  for (std::set<DeviceInfo>::const_iterator it = removed_devices.begin();
       it != removed_devices.end();
       ++it) {
    RaiseEvent(&TransportManagerListener::OnDeviceRemoved, *it);
  }
  LOG4CXX_TRACE(logger_, "exit");
}

void TransportManagerImpl::PostMessage(const RawMessagePtr message) {
  LOG4CXX_TRACE(logger_, "enter. RawMessageSptr: " << message);
  pthread_mutex_lock(&message_queue_mutex_);
  message_queue_.push_back(message);
  pthread_cond_signal(&message_queue_cond_);
  pthread_mutex_unlock(&message_queue_mutex_);
  LOG4CXX_TRACE(logger_, "exit");
}

void TransportManagerImpl::RemoveMessage(const RawMessagePtr message) {
  // TODO: Reconsider necessity of this method, remove if it's useless,
  //       make to work otherwise.
  //       2013-08-21 dchmerev@luxoft.com
  LOG4CXX_TRACE(logger_, "enter RawMessageSptr: " << message);
  pthread_mutex_lock(&message_queue_mutex_);
  std::remove(message_queue_.begin(), message_queue_.end(), message);
  pthread_mutex_unlock(&message_queue_mutex_);
  LOG4CXX_TRACE(logger_, "exit");
}

void TransportManagerImpl::PostEvent(const TransportAdapterEvent& event) {
  LOG4CXX_TRACE(logger_, "enter. TransportAdapterEvent: " << &event);
  pthread_mutex_lock(&event_queue_mutex_);
  event_queue_.push_back(event);
  pthread_cond_signal(&device_listener_thread_wakeup_);
  pthread_mutex_unlock(&event_queue_mutex_);
  LOG4CXX_TRACE(logger_, "exit");
}

void* TransportManagerImpl::EventListenerStartThread(void* data) {
  LOG4CXX_TRACE(logger_, "enter Data: " << data);
  if (NULL != data) {
    static_cast<TransportManagerImpl*>(data)->EventListenerThread();
  }
  LOG4CXX_TRACE(logger_, "exit with 0");
  return 0;
}

void TransportManagerImpl::AddConnection(const ConnectionInternal& c) {
  LOG4CXX_TRACE(logger_, "enter ConnectionInternal: " << &c);
  connections_.push_back(c);
  LOG4CXX_TRACE(logger_, "exit");
}

void TransportManagerImpl::RemoveConnection(uint32_t id) {
  LOG4CXX_TRACE(logger_, "enter Id: " << id);
  for (std::vector<ConnectionInternal>::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    if (it->id == id) {
      connections_.erase(it);
      break;
    }
  }
  LOG4CXX_TRACE(logger_, "exit");
}

TransportManagerImpl::ConnectionInternal* TransportManagerImpl::GetConnection(
  const ConnectionUID& id) {
  LOG4CXX_TRACE(logger_, "enter. ConnectionUID: " << &id);
  for (std::vector<ConnectionInternal>::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    if (it->id == id) {
      LOG4CXX_TRACE(logger_, "exit with ConnectionInternal. It's address: " << &*it);
      return &*it;
    }
  }
  LOG4CXX_TRACE(logger_, "exit with NULL");
  return NULL;
}

TransportManagerImpl::ConnectionInternal* TransportManagerImpl::GetConnection(
  const DeviceUID& device, const ApplicationHandle& application) {
  LOG4CXX_TRACE(logger_, "enter DeviceUID: " << &device << "ApplicationHandle: " <<
                &application);
  for (std::vector<ConnectionInternal>::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    if (it->device == device && it->application == application) {
      LOG4CXX_TRACE(logger_, "exit with ConnectionInternal. It's address: " << &*it);
      return &*it;
    }
  }
  LOG4CXX_TRACE(logger_, "exit with NULL");
  return NULL;
}

void TransportManagerImpl::OnDeviceListUpdated(TransportAdapter* ta) {
  LOG4CXX_TRACE(logger_, "enter. TransportAdapter: " << ta);
  const DeviceList device_list = ta->GetDeviceList();
  LOG4CXX_INFO(logger_, "DEVICE_LIST_UPDATED " << device_list.size());
  for (DeviceList::const_iterator it = device_list.begin();
       it != device_list.end(); ++it) {
    device_to_adapter_map_.insert(std::make_pair(*it, ta));
    DeviceHandle device_handle = converter_.UidToHandle(*it);
    DeviceInfo info(device_handle, *it, ta->DeviceName(*it), ta->GetConnectionType());
    RaiseEvent(&TransportManagerListener::OnDeviceFound, info);
  }
  UpdateDeviceList(ta);
  std::vector<DeviceInfo> device_infos;
  for (DeviceInfoList::const_iterator it = device_list_.begin();
       it != device_list_.end(); ++it) {
    device_infos.push_back(it->second);
  }
  RaiseEvent(&TransportManagerListener::OnDeviceListUpdated, device_infos);
  LOG4CXX_TRACE(logger_, "exit");
}

void TransportManagerImpl::EventListenerThread() {
  LOG4CXX_TRACE(logger_, "enter");
  pthread_mutex_lock(&event_queue_mutex_);
  LOG4CXX_INFO(logger_, "Event listener thread started");
  while (true) {
    while (event_queue_.size() > 0) {
      LOG4CXX_INFO(logger_, "Event listener queue pushed to process events");
      EventQueue::iterator current_it = event_queue_.begin();
      const TransportAdapterEvent event = *(current_it);
      event_queue_.erase(current_it);
      pthread_mutex_unlock(&event_queue_mutex_);
      ConnectionInternal* connection = GetConnection(event.device_uid, event.application_id);

      switch (event.event_type) {
        case TransportAdapterListenerImpl::EventTypeEnum::ON_SEARCH_DONE: {
          RaiseEvent(&TransportManagerListener::OnScanDevicesFinished);
          LOG4CXX_DEBUG(logger_, "event_type = ON_SEARCH_DONE");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_SEARCH_FAIL: {
          // error happened in real search process (external error)
          RaiseEvent(&TransportManagerListener::OnScanDevicesFailed,
                     *static_cast<SearchDeviceError*>(event.event_error.get()));
          LOG4CXX_DEBUG(logger_, "event_type = ON_SEARCH_FAIL");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_DEVICE_LIST_UPDATED: {
          OnDeviceListUpdated(event.transport_adapter);
          LOG4CXX_DEBUG(logger_, "event_type = ON_DEVICE_LIST_UPDATED");
          break;
        }
        case TransportAdapterListenerImpl::ON_FIND_NEW_APPLICATIONS_REQUEST: {
          RaiseEvent(&TransportManagerListener::OnFindNewApplicationsRequest);
          LOG4CXX_DEBUG(logger_, "event_type = ON_FIND_NEW_APPLICATIONS_REQUEST");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_CONNECT_DONE: {
          const DeviceHandle device_handle = converter_.UidToHandle(event.device_uid);
          AddConnection(ConnectionInternal(this, event.transport_adapter, ++connection_id_counter_,
                                           event.device_uid, event.application_id,
                                           device_handle));
          RaiseEvent(&TransportManagerListener::OnConnectionEstablished,
                     DeviceInfo(device_handle, event.device_uid,
                                event.transport_adapter->DeviceName(event.device_uid),
                                event.transport_adapter->GetConnectionType()),
                     connection_id_counter_);
          LOG4CXX_DEBUG(logger_, "event_type = ON_CONNECT_DONE");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_CONNECT_FAIL: {
          RaiseEvent(&TransportManagerListener::OnConnectionFailed,
                     DeviceInfo(converter_.UidToHandle(event.device_uid), event.device_uid,
                                event.transport_adapter->DeviceName(event.device_uid),
                                event.transport_adapter->GetConnectionType()),
                     ConnectError());
          LOG4CXX_DEBUG(logger_, "event_type = ON_CONNECT_FAIL");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_DISCONNECT_DONE: {
          if (NULL == connection) {
            LOG4CXX_ERROR(logger_, "Connection not found");
            LOG4CXX_DEBUG(logger_,
                          "event_type = ON_DISCONNECT_DONE && NULL == connection");
            break;
          }
          RaiseEvent(&TransportManagerListener::OnConnectionClosed,
                     connection->id);
          RemoveConnection(connection->id);
          LOG4CXX_DEBUG(logger_, "event_type = ON_DISCONNECT_DONE");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_DISCONNECT_FAIL: {
          const DeviceHandle device_handle = converter_.UidToHandle(event.device_uid);
          RaiseEvent(&TransportManagerListener::OnDisconnectFailed,
                     device_handle, DisconnectDeviceError());
          LOG4CXX_DEBUG(logger_, "event_type = ON_DISCONNECT_FAIL");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_SEND_DONE: {
#ifdef TIME_TESTER
          if (metric_observer_) {
            metric_observer_->StopRawMsg(event.event_data.get());
          }
#endif  // TIME_TESTER
          if (connection == NULL) {
            LOG4CXX_ERROR(logger_, "Connection ('" << event.device_uid << ", "
                          << event.application_id
                          << ") not found");
            LOG4CXX_DEBUG(logger_, "event_type = ON_SEND_DONE. Condition: NULL == connection");
            break;
          }
          RaiseEvent(&TransportManagerListener::OnTMMessageSend, event.event_data);
          this->RemoveMessage(event.event_data);
          if (connection->shutDown && --connection->messages_count == 0) {
            connection->timer->stop();
            connection->transport_adapter->Disconnect(connection->device,
                connection->application);
          }
          LOG4CXX_DEBUG(logger_, "event_type = ON_SEND_DONE");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_SEND_FAIL: {
#ifdef TIME_TESTER
          if (metric_observer_) {
            metric_observer_->StopRawMsg(event.event_data.get());
          }
#endif  // TIME_TESTER
          if (connection == NULL) {
            LOG4CXX_ERROR(logger_, "Connection ('" << event.device_uid << ", "
                          << event.application_id
                          << ") not found");
            LOG4CXX_DEBUG(logger_, "event_type = ON_SEND_FAIL. Condition: NULL == connection");
            break;
          }

          // TODO(YK): start timer here to wait before notify caller
          // and remove unsent messages
          LOG4CXX_ERROR(logger_, "Transport adapter failed to send data");
          // TODO(YK): potential error case -> thread unsafe
          // update of message content
          if (event.event_data.valid()) {
            event.event_data->set_waiting(true);
          } else {
            LOG4CXX_DEBUG(logger_, "Data is invalid");
          }
          LOG4CXX_DEBUG(logger_, "eevent_type = ON_SEND_FAIL");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_RECEIVED_DONE: {
          if (connection == NULL) {
            LOG4CXX_ERROR(logger_, "Connection ('" << event.device_uid << ", "
                          << event.application_id
                          << ") not found");
            LOG4CXX_DEBUG(logger_, "event_type = ON_RECEIVED_DONE. Condition: NULL == connection");
            break;
          }
          event.event_data->set_connection_key(connection->id);
#ifdef TIME_TESTER
          if (metric_observer_) {
            metric_observer_->StopRawMsg(event.event_data.get());
          }
#endif  // TIME_TESTER
          RaiseEvent(&TransportManagerListener::OnTMMessageReceived, event.event_data);
          LOG4CXX_DEBUG(logger_, "event_type = ON_RECEIVED_DONE");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_RECEIVED_FAIL: {
          LOG4CXX_DEBUG(logger_, "Event ON_RECEIVED_FAIL");
          if (connection == NULL) {
            LOG4CXX_ERROR(logger_, "Connection ('" << event.device_uid << ", "
                          << event.application_id
                          << ") not found");
            break;
          }

          RaiseEvent(&TransportManagerListener::OnTMMessageReceiveFailed,
                     connection->id, *static_cast<DataReceiveError*>(event.event_error.get()));
          LOG4CXX_DEBUG(logger_, "event_type = ON_RECEIVED_FAIL");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_COMMUNICATION_ERROR: {
          LOG4CXX_DEBUG(logger_, "event_type = ON_COMMUNICATION_ERROR");
          break;
        }
        case TransportAdapterListenerImpl::EventTypeEnum::ON_UNEXPECTED_DISCONNECT: {
          if (connection) {
            RaiseEvent(&TransportManagerListener::OnUnexpectedDisconnect,
                       connection->id,
                       *static_cast<CommunicationError*>(event.event_error.get()));
            RemoveConnection(connection->id);
          } else {
            LOG4CXX_ERROR(logger_, "Connection ('" << event.device_uid << ", "
                          << event.application_id
                          << ") not found");
          }
          LOG4CXX_DEBUG(logger_, "eevent_type = ON_UNEXPECTED_DISCONNECT");
          break;
        }
      }  // switch
      pthread_mutex_lock(&event_queue_mutex_);
    }  // while (event_queue_.size() > 0)

    if (all_thread_active_) {
      pthread_cond_wait(&device_listener_thread_wakeup_, &event_queue_mutex_);
    } else {
      LOG4CXX_DEBUG(logger_, "break. Condition: NOT all_thread_active_");
      break;
    }
  }  // while (true)

  pthread_mutex_unlock(&event_queue_mutex_);

  LOG4CXX_TRACE(logger_, "exit");
}

#ifdef TIME_TESTER
void TransportManagerImpl::SetTimeMetricObserver(TMMetricObserver* observer) {
  metric_observer_ = observer;
}
#endif  // TIME_TESTER

void* TransportManagerImpl::MessageQueueStartThread(void* data) {
  LOG4CXX_TRACE(logger_, "enter. Data:" << data);
  if (NULL != data) {
    static_cast<TransportManagerImpl*>(data)->MessageQueueThread();
  }
  LOG4CXX_TRACE(logger_, "exit with 0");
  return 0;
}

void TransportManagerImpl::MessageQueueThread() {
  LOG4CXX_TRACE(logger_, "enter");
  pthread_mutex_lock(&message_queue_mutex_);

  while (all_thread_active_) {
    // TODO(YK): add priority processing

    while (message_queue_.size() > 0) {
      MessageQueue::iterator it = message_queue_.begin();
      while (it != message_queue_.end() && it->valid() && (*it)->IsWaiting()) {
        ++it;
      }
      if (it == message_queue_.end()) {
        LOG4CXX_TRACE(logger_, "break. Condition: it == message_queue_.end()");
        break;
      }
      RawMessagePtr active_msg = *it;
      pthread_mutex_unlock(&message_queue_mutex_);
      if (active_msg.valid() && !active_msg->IsWaiting()) {
        ConnectionInternal* connection =
          GetConnection(active_msg->connection_key());
        if (connection == NULL) {
          std::stringstream ss;
          ss << "Connection " << active_msg->connection_key() << " not found";
          LOG4CXX_ERROR(logger_, ss.str());
          RaiseEvent(&TransportManagerListener::OnTMMessageSendFailed,
                     DataSendError(ss.str()), active_msg);
          message_queue_.remove(active_msg);
          continue;
        }
        TransportAdapter* transport_adapter = connection->transport_adapter;
        LOG4CXX_DEBUG(logger_, "Got adapter "
                      << transport_adapter << "["
                      << transport_adapter->GetDeviceType() << "]"
                      << " by session id "
                      << active_msg->connection_key());

        if (NULL == transport_adapter) {
          std::string error_text =
            "Transport adapter is not found - message removed";
          LOG4CXX_ERROR(logger_, error_text);
          RaiseEvent(&TransportManagerListener::OnTMMessageSendFailed,
                     DataSendError(error_text), active_msg);
          message_queue_.remove(active_msg);
        } else {
          if (TransportAdapter::OK ==
              transport_adapter->SendData(
                connection->device, connection->application, active_msg)) {
            LOG4CXX_INFO(logger_, "Data sent to adapter");
            active_msg->set_waiting(true);
          } else {
            LOG4CXX_ERROR(logger_, "Data sent error");
            RaiseEvent(&TransportManagerListener::OnTMMessageSendFailed,
                       DataSendError("Send failed - message removed"),
                       active_msg);
            message_queue_.remove(active_msg);
          }
        }
      }
      pthread_mutex_lock(&message_queue_mutex_);
    }
    pthread_cond_wait(&message_queue_cond_, &message_queue_mutex_);
  }  //  while(true)

  message_queue_.clear();

  pthread_mutex_unlock(&message_queue_mutex_);
  LOG4CXX_TRACE(logger_, "exit");
}

TransportManagerImpl::ConnectionInternal::ConnectionInternal(
  TransportManagerImpl* transport_manager, TransportAdapter* transport_adapter,
  const ConnectionUID& id, const DeviceUID& dev_id, const ApplicationHandle& app_id,
  const DeviceHandle& device_handle)
  : transport_manager(transport_manager),
    transport_adapter(transport_adapter),
    timer(new TimerInternal("TM DiscRoutine", this, &ConnectionInternal::DisconnectFailedRoutine)),
    shutDown(false),
    device_handle_(device_handle),
    messages_count(0) {
  Connection::id = id;
  Connection::device = dev_id;
  Connection::application = app_id;
}

void TransportManagerImpl::ConnectionInternal::DisconnectFailedRoutine() {
  LOG4CXX_TRACE(logger_, "enter");
  transport_manager->RaiseEvent(&TransportManagerListener::OnDisconnectFailed,
                                device_handle_, DisconnectDeviceError());
  shutDown = false;
  timer->stop();
  LOG4CXX_TRACE(logger_, "exit");
}

}  // namespace transport_manager


