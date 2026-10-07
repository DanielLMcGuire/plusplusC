# ++C

C with "classes"

++C is a CRT and OOP framework for C. It provides "Classes" via macros, and other features expected from a common CRT. It is not C standard compliant. It requires no dependencies (other than win32 APIs or Linux/FreeBSD syscalls)

## Usage

### Demo

```c
#include <class.pph> // for classes
#include <app.pph> // base app interface
#include <cio.h> // simple IO

CLASS(Demo, Application)
{
    BASE(Application) // add base class
    FIELD(bool, showVersion) // field
    METHOD(bool, check_state, (void *self)) // method
};

CLASS_EXPORT(Demo); // export the class

CONSTRUCTOR(Demo)
{
    SUPER_CTOR(self, Application); // calls the base class constructor
    self->showVersion = false; // initialize fields
    METHOD_LINK(base, Demo, run); // link the methods you need into the class
    METHOD_LINK(Demo, check_state);
}

DECONSTRUCTOR(Demo)
{
    SUPER_DTOR(self, Application); // calls the base class deconstructor
}

CLASS_INFO(Demo, Application); // generates the RTTI metadata

IMPLEMENT(Demo, int, run, (void *self))
{
    printf("Hello, World!\n");
    return 0;
}
```

```c
int program(parr_t csArgs)
{
    // heap-allocate class
    Demo *demo = NEW(Demo);

    // RTTI checked cast
    Application *app = AS(Application, demo);

    // dynamic invoking
    CALL1(app, parse_args, &csArgs);
    int exit_code = CALL0(app, main);

    // run destructor + free
    REMOVE(demo);

    return exit_code;
}
```

## Build

### Prerequisites

- Linux
  - CMake 3.25+
  - GCC 13+ or Clang 16+
  - as / GNU Assembler / GCC / Clang
- FreeBSD (x86_64, aarch64)
  - CMake 3.25+, Ninja (`pkg install cmake ninja`)
  - Clang + lld from the base system
  - aarch64 also links `libcompiler_rt` (in base) for 128-bit `long double` helpers
- Windows
  - CMake 3.25+ (Usually bundled with MSVC)
  - MSVC 2022+ or Clang (targeting MinGW or MSVC ABI)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release

cmake --build build

./build/main
```

FreeBSD

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/test
```

Windows (MSVC)

```batch
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

.\build\Release\main.exe
```

## Features

### Dynamic containers

- `parr_t` Pointer array
- `dstr_t` Dynamic string
- `iarr_t` Dynamic integer array

### Signals

#### signal

```c
#include <sig.h> // read the header for more info

static void handle_sigint(int sig) {
    // ...
}

int program(parr_t csArgs) {
    if (signal(SIGINT, handle_sigint) == SIG_ERR)
        return 1;
    // ...
}
```

#### sigaction

```c
#include <sig.h>

static void on_sigterm(int sig)
{
    // ...
}

bool setup_signals(void)
{
    sigaction_t act = {0};
    sigaction_t old_act = {0};

    act.sa_handler = on_sigterm;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGTERM, &act, &old_act) < 0)
        return false;

    return true;
}
```

#### raise

```c
#include <sig.h>

static void custom_handler(int sig)
{
    // ...
}

int program(parr_t csArgs)
{
    signal(SIGUSR1, custom_handler);
    raise(SIGUSR1);
    return 0;
}
```

#### Ignoring a signal

```c
signal(SIGINT, SIG_IGN);
```

### Streams

#### Files

```c
#include <files.pph>
#include <ios.pph> // for cerr()

void fwrite_demo(void)
{
    // "w", "w+", "r", "r+", "a", "a+"
    Stream *file = fopen("log.txt", "w");
    if (!file)
    {
        CALL1(cerr(), println, "Failed to open file for writing.");
        return;
    }

    CALL2(file, printf, "Application started: %s\n", "DemoApp");
    CALL3(file, printf, "Count: %d, Float: %f\n", 42, 3.14159);

    CALL1(file, println, "static text.");

    REMOVE(file);
}
```

#### Standard IO

```c
#include <ios.pph>

void stdio_demo(void)
{
    CALL1(cout(), println, "Hello, World!");
    CALL2(cerr(), printf, "Uh oh!: code %d\n", 500);

    dstr_t input = {0};
    CALL1(cin(), readline, &input);
    CALL2(cout(), printf, "You entered: %s\n", input.data);
    dstr_free(&input);
}
```

### Paths

```c
#include <fs.pph>
#include <dstr.h>
#include <cio.h>

void path_demo(void)
{
    dstr_t root_str = dstr_new("var/logs");
    FSPath *path = fs_path(root_str);
    dstr_free(&root_str);

    // join paths (handles separators)
    CALL1(path, join, "app/debug.log");

    // output normalized path
    dstr_t full_path = CALL0(path, data);
    printf("Full Path: %s\n", full_path.data);
    dstr_free(&full_path);

    // extract directory (parent) and filename (base)
    FSPath *parent = CALL0(path, parent);
    FSPath *base   = CALL0(path, base);

    dstr_t parent_str = CALL0(parent, data);
    dstr_t base_str   = CALL0(base, data);

    printf("Parent dir: %s\n", parent_str.data);
    printf("filename:    %s\n", base_str.data);

    dstr_free(&parent_str); dstr_free(&base_str);
    REMOVE(parent); REMOVE(base); REMOVE(path);
}
```

