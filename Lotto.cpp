#define _NO_CRT_STDIO_INLINE
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>
#include <winternl.h>
#include <intrin.h>

#pragma warning (disable : 4312)

const CHAR MSG_USAGE[]   = "Generate a lottery line.\n\nusage: lotto main_picks main_balls [extra_picks extra_balls]\n\n";
const CHAR MSG_NEWLINE[] = "\n";
const UINT MAX_PICKS     = 8;
const UINT MAX_BALLS     = 99;

// NTDLL function types
typedef int         (__stdcall* _wtoi_t)              (const wchar_t* _String);
typedef void*       (__stdcall* memmove_t)            (void* _Dst, const void* _Src, size_t _Size);
typedef void        (__stdcall* qsort_t)              (void* _Base, size_t _NumOfElements, size_t _SizeOfElements, _CoreCrtNonSecureSearchSortCompareFunction _CompareFunction);
typedef int         (__cdecl*   sprintf_s_t)          (char* const _Buffer, const size_t _BufferCount, const char* const _Format, ...);

// KERNEL32 function types
typedef void        (__stdcall* ExitProcess_t)        (UINT32 uExitCode);
typedef PWSTR       (__stdcall* GetCommandLineW_t)    ();
typedef HANDLE      (__stdcall* GetStdHandle_t)       (UINT32 nStdHandle);
typedef HINSTANCE   (__stdcall* LoadLibraryA_t)       (PSTR lpLibFileName);
typedef BOOL        (__stdcall* WriteFile_t)          (HANDLE hFile, PVOID lpBuffer, UINT32 nNumberOfBytesToWrite, UINT32* lpNumberOfBytesWritten, OVERLAPPED* lpOverlapped);

// SHELL32 function types
typedef PWSTR*      (__stdcall* CommandLineToArgvW_t) (PWSTR lpCmdLine, INT32* pNumArgs);

typedef struct {
    _wtoi_t              _wtoi;                 // NTDLL
    memmove_t            memmove;
    qsort_t              qsort;
    sprintf_s_t          sprintf_s;
    ExitProcess_t        ExitProcess;           // KERNEL32
    GetCommandLineW_t    GetCommandLineW;
    GetStdHandle_t       GetStdHandle;
	LoadLibraryA_t       LoadLibraryA;
    WriteFile_t          WriteFile;
    CommandLineToArgvW_t CommandLineToArgvW;    // SHELL32
} API, *PAPI;

int  compare (const void* arg1, const void* arg2);
void draw    (UINT picks, UINT balls, PBYTE seed, PBYTE line, PAPI papi);
UINT hash    (PCHAR name);
void import  (HMODULE hDll, UINT_PTR* pFunction, UINT function_count);

