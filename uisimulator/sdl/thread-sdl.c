/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <time.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <stdlib.h>
#include <memory.h>
#include <setjmp.h>
#include "system-sdl.h"
#include "thread-sdl.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "debug.h"

/* Define this as 1 to show informational messages that are not errors. */
#define THREAD_SDL_DEBUGF_ENABLED 0

#if THREAD_SDL_DEBUGF_ENABLED
#define THREAD_SDL_DEBUGF(...) DEBUGF(__VA_ARGS__)
static char __name[32];
#define THREAD_SDL_GET_NAME(thread) \
    ({ thread_get_name(__name, ARRAYLEN(__name), thread); __name; })
#else
#define THREAD_SDL_DEBUGF(...)
#define THREAD_SDL_GET_NAME(thread)
#endif

#define THREAD_PANICF(str...) \
    ({ fprintf(stderr, str); exit(-1); })

/* Thread/core entries as in rockbox core */
struct core_entry cores[NUM_CORES];
struct thread_entry threads[MAXTHREADS];
/* Jump buffers for graceful exit - kernel threads don't stay neatly
 * in their start routines responding to messages so this is the only
 * way to get them back in there so they may exit */
static jmp_buf thread_jmpbufs[MAXTHREADS];
static SDL_mutex *m;
static bool threads_exit = false;

extern long start_tick;

void thread_sdl_shutdown(void)
{
    int i;
    /* Take control */
    SDL_LockMutex(m);

    /* Tell all threads jump back to their start routines, unlock and exit
       gracefully - we'll check each one in turn for it's status. Threads
       _could_ terminate via remove_thread or multiple threads could exit
       on each unlock but that is safe. */
    threads_exit = true;

    for (i = 0; i < MAXTHREADS; i++)
    {
        struct thread_entry *thread = &threads[i];
        if (thread->context.t != NULL)
        {
            /* Signal thread on delay or block */
            SDL_Thread *t = thread->context.t;
            SDL_SemPost(thread->context.s);
            SDL_UnlockMutex(m);
            /* Wait for it to finish */
            SDL_WaitThread(t, NULL);
            /* Relock for next thread signal */
            SDL_LockMutex(m);
        }        
    }

    SDL_UnlockMutex(m);
    SDL_DestroyMutex(m);
}

/* Do main thread creation in this file scope to avoid the need to double-
   return to a prior call-level which would be unaware of the fact setjmp
   was used */
extern void app_main(void *param);
static int thread_sdl_app_main(void *param)
{
    SDL_LockMutex(m);
    cores[CURRENT_CORE].running = &threads[0];

    /* Set the jump address for return */
    if (setjmp(thread_jmpbufs[0]) == 0)
    {
        app_main(param);
        /* should not ever be reached but... */
        THREAD_PANICF("app_main returned!\n");
    }

    /* Unlock and exit */
    SDL_UnlockMutex(m);
    return 0;
}

/* Initialize SDL threading */
bool thread_sdl_init(void *param)
{
    struct thread_entry *thread;
    memset(cores, 0, sizeof(cores));
    memset(threads, 0, sizeof(threads));

    m = SDL_CreateMutex();

    if (SDL_LockMutex(m) == -1)
    {
        fprintf(stderr, "Couldn't lock mutex\n");
        return false;
    }

    /* Slot 0 is reserved for the main thread - initialize it here and
       then create the SDL thread - it is possible to have a quick, early
       shutdown try to access the structure. */
    thread = &threads[0];
    thread->stack = (uintptr_t *)"       ";
    thread->stack_size = 8;
    thread->name = "main";
    thread->state = STATE_RUNNING;
    thread->context.s = SDL_CreateSemaphore(0);
    cores[CURRENT_CORE].running = thread;
 
    if (thread->context.s == NULL)
    {
        fprintf(stderr, "Failed to create main semaphore\n");
        return false;
    }

    thread->context.t = SDL_CreateThread(thread_sdl_app_main, param);

    if (thread->context.t == NULL)
    {
        SDL_DestroySemaphore(thread->context.s);
        fprintf(stderr, "Failed to create main thread\n");
        return false;
    }

    THREAD_SDL_DEBUGF("Main thread: %p\n", thread);

    SDL_UnlockMutex(m);
    return true;
}

/* A way to yield and leave the threading system for extended periods */
void thread_sdl_thread_lock(void *me)
{
    SDL_LockMutex(m);
    cores[CURRENT_CORE].running = (struct thread_entry *)me;

    if (threads_exit)
        thread_exit();
}

void * thread_sdl_thread_unlock(void)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    SDL_UnlockMutex(m);
    return current;
}

static struct thread_entry * find_empty_thread_slot(void)
{
    struct thread_entry *thread = NULL;
    int n;

    for (n = 0; n < MAXTHREADS; n++)
    {
        int state = threads[n].state;

        if (state == STATE_KILLED)
        {
            thread = &threads[n];
            break;
        }
    }

    return thread;
}

