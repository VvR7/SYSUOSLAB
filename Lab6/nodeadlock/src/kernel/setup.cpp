#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"

// Screen IO processor
STDIO stdio;
// Interrupt manager
InterruptManager interruptManager;
// Program manager
ProgramManager programManager;

Semaphore semaphore;

// Philosopher states
const char *states[] = {"THINKING", "HUNGRY", "EATING"};
int philosopher_state[5];
int philosopher_eat_count[5];
int finished_count = 0;

void philosopher(void *arg)
{
    int id = (int)arg;
    int left = id;
    int right = (id + 1) % 5;
    
    // Thinking
    philosopher_state[id] = 0; // THINKING
    int thinking_time = 0x1ffffff;
    while (thinking_time--);
    printf("Philosopher %d: %s\n", id, states[philosopher_state[id]]);
    
    // Hungry
    philosopher_state[id] = 1; // HUNGRY
    printf("Philosopher %d: %s\n", id, states[philosopher_state[id]]);
    
    // Try to get left chopstick
    printf("Philosopher %d: trying to get chopstick %d\n", id, left);
    semaphore.P(left);
    printf("Philosopher %d: got chopstick %d\n", id, left);
    

    // Try to get right chopstick
    printf("Philosopher %d: trying to get chopstick %d\n", id, right);
    semaphore.P(right);
    printf("Philosopher %d: got chopstick %d\n", id, right);
    
    // Eating
    philosopher_state[id] = 2; // EATING
    philosopher_eat_count[id]++;
    // printf("Philosopher %d: %s (count: %d)\n", id, states[philosopher_state[id]], philosopher_eat_count[id]);
    

    uint32 eating_time = 0x1ffffff;
    while (eating_time--);
    
    // Release chopsticks
    semaphore.V(left);
    printf("Philosopher %d: released chopstick %d\n", id, left);
    
    semaphore.V(right);
    printf("Philosopher %d: released chopstick %d\n", id, right);
    
    // Back to thinking and finish
    philosopher_state[id] = 0; // THINKING
    printf("Philosopher %d: finished dining\n", id);
    finished_count++;
    
    if (finished_count == 5) {
        printf("\nAll philosophers have finished dining!\n");
    }
}

void first_thread(void *arg)
{
    // First thread cannot return
    stdio.moveCursor(0);
    for (int i = 0; i < 25 * 80; ++i)
    {
        stdio.print(' ');
    }
    stdio.moveCursor(0);

    // Initialize philosopher states and counters
    for (int i = 0; i < 5; i++) {
        philosopher_state[i] = 0; // THINKING
        philosopher_eat_count[i] = 0;
    }
    finished_count = 0;
    
    // Initialize semaphore
    semaphore.initialize();
    
    printf("Dining Philosophers Problem (One meal only)\n");
    printf("----------------------------------------\n");
    
    // Create philosopher threads
    for (int i = 0; i < 5; i++) {
        char name[20]="Philosopher";
        name[11] = '1' + i;
        name[12] = '\0';
        programManager.executeThread(philosopher, (void *)i, name, 1);
    }

    asm_halt();
}

extern "C" void setup_kernel()
{
    // Interrupt manager
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);

    // Output manager
    stdio.initialize();

    // Process/thread manager
    programManager.initialize();

    // Create first thread
    int pid = programManager.executeThread(first_thread, nullptr, "first thread", 1);
    if (pid == -1)
    {
        printf("can not execute thread\n");
        asm_halt();
    }

    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);
    firstThread->status = RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);

    asm_halt();
}
