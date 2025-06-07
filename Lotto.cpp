#define _NO_CRT_STDIO_INLINE
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>
#include <winternl.h>
#include <intrin.h>

#pragma warning (disable : 4312)

const CHAR MSG_USAGE[]   = "Generate a lottery line.\n\nusage: lotto main_picks main_balls [extra_picks extra_balls]\n\n";
const CHAR MSG_RDSEED[]  = "The required RDSEED instruction is not supported on this CPU.\n\n";
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
    _wtoi_t              _wtoi;                     // NTDLL
    memmove_t            memmove;
    qsort_t              qsort;
    sprintf_s_t          sprintf_s;
    ExitProcess_t        ExitProcess;               // KERNEL32
    GetCommandLineW_t    GetCommandLineW;
    GetStdHandle_t       GetStdHandle;
    LoadLibraryA_t       LoadLibraryA;
    WriteFile_t          WriteFile;
    CommandLineToArgvW_t CommandLineToArgvW;        // SHELL32
} API, *PAPI;

int  compare (const void* arg1, const void* arg2);
void draw    (UINT picks, UINT balls, PBYTE seed, PBYTE line, PAPI papi);
UINT hash    (PCHAR name);
void import  (HMODULE hDll, UINT_PTR* pFunction, UINT function_count);

extern "C" void __stdcall lotto(BOOL rdseed_support, DWORD64 seed) {
/*
    // pre-calculate hashes of the import function names

    DWORD dwHash;    

    dwHash = hash((PCHAR)"_wtoi");
    dwHash = hash((PCHAR)"memmove");
    dwHash = hash((PCHAR)"qsort");
    dwHash = hash((PCHAR)"sprintf_s");
    dwHash = hash((PCHAR)"ExitProcess");
    dwHash = hash((PCHAR)"GetCommandLineW");
    dwHash = hash((PCHAR)"GetStdHandle");
    dwHash = hash((PCHAR)"LoadLibraryA");
    dwHash = hash((PCHAR)"WriteFile");
    dwHash = hash((PCHAR)"CommandLineToArgvW");
*/
    // initialize the API structure with function name hashes
    API api = { (_wtoi_t)               0x6b4f8bf0, // NTDLL
                (memmove_t)             0x62c2fb57,
                (qsort_t)               0x61a74cb9,
                (sprintf_s_t)           0x4146fe38,
                (ExitProcess_t)         0xc3f39f16, // KERNEL32
                (GetCommandLineW_t)     0x9c31b77b,
                (GetStdHandle_t)        0xc11ba43e,
                (LoadLibraryA_t)        0x74776072,
                (WriteFile_t)           0xc8ff4053,
                (CommandLineToArgvW_t)  0xe0454601  // SHELL32
              };

    HMODULE     hModule;
    PLIST_ENTRY ple = ((PPEB)__readgsqword(0x60))->Ldr->InMemoryOrderModuleList.Flink->Flink;

    // NTDLL imports
    hModule = (HMODULE)CONTAINING_RECORD(ple, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks)->DllBase;
    import(hModule, (PUINT_PTR)&api._wtoi, 4);

	// KERNEL32 imports
    hModule = (HMODULE)CONTAINING_RECORD(ple->Flink, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks)->DllBase;
    import(hModule, (PUINT_PTR)&api.ExitProcess, 5);

	// SHELL32 imports
    hModule = api.LoadLibraryA((PCHAR)"SHELL32");
    import(hModule, (PUINT_PTR)&api.CommandLineToArgvW, 1);

    UINT   wrote;
    HANDLE hOut = api.GetStdHandle(STD_OUTPUT_HANDLE);

    if (!rdseed_support) {
        api.WriteFile(hOut, (PVOID)MSG_RDSEED, _countof(MSG_RDSEED), &wrote, nullptr);
        api.ExitProcess(1);
    }

    CHAR    msg[100];
    BYTE    line[8];
    UINT    main_picks;
    UINT    main_balls;
    UINT    xtra_picks = 0;
    UINT    xtra_balls = 0;
    PBYTE   pbSeed     = (PBYTE)&seed;

    INT32   argc;
    PWCHAR* argv = api.CommandLineToArgvW(api.GetCommandLineW(), &argc);

    // minimal argument validation...
    switch (argc) {
    case 5:
        xtra_picks = api._wtoi(argv[3]);
        xtra_balls = api._wtoi(argv[4]);
    case 3:
        main_picks = api._wtoi(argv[1]);
        main_balls = api._wtoi(argv[2]);
        break;

    default:
        api.WriteFile(hOut, (PVOID)MSG_USAGE, _countof(MSG_USAGE), &wrote, nullptr);
        api.ExitProcess(ERROR_BAD_ARGUMENTS);
    }

    draw(main_picks, main_balls, pbSeed, line, &api);
  
    api.WriteFile(hOut, msg, api.sprintf_s(msg, _countof(msg), " main: %d balls\n", main_balls), &wrote, nullptr);

    for (UINT i = 0; i < main_picks; i++)
        api.WriteFile(hOut, msg, api.sprintf_s(msg, _countof(msg), "    %d: %2d\n", i + 1, line[i]), &wrote, nullptr);

    if (xtra_picks) {
        draw(xtra_picks, xtra_balls, &pbSeed[main_picks], line, &api);

        api.WriteFile(hOut, msg, api.sprintf_s(msg, _countof(msg), "\nextra: %d balls\n", xtra_balls), &wrote, nullptr);

        for (UINT i = 0; i < xtra_picks; i++)
            api.WriteFile(hOut, msg, api.sprintf_s(msg, _countof(msg), "    %d: %2d\n", i + 1, line[i]), &wrote, nullptr);
    }

    api.WriteFile(hOut, (PVOID)MSG_NEWLINE, _countof(MSG_NEWLINE), &wrote, nullptr);
    api.ExitProcess(ERROR_SUCCESS);
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
        papi->memmove(&drum[pick], &drum[pick + 1], balls - pick - 1);
    }

    papi->qsort(line, picks, 1, compare);
}