static void add_to_list_l(struct thread_entry **list,
                          struct thread_entry *thread)
{
    if (*list == NULL)
    {
        /* Insert into unoccupied list */
        thread->l.next = thread;
        thread->l.prev = thread;
        *list = thread;
    }
    else
    {
        /* Insert last */
        thread->l.next = *list;
        thread->l.prev = (*list)->l.prev;
        thread->l.prev->l.next = thread;
        (*list)->l.prev = thread;
    }
}

static void remove_from_list_l(struct thread_entry **list,
                               struct thread_entry *thread)
{
    if (thread == thread->l.next)
    {
        /* The only item */
        *list = NULL;
        return;
    }

    if (thread == *list)
    {
        /* List becomes next item */
        *list = thread->l.next;
    }

    /* Fix links to jump over the removed entry. */
    thread->l.prev->l.next = thread->l.next;
    thread->l.next->l.prev = thread->l.prev;
}

struct thread_entry *thread_get_current(void)
{
    return cores[CURRENT_CORE].running;
}

void switch_thread(void)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;

    set_irq_level(0);

    switch (current->state)
    {
    case STATE_RUNNING:
    {
        SDL_UnlockMutex(m);
        /* Any other thread waiting already will get it first */
        SDL_LockMutex(m);
        break;
        } /* STATE_RUNNING: */

    case STATE_BLOCKED:
    {
        int oldlevel;

        SDL_UnlockMutex(m);
        SDL_SemWait(current->context.s);
        SDL_LockMutex(m);

        oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
        current->state = STATE_RUNNING;
        set_irq_level(oldlevel);
        break;
        } /* STATE_BLOCKED: */

    case STATE_BLOCKED_W_TMO:
    {
        int result, oldlevel;

        SDL_UnlockMutex(m);
        result = SDL_SemWaitTimeout(current->context.s, current->tmo_tick);
        SDL_LockMutex(m);

        oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

        if (current->state == STATE_BLOCKED_W_TMO)
        {
            /* Timed out */
            remove_from_list_l(current->bqp, current);

#ifdef HAVE_WAKEUP_EXT_CB
            if (current->wakeup_ext_cb != NULL)
                current->wakeup_ext_cb(current);
#endif
            current->state = STATE_RUNNING;
        }

        if (result == SDL_MUTEX_TIMEDOUT)
        {
            /* Other signals from an explicit wake could have been made before
             * arriving here if we timed out waiting for the semaphore. Make
             * sure the count is reset. */
            while (SDL_SemValue(current->context.s) > 0)
                SDL_SemTryWait(current->context.s);
        }

        set_irq_level(oldlevel);
        break;
        } /* STATE_BLOCKED_W_TMO: */

    case STATE_SLEEPING:
    {
        SDL_UnlockMutex(m);
        SDL_SemWaitTimeout(current->context.s, current->tmo_tick);
        SDL_LockMutex(m);
        current->state = STATE_RUNNING;
        break;
        } /* STATE_SLEEPING: */
    }

    cores[CURRENT_CORE].running = current;

    if (threads_exit)
        thread_exit();
}

void sleep_thread(int ticks)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    int rem;

    current->state = STATE_SLEEPING;

    rem = (SDL_GetTicks() - start_tick) % (1000/HZ);
    if (rem < 0)
        rem = 0;

    current->tmo_tick = (1000/HZ) * ticks + ((1000/HZ)-1) - rem;
}

void block_thread(struct thread_entry *current)
{
    current->state = STATE_BLOCKED;
    add_to_list_l(current->bqp, current);
}

void block_thread_w_tmo(struct thread_entry *current, int ticks)
{
    current->state = STATE_BLOCKED_W_TMO;
    current->tmo_tick = (1000/HZ)*ticks;
    add_to_list_l(current->bqp, current);
}

unsigned int wakeup_thread(struct thread_entry **list)
{
    struct thread_entry *thread = *list;

    if (thread != NULL)
    {
        switch (thread->state)
        {
        case STATE_BLOCKED:
        case STATE_BLOCKED_W_TMO:
            remove_from_list_l(list, thread);
            thread->state = STATE_RUNNING;
            SDL_SemPost(thread->context.s);
            return THREAD_OK;
        }
    }

    return THREAD_NONE;
}

unsigned int thread_queue_wake(struct thread_entry **list)
{
    unsigned int result = THREAD_NONE;

    for (;;)
    {
        unsigned int rc = wakeup_thread(list);

        if (rc == THREAD_NONE)
            break;

        result |= rc;        
    }

    return result;
}

void thread_thaw(struct thread_entry *thread)
{
    if (thread->state == STATE_FROZEN)
    {
        thread->state = STATE_RUNNING;
        SDL_SemPost(thread->context.s);
    }
}

