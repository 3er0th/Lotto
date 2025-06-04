#define _NO_CRT_STDIO_INLINE
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>

const char msg_usage[]   = "Generate a lottery line.\n\nusage: lotto main_picks main_balls [extra_picks extra_balls]\n\n";
const char msg_newline[] = "\n";

const DWORD MAX_PICKS = 8;
const DWORD MAX_BALLS = 99;

// KERNEL32 API function pointers
typedef void        (__stdcall* ExitProcess_t)      (UINT32 uExitCode);
typedef PWSTR       (__stdcall* GetCommandLineW_t)  ();
typedef HANDLE      (__stdcall* GetStdHandle_t)     (UINT32 nStdHandle);
typedef HINSTANCE   (__stdcall* LoadLibraryA_t)     (PSTR lpLibFileName);
typedef BOOL        (__stdcall* WriteFile_t)        (HANDLE hFile, void* lpBuffer, UINT32 nNumberOfBytesToWrite, UINT32* lpNumberOfBytesWritten, OVERLAPPED* lpOverlapped);

// NTDLL API function pointers
typedef void*       (__stdcall* memmove_t)          (void* _Dst, const void* _Src, size_t _Size);
typedef void        (__stdcall* qsort_t)            (void* _Base, size_t _NumOfElements, size_t _SizeOfElements, _CoreCrtNonSecureSearchSortCompareFunction _CompareFunction);
typedef int         (__cdecl*   sprintf_s_t)        (char* const _Buffer, const size_t _BufferCount, const char* const _Format, ...);
typedef int         (__stdcall* _wtoi_t)            (const wchar_t* _String);

// SHELL32 API function pointers
typedef PWSTR* (__stdcall* CommandLineToArgvW_t) (PWSTR lpCmdLine, INT32* pNumArgs);

typedef struct {
    ExitProcess_t      ExitProcess;
    GetCommandLineW_t  GetCommandLineW;
    GetStdHandle_t     GetStdHandle;
	LoadLibraryA_t     LoadLibraryA;
    WriteFile_t        WriteFile;
    memmove_t          memmove;
    qsort_t            qsort;
    sprintf_s_t        sprintf_s;
    _wtoi_t            _wtoi;
    CommandLineToArgvW_t CommandLineToArgvW;
} API_FUNCTIONS;

int  compare (const void* arg1, const void* arg2);
void decode  (UINT picks, UINT balls, PBYTE seed, PBYTE line, API_FUNCTIONS* api);

extern "C" void __stdcall lotto(DWORD64 seed) {
    API_FUNCTIONS api = { (ExitProcess_t)           0,
                          (GetCommandLineW_t)       0,
                          (GetStdHandle_t)          0,
                          (LoadLibraryA_t)          0,
		                  (WriteFile_t)             0,
                          (memmove_t)               0,
                          (qsort_t)                 0,
                          (sprintf_s_t)             0,
                          (_wtoi_t)                 0,
						  (CommandLineToArgvW_t)    0 
                        };

    PBYTE   pbSeed     = (PBYTE)&seed;
    UINT    main_picks = 0;
    UINT    main_balls = 0;
    UINT    xtra_picks = 0;
    UINT    xtra_balls = 0;
    CHAR    msg[128];
    UINT32  wrote;
    INT     argc;
    PWCHAR* argv = api.CommandLineToArgvW(api.GetCommandLineW(), &argc);
    HANDLE  hOut = api.GetStdHandle(STD_OUTPUT_HANDLE);

    switch (argc) {
    case 5:
        xtra_balls = api._wtoi(argv[4]);
        xtra_picks = api._wtoi(argv[3]);
    case 3:
        main_balls = api._wtoi(argv[2]);
        main_picks = api._wtoi(argv[1]);
        break;

    default:
_usage: api.WriteFile(hOut, (void*)msg_usage, sizeof(msg_usage), &wrote, NULL);
        goto _exit;
    }

    if (main_picks + xtra_picks > MAX_PICKS || main_picks < 1 || main_balls > MAX_BALLS || xtra_balls > MAX_BALLS)
        goto _usage;

	UINT i;
    BYTE line[8];

    decode(main_picks, main_balls, pbSeed, line, &api);
  
    api.WriteFile(hOut, msg, api.sprintf_s(msg, sizeof(msg), " main: %d balls\n", main_balls), &wrote, NULL);

    for (i = 0; i < main_picks; i++)
        api.WriteFile(hOut, msg, api.sprintf_s(msg, sizeof(msg), "    %d: %2d\n", i + 1, line[i]), &wrote, NULL);

    if (xtra_picks) {
        decode(xtra_picks, xtra_balls, &pbSeed[main_picks], line, &api);

        api.WriteFile(hOut, msg, api.sprintf_s(msg, sizeof(msg), "\nextra: %d balls\n", xtra_balls), &wrote, NULL);

        for (i = 0; i < xtra_picks; i++)
            api.WriteFile(hOut, msg, api.sprintf_s(msg, sizeof(msg), "    %d: %2d\n", i + 1, line[i]), &wrote, NULL);
    }

    api.WriteFile(hOut, (void*)msg_newline, sizeof(msg_newline), &wrote, NULL);

_exit:
    api.ExitProcess(0);
}

void decode(UINT picks, UINT balls, PBYTE seed, PBYTE line, API_FUNCTIONS* api) {
    BYTE drum[MAX_BALLS];
    UINT index;
    UINT i, j;

	// fill the drum with balls
    for (i = 0; i < balls; i++)
        drum[i] = i + 1;

	// pick each ball
    for (i = 0, j = balls; i < picks; i++, j--) {
        index = seed[i] % j;

        line[i] = drum[index];

        if (index != j - 1)
			// move the balls greater than that picked down one position
            api->memmove(&drum[index], &drum[index + 1], (j - index - 1) * sizeof(BYTE));

        drum[j - 1] = 0;
    }

    if (picks > 1)
        api->qsort(line, picks, sizeof(BYTE), compare);
}

int compare(const void* arg1, const void* arg2) {
    return *((PBYTE)arg1) - *((PBYTE)arg2);
}