#include "syscall.h"

int main(){
  int *array = (int *)system_ShmAllocate(2*sizeof(int));
  int private_array[3], p;
  int sem_id = system_SemGet(4);
  int *shared_array = (int *)system_ShmAllocate(2*sizeof(int));
  int x = system_Fork();
  private_array[0] = 364317;
  array[0] = 1;
  if(x == 0) {
    system_SemOp(sem_id, -1);
    array[0] = 50;
    private_array[0] = 21;
    shared_array[0] = 31;
    system_PrintString("Forked Process: ");
    system_PrintString("\n");
    system_PrintString("Shared : ");
    system_PrintInt(array[0]);
    system_PrintString("\n");
    system_PrintString("Private : ");
    system_PrintInt(private_array[0]);
    system_PrintString("\n");
    system_PrintString("Shared : ");
    system_PrintInt(shared_array[0]);
    system_PrintString("\n");
    p = system_Fork();
    if(p == 0) {
      system_PrintString("Twice Forked Process\n");
    }
    else {
      system_Join(p);
      return 0;
    }
    system_SemOp(sem_id, 1);
  }
  else{
    system_SemOp(sem_id, -1);
    system_PrintString("Parent Process ");
    system_PrintString("\n");
    system_PrintString("Shared : ");
    system_PrintInt(array[0]);
    system_PrintString("\n");
    system_PrintString("Private : ");
    system_PrintInt(private_array[0]);
    system_PrintString("\n");
    system_PrintString("Shared : ");
    system_PrintInt(shared_array[0]);
    system_PrintString("\n");
    system_SemOp(sem_id, 1);
  }
  return 0;
}
