#include <windows.h>
#include <stdio.h>

#include "../ntix_lib.h"


typedef LONG (NTAPI *RtlAdjustPrivilege_t)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);
typedef LONG (NTAPI *NtShutdownSystem_t)(ULONG);

int main(int argc, char **argv) {
    ULONG action = 1; /* 0 halt, 1 reboot, 2 poweroff */

    opt_t opts[] = {
        {'h', "help", 0, NULL, 0},
        {'H', "halt", 0, NULL, 0},
        {'R', "reboot", 0, NULL, 0},
        {'P', "poweroff", 0, NULL, 0},
        {0, NULL, 0, NULL, 0}
    };

    if (!ntix_parse_args(&argc, &argv, opts)) {
        printf("Usage: %s [-h|--help] [-H|-P|-R]\n",argv[0]);
        return 1;
    }

    if (ntix_has_opt(opts, "h")) {
        printf("Usage: nt_reboot [-h|--help] [-H|-P|-R]\n");
        printf("  -H|--halt      Halt machine\n");
        printf("  -P|--poweroff  Poweroff machine\n");
        printf("  -R|--reboot    Reboot machine\n");
        return 0;
    }

    if(ntix_has_opt(opts,"H"))
        action = 0;

    if(ntix_has_opt(opts,"R"))
        action = 1;

    if(ntix_has_opt(opts,"P"))
        action = 2;

    HMODULE nt = GetModuleHandleA("ntdll.dll");
    RtlAdjustPrivilege_t adjust = (RtlAdjustPrivilege_t)GetProcAddress(nt, "RtlAdjustPrivilege");
    NtShutdownSystem_t shutdown = (NtShutdownSystem_t)GetProcAddress(nt, "NtShutdownSystem");

    BOOLEAN old;
    LONG s = adjust(19, TRUE, FALSE, &old);   /* SE_SHUTDOWN_PRIVILEGE */
    if (s < 0) { printf("privilege failed: 0x%lx\n", s); return 1; }
    s = shutdown(action);
    printf("NtShutdownSystem: 0x%lx\n", s);
    return 1;
}
