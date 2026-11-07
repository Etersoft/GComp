/*
 * Workaround for Wine's static atexit() used by winegcc PE modules
 * (dlls/ucrtbase/atexit.c): it registers functions in an _onexit_table_t
 * that is never passed to _initialize_onexit_table(). Wine's ucrtbase
 * accepts a zeroed table, but Windows' ucrtbase keeps encoded pointers
 * there, so the first atexit() from a DLL's C++ static constructor
 * corrupts the heap (STATUS_HEAP_CORRUPTION while loading the DLL).
 *
 * Defining atexit here keeps the linker from pulling atexit.o from
 * libucrtbase.a. Only used by build-winegcc.sh.
 */
#include <windows.h>
#include <process.h>

static _onexit_table_t table;
static int initialized;

int __cdecl atexit(_PVFV func)
{
    if (!initialized)
    {
        _initialize_onexit_table(&table);
        initialized = 1;
    }
    return _register_onexit_function(&table, (_onexit_t)func);
}

static void __cdecl run_atexit(void)
{
    if (initialized) _execute_onexit_table(&table);
}

#pragma section(".CRT$XTB", read)
__declspec(allocate(".CRT$XTB")) __attribute__((used)) static _PVFV run_atexit_ptr = run_atexit;
