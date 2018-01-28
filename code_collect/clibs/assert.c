#define _unreachable() \
    (do_unreachable(__FILE__, __LINE__, FUNCTION_NAME))

extern void do_unreachable(const char*, int, const char*)
    ATTRIBUTE_NORETURN;

    // Assertion check.

#define _assert(expr) ((void)(!(expr) ? _unreachable(), 0 : 0))


void
do_unreachable(const char* filename, int lineno, const char* function)
{
    fprintf(stderr, _("%s: internal error in %s, at %s:%d\n"),
            program_name, function, filename, lineno);
    _exit(GOLD_ERR);
}
