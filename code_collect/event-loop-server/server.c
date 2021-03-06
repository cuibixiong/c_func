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

static int
process_serial_event (void)
{
	return 0;
}

int
handle_serial_event (int err, remote_client_data client_data)
{
  /* Really handle it.  */
  if (process_serial_event () < 0)
    return -1;

  /* Be sure to not change the selected thread behind GDB's back.
     Important in the non-stop mode asynchronous protocol.  */
  //set_desired_thread (1);

  return 0;
}