```c
#include <fs.pph>
#include <cio.h>

bool file_ops_demo(void)
{
    dstr_t name = dstr_new("test_file.tmp");
    FSPath *p = fs_path(name);
    dstr_free(&name);

    // create an empty file
    if (!CALL0(p, touch)) {
        REMOVE(p); return false;
    }

    // delete the file
    if (!CALL0(p, remove)) {
        REMOVE(p); return false;
    }

    REMOVE(p);

    return true;
}
```

#### Memory

```c
#include <ios.pph>
#include <mems.pph>

void memory_stream_demo(void)
{
    MemoryStream *ms = NEW(MemoryStream);
    Stream *stream = AS(Stream, ms);

    // write formatted data into memory
    CALL3(stream, printf, "Message: %s (ID: %d)", "MemoryStream Test", 101);

    // rewind back to the beginning
    CALL2(stream, seek, 0, FS_SEEK_SET);

    // read bytes back from the memory stream
    char buffer[64] = {0};
    i64 bytes_read = CALL2(stream, read, buffer, sizeof(buffer) - 1);
    CALL2(cout(), printf, "Read %lld bytes from memory: %s\n", bytes_read, buffer);

    // inspect the underlying buffer directly
    const char *raw_buf = mstream_get_buffer(ms);
    CALL2(cout(), printf, "Raw contents: %s\n", raw_buf);

    REMOVE(ms);
}
```

### Sockets

```c
#include <socket.pph>
#include <cio.h>
#include <str.h>

static void run_server(void)
{
    SocketStream *listener = tcp_listen(SOCK_INADDR_ANY, 8080, 5);
    if (!listener) return;

    Stream *client = tcp_accept(listener, NULL);
    if (client) 
    {
        CALL2(client, write, "Welcome!\n", 9);
        
        char buf[64] = {0};
        CALL2(client, read, buf, sizeof(buf) - 1);
        printf("Client says: %s\n", buf);
        
        REMOVE(client);
    }
    REMOVE(listener);
}

static void run_client(void)
{
    printf("Connecting to 127.0.0.1:8080...\n");
    
    Stream *conn = tcp_connect("127.0.0.1", 8080);
    if (!conn) return;

    char buf[64] = {0};
    CALL2(conn, read, buf, sizeof(buf) - 1);
    printf("Server says: %s", buf);

    CALL2(conn, write, "Hello!\n", 7);
    
    REMOVE(conn);
}
```

### Maps

```c
#include <map.pph>
#include <cio.h>

void map_demo(void)
{
    Map *m = NEW(Map);

    // set values (values are void pointers)
    CALL2(m, set, "key1", (void *)(uintptr_t)42);
    CALL2(m, set, "key2", (void *)(uintptr_t)100);

    // check and get
    if (CALL1(m, contains, "key1")) 
    {
        int val = (int)(uintptr_t)CALL1(m, get, "key1");
        printf("key1: %d\n", val);
    }

    // iterate through map
    size_t cursor = 0;
    const char *key;
    void *value;
    
    while (CALL4(m, iterate, &cursor, &key, NULL, &value)) 
    {
        printf("%s -> %d\n", key, (int)(uintptr_t)value);
    }

    // remove a key and free the map
    CALL1(m, remove, "key1");
    REMOVE(m);
}
```

### Threads

```c
#include <thread.pph>
#include <atomic.h>
#include <cio.h>

static void *thread_task(void *arg)
{
    atomic_i32_t *counter = (atomic_i32_t *)arg;
    atomic_i32_fetch_add(counter, 1);
    return (void *)(uintptr_t)0;
}

void thread_demo(void)
{
    atomic_i32_t counter = ATOMIC_INIT(0);

    // create a thread using a function pointer
    Thread *t = thread_new_fn(thread_task, &counter);

    // start the thread
    CALL0(t, start);

    // wait for it to finish
    CALL0(t, join);

    printf("Counter: %d\n", atomic_i32_load(&counter));

    // cleanup
    REMOVE(t);
}
```

### Semaphores

```c
#include <sem.h>
#include <cio.h>

void semaphore_demo(void)
{
    sem_t sem;

    // initialize with a count of 1
    sem_init(&sem, 1);

    // acquire the semaphore (blocks if count is 0)
    if (sem_wait(&sem) == SEM_SUCCESS) 
    {
        printf("Semaphore acquired!\n");

        // release the semaphore (increments count)
        sem_post(&sem);
    }

    // try to acquire without blocking
    if (sem_trywait(&sem) == SEM_SUCCESS) 
    {
        sem_post(&sem);
    }

    // destroy when finished
    sem_destroy(&sem);
}
```

### Condition Variables

```c
#include <cond.h>
#include <mutex.h>
#include <cio.h>

static mutex_t mtx = MUTEX_INIT;
static cond_t cv;
static bool ready = false;

static void *waiter_thread(void *arg)
{
    mutex_lock(&mtx);
    
    // wait for the condition to be signaled
    while (!ready) 
    {
        cond_wait(&cv, &mtx);
    }
    
    printf("Ready state reached!\n");
    mutex_unlock(&mtx);
    
    return NULL;
}

void cond_demo(void)
{
    cond_init(&cv);

    // ... spawn waiter thread here ...

    mutex_lock(&mtx);
    ready = true;
    
    // wake up one waiting thread (use cond_broadcast to wake all)
    cond_signal(&cv);
    mutex_unlock(&mtx);

    // ... join thread ...

    cond_destroy(&cv);
}
```
