#if !defined (__TERMINAL_H__)
#define __TERMINAL_H__ 1

/* Autoconf will have defined HAVE_TERMIOS_H, HAVE_TERMIO_H,
   and HAVE_SGTTY_H for us as appropriate.  */

#if defined(HAVE_TERMIOS_H)
#define HAVE_TERMIOS
#include <termios.h>
#else /* ! HAVE_TERMIOS_H */
#if defined(HAVE_TERMIO_H)
#define HAVE_TERMIO
#include <termio.h>

#undef TIOCGETP
#define TIOCGETP TCGETA
#undef TIOCSETN
#define TIOCSETN TCSETA
#undef TIOCSETP
#define TIOCSETP TCSETAF
#define TERMINAL struct termio
#else /* ! HAVE_TERMIO_H */
#ifdef HAVE_SGTTY_H
#define HAVE_SGTTY
#include <fcntl.h>
#include <sgtty.h>
#include <sys/ioctl.h>
#define TERMINAL struct sgttyb
#endif
#endif
#endif

#endif /* !defined (TERMINAL_H) */
