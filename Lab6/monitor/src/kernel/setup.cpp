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

Monitor monitor; // Using monitor instead of semaphore

// Philosopher states
const char *states[] = {"THINKING", "HUNGRY", "EATING"};
int philosopher_eat_count[5];
int finished_count = 0;

void philosopher(void *arg)
{
    int id = (int)arg;
    
    // Thinking
    printf("Philosopher %d: %s\n", id, states[THINKING]);
    int thinking_time = 0x1ffffff;
    while (thinking_time--);
    
    // Hungry
    printf("Philosopher %d: %s\n", id, states[HUNGRY]);
    
    // Try to pick up chopsticks
    printf("Philosopher %d: trying to pick up chopsticks\n", id);
    monitor.pickup(id);
    printf("Philosopher %d: successfully picked up chopsticks\n", id);
    programManager.schedule();
    // Eating
    printf("Philosopher %d: %s (count: %d)\n", id, states[EATING], ++philosopher_eat_count[id]);
    
    // Eating for some time
    uint32 eating_time = 0x1ffffff;
    while (eating_time--);
    
    // Put down chopsticks
    monitor.putdown(id);
    printf("Philosopher %d: put down chopsticks\n", id);
    
    // Back to thinking and finish
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

    // Initialize philosopher counters
    for (int i = 0; i < 5; i++) {
        philosopher_eat_count[i] = 0;
    }
    finished_count = 0;
    
    // Initialize monitor
    monitor.initialize();
    
    printf("Dining Philosophers Problem (Using Monitor Solution)\n");
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
