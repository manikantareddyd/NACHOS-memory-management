#include "syscall.h"

int main(){
  int *array = (int *)system_ShmAllocate(2*sizeof(int));
 
  int x = system_Fork();
  int sem_id = system_SemGet(3);
  //system_PrintString("sem_id\n");
  system_PrintInt(sem_id);
  system_PrintString("\n");
  system_PrintString("sem_id\n");
  //printf("sem_id %d\n",sem_id);
  int p;
  p=1;
  //system_SemCtl(sem_id, 2, &p);
  if(x == 0) {
    //system_PrintString("Holadklfa\n");
     array[0] = 9;
  array[1]=7;
 
    system_SemOp(sem_id,-1);
    system_PrintString("array elements\n");
    system_PrintInt(array[1]);
    system_PrintInt(array[3]);
    system_PrintString("forked child inside semaphore\n");
    system_PrintString("FORKED CHILD INSIDE SEMAPHORE\n");
    system_SemOp(sem_id, 1);
    //system_PrintString("Child Oudhfa sempa\n");
  }
  else{
    system_Sleep(1000000);
    //system_PrintString("fadafdklfa\n");
    system_SemOp(sem_id,-1);
       //printf(" %d \n", array[1]);
  
    system_PrintString("array elements\n");
system_PrintInt(array[1]);
system_PrintInt(array[0]);
    system_PrintInt(p);
    system_PrintString("PARENT INSIDE SEMAPHORE\n");
    system_SemOp(sem_id, 1);
    system_Join(x);
    system_SemCtl(sem_id,1,&p);
    system_PrintString("Value of semaphore :");
    system_PrintInt(p);
    system_PrintString("\n");
    system_SemCtl(sem_id, 0, 0);
  }
  return 0;
}
