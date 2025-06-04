#define _NO_CRT_STDIO_INLINE
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>

const char msg_usage[]   = "Generate a lottery line.\n\nusage: lotto main_picks main_balls [extra_picks extra_balls]\n\n";
const char msg_newline[] = "\n";

int  compare (const void* arg1, const void* arg2);
void decode  (UINT picks, UINT balls, PBYTE seed, PBYTE line);

const DWORD MAX_PICKS = 8;
const DWORD MAX_BALLS = 99;

extern "C" void __stdcall lotto(DWORD64 seed) {
    PBYTE   pbSeed     = (PBYTE)&seed;
    UINT    main_picks = 0;
    UINT    main_balls = 0;
    UINT    xtra_picks = 0;
    UINT    xtra_balls = 0;
    CHAR    msg[128];
    DWORD   wrote;
    INT     argc;
    PWCHAR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    HANDLE  hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    switch (argc) {
    case 5:
        xtra_balls = _wtoi(argv[4]);
        xtra_picks = _wtoi(argv[3]);
    case 3:
        main_balls = _wtoi(argv[2]);
        main_picks = _wtoi(argv[1]);
        break;

    default:
_usage: WriteFile(hOut, msg_usage, sizeof(msg_usage), &wrote, NULL);
        goto _exit;
    }

    if (main_picks + xtra_picks > MAX_PICKS || main_picks < 1 || main_balls > MAX_BALLS || xtra_balls > MAX_BALLS)
        goto _usage;

	UINT i;
    BYTE line[8];

    decode(main_picks, main_balls, pbSeed, line);
  
    WriteFile(hOut, msg, sprintf_s(msg, sizeof(msg), " main: %d balls\n", main_balls), &wrote, NULL);

    for (i = 0; i < main_picks; i++)
        WriteFile(hOut, msg, sprintf_s(msg, sizeof(msg), "    %d: %2d\n", i + 1, line[i]), &wrote, NULL);

    if (xtra_picks) {
        decode(xtra_picks, xtra_balls, &pbSeed[main_picks], line);

        WriteFile(hOut, msg, sprintf_s(msg, sizeof(msg), "\nextra: %d balls\n", xtra_balls), &wrote, NULL);

        for (i = 0; i < xtra_picks; i++)
            WriteFile(hOut, msg, sprintf_s(msg, sizeof(msg), "    %d: %2d\n", i + 1, line[i]), &wrote, NULL);
    }

    WriteFile(hOut, msg_newline, sizeof(msg_newline), &wrote, NULL);

_exit:
    ExitProcess(0);
}

void decode(UINT picks, UINT balls, PBYTE seed, PBYTE line) {
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
            memmove(&drum[index], &drum[index + 1], (j - index - 1) * sizeof(BYTE));

        drum[j - 1] = 0;
    }

    if (picks > 1)
        qsort(line, picks, sizeof(BYTE), compare);
}

int compare(const void* arg1, const void* arg2) {
    return *((PBYTE)arg1) - *((PBYTE)arg2);
}