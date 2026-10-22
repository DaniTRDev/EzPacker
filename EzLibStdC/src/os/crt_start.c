// EzLibStdC: C Runtime Startup Entry Points (Windows & Linux)
// Permissive MIT License

extern int main(int argc, char **argv);
extern void exit(int status);

#if defined(_WIN32)
// Default entry point for MSVC link.exe with /subsystem:console
void mainCRTStartup(void)
{
    int rc = main(0, (void *)0);
    exit(rc);
}
#endif

#if defined(__linux__) || defined(__ELF__)
// Default entry point for ELF ld/ld.lld
void _start(void)
{
    int rc = main(0, (void *)0);
    exit(rc);
}
#endif
