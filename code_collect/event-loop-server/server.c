#include "server.h"

#include <setjmp.h>
#include <unistd.h>
#include <signal.h>

int
main (int argc, char *argv[])
{
	char *port;
	char **next_arg = &argv[1];

	port = *next_arg;

	if (port == NULL) {
		exit (1);
	}

	remote_prepare (port);

	initialize_async_io ();
	initialize_event_loop ();

	while (1)
	{
		remote_open (port);

		/* Wait for events.  This will return when all event sources are
		   removed from the event loop.  */
		start_event_loop ();
	}
}