int runthread(void *data)
{
    struct thread_entry *current;
    jmp_buf *current_jmpbuf;

    /* Cannot access thread variables before locking the mutex as the
       data structures may not be filled-in yet. */
    SDL_LockMutex(m);
    cores[CURRENT_CORE].running = (struct thread_entry *)data;
    current = cores[CURRENT_CORE].running;
    current_jmpbuf = &thread_jmpbufs[current - threads];

    /* Setup jump for exit */
    if (setjmp(*current_jmpbuf) == 0)
    {
        /* Run the thread routine */
        if (current->state == STATE_FROZEN)
        {
            SDL_UnlockMutex(m);
            SDL_SemWait(current->context.s);
            SDL_LockMutex(m);
            cores[CURRENT_CORE].running = current;
        }

        if (!threads_exit)
        {
            current->context.start();
            THREAD_SDL_DEBUGF("Thread Done: %d (%s)\n",
                              current - threads, THREAD_SDL_GET_NAME(current));
            /* Thread routine returned - suicide */
        }

        thread_exit();
    }
    else
    {
        /* Unlock and exit */
        SDL_UnlockMutex(m);
    }

    return 0;
}

struct thread_entry* 
    create_thread(void (*function)(void), void* stack, size_t stack_size,
                  unsigned flags, const char *name)
{
    struct thread_entry *thread;
    SDL_Thread* t;
    SDL_sem *s;

    THREAD_SDL_DEBUGF("Creating thread: (%s)\n", name ? name : "");

    thread = find_empty_thread_slot();
    if (thread == NULL)
    {
        DEBUGF("Failed to find thread slot\n");
        return NULL;
    }

    s = SDL_CreateSemaphore(0);
    if (s == NULL)
    {
        DEBUGF("Failed to create semaphore\n");
        return NULL;
    }

    t = SDL_CreateThread(runthread, thread);
    if (t == NULL)
    {
        DEBUGF("Failed to create SDL thread\n");
        SDL_DestroySemaphore(s);
        return NULL;
    }

    thread->stack = stack;
    thread->stack_size = stack_size;
    thread->name = name;
    thread->state = (flags & CREATE_THREAD_FROZEN) ?
        STATE_FROZEN : STATE_RUNNING;
    thread->context.start = function;
    thread->context.t = t;
    thread->context.s = s;

    THREAD_SDL_DEBUGF("New Thread: %d (%s)\n",
                      thread - threads, THREAD_SDL_GET_NAME(thread));

    return thread;
}

void init_threads(void)
{
    /* Main thread is already initialized */
    if (cores[CURRENT_CORE].running != &threads[0])
    {
        THREAD_PANICF("Wrong main thread in init_threads: %p\n",
                      cores[CURRENT_CORE].running);
    }

    THREAD_SDL_DEBUGF("First Thread: %d (%s)\n",
            0, THREAD_SDL_GET_NAME(&threads[0]));
}

void remove_thread(struct thread_entry *thread)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;
    SDL_Thread *t;
    SDL_sem *s;

    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    if (thread == NULL)
    {
        thread = current;
    }

    t = thread->context.t;
    s = thread->context.s;
    thread->context.t = NULL;

    if (thread != current)
    {
        switch (thread->state)
        {
        case STATE_BLOCKED:
        case STATE_BLOCKED_W_TMO:
            /* Remove thread from object it's waiting on */
            remove_from_list_l(thread->bqp, thread);

#ifdef HAVE_WAKEUP_EXT_CB
            if (thread->wakeup_ext_cb != NULL)
                thread->wakeup_ext_cb(thread);
#endif
            break;
        }

        SDL_SemPost(s);
    }

    THREAD_SDL_DEBUGF("Removing thread: %d (%s)\n",
        thread - threads, THREAD_SDL_GET_NAME(thread));

    thread->state = STATE_KILLED;
    thread_queue_wake(&thread->queue);

    SDL_DestroySemaphore(s);

    if (thread == current)
    {
        /* Do a graceful exit - perform the longjmp back into the thread
           function to return */
        set_irq_level(oldlevel);
        longjmp(thread_jmpbufs[current - threads], 1);
    }

    SDL_KillThread(t);
    set_irq_level(oldlevel);
}

void thread_exit(void)
{
    remove_thread(NULL);
}

void thread_wait(struct thread_entry *thread)
{
    struct thread_entry *current = cores[CURRENT_CORE].running;

    if (thread == NULL)
        thread = current;

    if (thread->state != STATE_KILLED)
    {
        current->bqp = &thread->queue;
        block_thread(current);
        switch_thread();
    }
}

int thread_stack_usage(const struct thread_entry *thread)
{
    return 50;
    (void)thread;
}

unsigned thread_get_status(const struct thread_entry *thread)
{
    return thread->state;
}

/* Return name if one or ID if none */
void thread_get_name(char *buffer, int size,
                     struct thread_entry *thread)
{
    if (size <= 0)
        return;

    *buffer = '\0';

    if (thread)
    {
        /* Display thread name if one or ID if none */
        bool named = thread->name && *thread->name;
        const char *fmt = named ? "%s" : "%08lX";
        intptr_t name = named ?
            (intptr_t)thread->name : (intptr_t)thread;
        snprintf(buffer, size, fmt, name);
    }
}
