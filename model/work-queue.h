#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include "ns3/system-mutex.h"
#include <queue>

namespace ns3{

template <class T>
class WorkQueue
{

public:
  WorkQueue() {}

  bool IsEmpty ()
  {
    CriticalSection cs(m_mutex);
    bool result = m_storage.empty();
    return result;
  }

  bool TryGetWork (T& item)
  {
    CriticalSection cs(m_mutex);
    if(m_storage.empty())
      return false;
    
    item = m_storage.front();
    m_storage.pop();
    return true;
  }

  void PutWork(const T& item)
  {
    CriticalSection cs(m_mutex);
    m_storage.push(item);
    return;
  }

  void Clear()
  {
    CriticalSection cs(m_mutex);
    m_storage.clear();
  }
  

private:
  SystemMutex    m_mutex;
  std::queue<T>  m_storage;
};
  
}

#endif