extern "C" void __stdcall lotto(DWORD64 seed) {
    API api = {  (_wtoi_t)               0x15AEC71E, // NTDLL
                 (memmove_t)             0x6B995961,
                 (qsort_t)               0x0DC7E5E9,
                 (sprintf_s_t)           0xEB624A6B,
                 (ExitProcess_t)         0x5AB36941, // KERNEL32
		         (GetCommandLineW_t)     0x3FFBA1F3,
		         (GetStdHandle_t)        0xFA9F0EA7,
                 (LoadLibraryA_t)        0x72258513,
                 (WriteFile_t)           0xF3A5BB62,
                 (CommandLineToArgvW_t)  0x82D25465  // SHELL32
              };

    HMODULE     hDll;
    PLIST_ENTRY ple = ((PPEB)__readgsqword(0x60))->Ldr->InMemoryOrderModuleList.Flink->Flink;

    // NTDLL imports
    hDll = (HMODULE)CONTAINING_RECORD(ple,        LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks)->DllBase;
    import(hDll, (PUINT_PTR)&api._wtoi, 4);

	// KERNEL32 imports
    hDll = (HMODULE)CONTAINING_RECORD(ple->Flink, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks)->DllBase;
    import(hDll, (PUINT_PTR)&api.ExitProcess, 5);

	// SHELL32 imports
    hDll = api.LoadLibraryA((PCHAR)"SHELL32");
    import(hDll, (PUINT_PTR)&api.CommandLineToArgvW, 1);

    CHAR    msg[100];
    BYTE    line[8];
    INT     argc;
    UINT    wrote;
    UINT    main_picks;
    UINT    main_balls;
    UINT    xtra_picks = 0;
    UINT    xtra_balls = 0;
    PBYTE   pbSeed     = (PBYTE)&seed;
    PWCHAR* argv       = api.CommandLineToArgvW(api.GetCommandLineW(), &argc);
    HANDLE  hOut       = api.GetStdHandle(STD_OUTPUT_HANDLE);

    switch (argc) {
    case 5:
        xtra_balls = api._wtoi(argv[4]);
        xtra_picks = api._wtoi(argv[3]);
    case 3:
        main_balls = api._wtoi(argv[2]);
        main_picks = api._wtoi(argv[1]);
        break;

    default:
        api.WriteFile(hOut, (PVOID)MSG_USAGE, sizeof(MSG_USAGE), &wrote, NULL);
        goto _exit;
    }

    draw(main_picks, main_balls, pbSeed, line, &api);
  
    api.WriteFile(hOut, msg, api.sprintf_s(msg, sizeof(msg), " main: %d balls\n", main_balls), &wrote, NULL);

    for (UINT i = 0; i < main_picks; i++)
        api.WriteFile(hOut, msg, api.sprintf_s(msg, sizeof(msg), "    %d: %2d\n", i + 1, line[i]), &wrote, NULL);

    if (xtra_picks) {
        draw(xtra_picks, xtra_balls, &pbSeed[main_picks], line, &api);

        api.WriteFile(hOut, msg, api.sprintf_s(msg, sizeof(msg), "\nextra: %d balls\n", xtra_balls), &wrote, NULL);

        for (UINT i = 0; i < xtra_picks; i++)
            api.WriteFile(hOut, msg, api.sprintf_s(msg, sizeof(msg), "    %d: %2d\n", i + 1, line[i]), &wrote, NULL);
    }

    api.WriteFile(hOut, (PVOID)MSG_NEWLINE, sizeof(MSG_NEWLINE), &wrote, NULL);

_exit:
    api.ExitProcess(0);
}

// draw a lottery line
void draw(UINT picks, UINT balls, PBYTE seed, PBYTE line, PAPI papi) {
    BYTE drum[MAX_BALLS];

	// fill the drum with balls
    for (UINT i = 0; i < balls; i++)
        drum[i] = i + 1;

	// pick each ball
    for (UINT i = 0, pick; i < picks; i++, balls--) {
        pick = seed[i] % balls;

        line[i] = drum[pick];
        papi->memmove(&drum[pick], &drum[pick + 1], (balls - pick - 1) * sizeof(BYTE));
    }

    papi->qsort(line, picks, sizeof(BYTE), compare);
}

// sort compare function
int compare(const void* arg1, const void* arg2) {
    return *((PBYTE)arg1) - *((PBYTE)arg2);
}

// calculate function name hash
__inline UINT hash(PCHAR name) {
    DWORD hash = 0x48617368;

    while (*name) {
        hash  = _lrotr(hash, 13);
        hash += *name++;
    }

    return hash;
}

// import function addresses from dll
void import(HMODULE hDll, UINT_PTR* pFunction, UINT function_count) {
    auto modb       = (PCHAR)hDll;
    auto dos_header = (PIMAGE_DOS_HEADER) modb;
    auto nt_headers = (PIMAGE_NT_HEADERS)(modb + dos_header->e_lfanew);
    auto opt_header = &nt_headers->OptionalHeader;
    auto exp_entry  = (PIMAGE_DATA_DIRECTORY)(&opt_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    auto exp_dir    = (PIMAGE_EXPORT_DIRECTORY)(modb + exp_entry->VirtualAddress);
    auto ord_table  = (PWORD) (modb + exp_dir->AddressOfNameOrdinals);
    auto name_table = (PULONG)(modb + exp_dir->AddressOfNames);
    auto func_hash  = (UINT)*pFunction;

    for (UINT32 i = 0, n = exp_dir->NumberOfNames; i < n; i++)
        if (func_hash == hash(modb + (UINT_PTR)name_table[i])) {
            *pFunction = (UINT_PTR)(modb + ((PULONG)(modb + exp_dir->AddressOfFunctions))[ord_table[i]]);

            if (--function_count == 0)
                return;

            func_hash = (UINT) *(++pFunction);
        }
}