// sort compare function
int compare(const void* arg1, const void* arg2) {
    return *((PBYTE)arg1) - *((PBYTE)arg2);
}

// calculate function name hash
__inline UINT hash(PCHAR name) {
    DWORD hash = 0;

    while (*name) {
        hash += *name++;
        hash  = _lrotr(hash, 13);
    }

    return hash;
}

// import function addresses from dll
//
// NB: this simple implementation assumes...
// 
//  1. the imort functions are in the same order as they exist in the dll's export table - allowing a single pass through the name_table
//     (Windows dll export names all appear to be in alphabetical order; presumably to enable search optimization)
// 
//  2. that none of the imports are forwarded to another dll - no support for import forwarding
//
void import(HMODULE hModule, UINT_PTR* pFunction, UINT function_count) {
    auto module     = (PCHAR)  hModule;
    auto exp_dir    = (PIMAGE_EXPORT_DIRECTORY) 
                               (module + (&(&((PIMAGE_NT_HEADERS)(module + ((PIMAGE_DOS_HEADER)module)->e_lfanew))->OptionalHeader)->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT])->VirtualAddress);
    auto ordi_table = (PWORD)  (module + exp_dir->AddressOfNameOrdinals);
    auto name_table = (PULONG) (module + exp_dir->AddressOfNames);
    auto func_table = (PULONG) (module + exp_dir->AddressOfFunctions);
    auto func_hash  = (UINT32) *pFunction;

    for (UINT i = 0, n = exp_dir->NumberOfNames; i < n; i++)
        if (func_hash == hash(module + name_table[i])) {
            *pFunction = (UINT_PTR) module + func_table[ ordi_table[i] ];

            if (--function_count == 0)
                return;

            func_hash = (UINT) *(++pFunction);
        }
}