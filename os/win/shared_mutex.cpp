#include <Windows.h>
HANDLE global_mutex=NULL;

void init_process_mutex() {
  global_mutex = OpenMutexA(MUTEX_ALL_ACCESS,FALSE,"upgrade_tool_mutex");
  if (global_mutex==NULL)
    global_mutex = CreateMutexA(NULL,FALSE,"upgrade_tool_mutex");
}
void destroy_process_mutex() {
  if (global_mutex)
    CloseHandle(global_mutex);
}
void lock_process_mutex() {
  unsigned int ret;
  if (global_mutex) {
    ret = WaitForSingleObject(global_mutex,INFINITE);
  }
}
void unlock_process_mutex() {
  if (global_mutex)
    ReleaseMutex(global_mutex);
}