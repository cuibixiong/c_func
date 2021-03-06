#include "queue.h"
#include "server.h"

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef USE_WIN32API
#include <io.h>
#include <windows.h>
#endif

#include <errno.h>
#include <unistd.h>

typedef struct remote_event remote_event;
typedef int(event_handler_func)(remote_fildes_t);

/* Tell create_file_handler what events we are interested in.  */

#define remote_READABLE (1 << 1)
#define remote_WRITABLE (1 << 2)
#define remote_EXCEPTION (1 << 3)

/* Events are queued by calling 'QUEUE_enque (remote_event_p, event_queue,
   file_event_ptr)' and serviced later
   on by do_one_event.  An event can be, for instance, a file
   descriptor becoming ready to be read.  Servicing an event simply
   means that the procedure PROC will be called.  We have 2 queues,
   one for file handlers that we listen to in the event loop, and one
   for the file handlers+events that are ready.  The procedure PROC
   associated with each event is always the same (handle_file_event).
   Its duty is to invoke the handler associated with the file
   descriptor whose state change generated the event, plus doing other
   cleanups and such.  */

typedef struct remote_event {
  /* Procedure to call to service this event.  */
  event_handler_func *proc;

  /* File descriptor that is ready.  */
  remote_fildes_t fd;
} * remote_event_p;

/* Information about each file descriptor we register with the event
   loop.  */

typedef struct file_handler {
  /* File descriptor.  */
  remote_fildes_t fd;

  /* Events we want to monitor.  */
  int mask;

  /* Events that have been seen since the last time.  */
  int ready_mask;

  /* Procedure to call when fd is ready.  */
  handler_func *proc;

  /* Argument to pass to proc.  */
  remote_client_data client_data;

  /* Was an error detected on this fd?  */
  int error;

  /* Next registered file descriptor.  */
  struct file_handler *next_file;
} file_handler;

DECLARE_QUEUE_P(remote_event_p);
static QUEUE(remote_event_p) *event_queue = NULL;
DEFINE_QUEUE_P(remote_event_p);

/* remote_notifier is just a list of file descriptors is interested
   in.  These are the input file descriptor, and the target file
   descriptor.  Each of the elements in the remote_notifier list is
   basically a description of what kind of events is interested
   in, for each fd.  */

static struct {
  /* Ptr to head of file handler list.  */
  file_handler *first_file_handler;

  /* Masks to be used in the next call to select.  Bits are set in
     response to calls to create_file_handler.  */
  fd_set check_masks[3];

  /* What file descriptors were found ready by select.  */
  fd_set ready_masks[3];

  /* Number of valid bits (highest fd value + 1). (for select) */
  int num_fds;
} remote_notifier;

/* Callbacks are just routines that are executed before waiting for the
   next event.  In this is struct remote_timer.  We don't need timers
   so rather than copy all that complexity , we provide what
   we need, but we do so in a way that if/when the day comes that we need
   that complexity, it'll be easier to add - replace callbacks with timers
   and use a delta of zero (which is all currently uses timers for anyway).

   PROC will be executed before goes to sleep to wait for the
   next event.  */

struct callback_event {
  int id;
  callback_handler_func *proc;
  remote_client_data *data;
  struct callback_event *next;
};

/* Table of registered callbacks.  */

static struct {
  struct callback_event *first;
  struct callback_event *last;

  /* Id of the last callback created.  */
  int num_callbacks;
} callback_list;

/* Free EVENT.  */

static void remote_event_free(struct remote_event *event) { free(event); }

void initialize_event_loop(void) {
  event_queue = QUEUE_alloc(remote_event_p, remote_event_free);
}

/* Process one event.  If an event was processed, 1 is returned
   otherwise 0 is returned.  Scan the queue from head to tail,
   processing therefore the high priority events first, by invoking
   the associated event handler procedure.  */

static int process_event(void) {
  /* Let's get rid of the event from the event queue.  We need to
     do this now because while processing the event, since the
     proc function could end up jumping out to the caller of this
     function.  In that case, we would have on the event queue an
     event which has been processed, but not deleted.  */
  if (!QUEUE_is_empty(remote_event_p, event_queue)) {
    remote_event *event_ptr = QUEUE_deque(remote_event_p, event_queue);
    event_handler_func *proc = event_ptr->proc;
    remote_fildes_t fd = event_ptr->fd;

    remote_event_free(event_ptr);
    /* Now call the procedure associated with the event.  */
    if ((*proc)(fd)) return -1;
    return 1;
  }

  /* This is the case if there are no event on the event queue.  */
  return 0;
}

/* Append PROC to the callback list.
   The result is the "id" of the callback that can be passed back to
   delete_callback_event.  */

int append_callback_event(callback_handler_func *proc,
                          remote_client_data data) {
  struct callback_event *event_ptr;

  event_ptr = malloc(sizeof(*event_ptr));
  event_ptr->id = callback_list.num_callbacks++;
  event_ptr->proc = proc;
  event_ptr->data = data;
  event_ptr->next = NULL;
  if (callback_list.first == NULL) callback_list.first = event_ptr;
  if (callback_list.last != NULL) callback_list.last->next = event_ptr;
  callback_list.last = event_ptr;
  return event_ptr->id;
}

/* Delete callback ID.
   It is not an error callback ID doesn't exist.  */

void delete_callback_event(int id) {
  struct callback_event **p;

  for (p = &callback_list.first; *p != NULL; p = &(*p)->next) {
    struct callback_event *event_ptr = *p;

    if (event_ptr->id == id) {
      *p = event_ptr->next;
      if (event_ptr == callback_list.last) callback_list.last = NULL;
      free(event_ptr);
      break;
    }
  }
}

/* Run the next callback.
   The result is 1 if a callback was called and event processing
   should continue, -1 if the callback wants the event loop to exit,
   and 0 if there are no more callbacks.  */

static int process_callback(void) {
  struct callback_event *event_ptr;

  event_ptr = callback_list.first;
  if (event_ptr != NULL) {
    callback_handler_func *proc = event_ptr->proc;
    remote_client_data *data = event_ptr->data;

    /* Remove the event before calling PROC,
       more events may get added by PROC.  */
    callback_list.first = event_ptr->next;
    if (callback_list.first == NULL) callback_list.last = NULL;
    free(event_ptr);
    if ((*proc)(data)) return -1;
    return 1;
  }

  return 0;
}

/* Add a file handler/descriptor to the list of descriptors we are
   interested in.  FD is the file descriptor for the file/stream to be
   listened to.  MASK is a combination of READABLE, WRITABLE,
   EXCEPTION.  PROC is the procedure that will be called when an event
   occurs for FD.  CLIENT_DATA is the argument to pass to PROC.  */

static void create_file_handler(remote_fildes_t fd, int mask,
                                handler_func *proc,
                                remote_client_data client_data) {
  file_handler *file_ptr;

  /* Do we already have a file handler for this file? (We may be
     changing its associated procedure).  */
  for (file_ptr = remote_notifier.first_file_handler; file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    if (file_ptr->fd == fd) break;

  /* It is a new file descriptor.  Add it to the list.  Otherwise,
     just change the data associated with it.  */
  if (file_ptr == NULL) {
    file_ptr = malloc(sizeof(*file_ptr));
    file_ptr->fd = fd;
    file_ptr->ready_mask = 0;
    file_ptr->next_file = remote_notifier.first_file_handler;
    remote_notifier.first_file_handler = file_ptr;

    if (mask & remote_READABLE)
      FD_SET(fd, &remote_notifier.check_masks[0]);
    else
      FD_CLR(fd, &remote_notifier.check_masks[0]);

    if (mask & remote_WRITABLE)
      FD_SET(fd, &remote_notifier.check_masks[1]);
    else
      FD_CLR(fd, &remote_notifier.check_masks[1]);

    if (mask & remote_EXCEPTION)
      FD_SET(fd, &remote_notifier.check_masks[2]);
    else
      FD_CLR(fd, &remote_notifier.check_masks[2]);

    if (remote_notifier.num_fds <= fd) remote_notifier.num_fds = fd + 1;
  }

  file_ptr->proc = proc;
  file_ptr->client_data = client_data;
  file_ptr->mask = mask;
}

/* Wrapper function for create_file_handler.  */

void add_file_handler(remote_fildes_t fd, handler_func *proc,
                      remote_client_data client_data) {
  create_file_handler(fd, remote_READABLE | remote_EXCEPTION, proc,
                      client_data);
}

/* Remove the file descriptor FD from the list of monitored fd's:
   i.e. we don't care anymore about events on the FD.  */

void delete_file_handler(remote_fildes_t fd) {
  file_handler *file_ptr, *prev_ptr = NULL;
  int i;

  /* Find the entry for the given file. */

  for (file_ptr = remote_notifier.first_file_handler; file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    if (file_ptr->fd == fd) break;

  if (file_ptr == NULL) return;

  if (file_ptr->mask & remote_READABLE)
    FD_CLR(fd, &remote_notifier.check_masks[0]);
  if (file_ptr->mask & remote_WRITABLE)
    FD_CLR(fd, &remote_notifier.check_masks[1]);
  if (file_ptr->mask & remote_EXCEPTION)
    FD_CLR(fd, &remote_notifier.check_masks[2]);

  /* Find current max fd.  */

  if ((fd + 1) == remote_notifier.num_fds) {
    remote_notifier.num_fds--;
    for (i = remote_notifier.num_fds; i; i--) {
      if (FD_ISSET(i - 1, &remote_notifier.check_masks[0]) ||
          FD_ISSET(i - 1, &remote_notifier.check_masks[1]) ||
          FD_ISSET(i - 1, &remote_notifier.check_masks[2]))
        break;
    }
    remote_notifier.num_fds = i;
  }

  /* Deactivate the file descriptor, by clearing its mask, so that it
     will not fire again.  */

  file_ptr->mask = 0;

  /* Get rid of the file handler in the file handler list.  */
  if (file_ptr == remote_notifier.first_file_handler)
    remote_notifier.first_file_handler = file_ptr->next_file;
  else {
    for (prev_ptr = remote_notifier.first_file_handler;
         prev_ptr->next_file != file_ptr; prev_ptr = prev_ptr->next_file)
      ;
    prev_ptr->next_file = file_ptr->next_file;
  }
  free(file_ptr);
}

/* Handle the given event by calling the procedure associated to the
   corresponding file handler.  Called by process_event indirectly,
   through event_ptr->proc.  EVENT_FILE_DESC is file descriptor of the
   event in the front of the event queue.  */

static int handle_file_event(remote_fildes_t event_file_desc) {
  file_handler *file_ptr;
  int mask;

  /* Search the file handler list to find one that matches the fd in
     the event.  */
  for (file_ptr = remote_notifier.first_file_handler; file_ptr != NULL;
       file_ptr = file_ptr->next_file) {
    if (file_ptr->fd == event_file_desc) {
      /* See if the desired events (mask) match the received
         events (ready_mask).  */

      if (file_ptr->ready_mask & remote_EXCEPTION) {
#if 0
        //fprintf(stderr, "Exception condition detected on fd %s\n",
        //        pfildes(file_ptr->fd));
#endif
        file_ptr->error = 1;
      } else
        file_ptr->error = 0;
      mask = file_ptr->ready_mask & file_ptr->mask;

      /* Clear the received events for next time around.  */
      file_ptr->ready_mask = 0;

      /* If there was a match, then call the handler.  */
      if (mask != 0) {
        if ((*file_ptr->proc)(file_ptr->error, file_ptr->client_data) < 0)
          return -1;
      }
      break;
    }
  }

  return 0;
}

/* Create a file event, to be enqueued in the event queue for
   processing.  The procedure associated to this event is always
   handle_file_event, which will in turn invoke the one that was
   associated to FD when it was registered with the event loop.  */

static remote_event *create_file_event(remote_fildes_t fd) {
  remote_event *file_event_ptr;

  file_event_ptr = malloc(sizeof(remote_event));
  file_event_ptr->proc = handle_file_event;
  file_event_ptr->fd = fd;
  return file_event_ptr;
}

/* Called by do_one_event to wait for new events on the monitored file
   descriptors.  Queue file events as they are detected by the poll.
   If there are no events, this function will block in the call to
   select.  Return -1 if there are no files descriptors to monitor,
   otherwise return 0.  */

static int wait_for_event(void) {
  file_handler *file_ptr;
  int num_found = 0;

  /* Make sure all output is done before getting another event.  */
  fflush(stdout);
  fflush(stderr);

  if (remote_notifier.num_fds == 0) return -1;

  remote_notifier.ready_masks[0] = remote_notifier.check_masks[0];
  remote_notifier.ready_masks[1] = remote_notifier.check_masks[1];
  remote_notifier.ready_masks[2] = remote_notifier.check_masks[2];
  num_found = select(remote_notifier.num_fds, &remote_notifier.ready_masks[0],
                     &remote_notifier.ready_masks[1],
                     &remote_notifier.ready_masks[2], NULL);

  /* Clear the masks after an error from select.  */
  if (num_found == -1) {
    FD_ZERO(&remote_notifier.ready_masks[0]);
    FD_ZERO(&remote_notifier.ready_masks[1]);
    FD_ZERO(&remote_notifier.ready_masks[2]);
#ifdef EINTR
    /* Dont print anything if we got a signal, let handle
       it.  */
    if (errno != EINTR) perror("select");
#endif
  }

  /* Enqueue all detected file events.  */

  for (file_ptr = remote_notifier.first_file_handler;
       file_ptr != NULL && num_found > 0; file_ptr = file_ptr->next_file) {
    int mask = 0;

    if (FD_ISSET(file_ptr->fd, &remote_notifier.ready_masks[0]))
      mask |= remote_READABLE;
    if (FD_ISSET(file_ptr->fd, &remote_notifier.ready_masks[1]))
      mask |= remote_WRITABLE;
    if (FD_ISSET(file_ptr->fd, &remote_notifier.ready_masks[2]))
      mask |= remote_EXCEPTION;

    if (!mask)
      continue;
    else
      num_found--;

    /* Enqueue an event only if this is still a new event for this
       fd.  */

    if (file_ptr->ready_mask == 0) {
      remote_event *file_event_ptr = create_file_event(file_ptr->fd);

      QUEUE_enque(remote_event_p, event_queue, file_event_ptr);
    }
    file_ptr->ready_mask = mask;
  }

  return 0;
}

/* Start up the event loop.  This is the entry point to the event
   loop.  */

void start_event_loop(void) {
  /* Loop until there is nothing to do.  This is the entry point to
     the event loop engine.  If nothing is ready at this time, wait
     for something to happen (via wait_for_event), then process it.
     Return when there are no longer event sources to wait for.  */

  while (1) {
    /* Any events already waiting in the queue?  */
    int res = process_event();

    /* Did the event handler want the event loop to stop?  */
    if (res == -1) return;

    if (res) continue;

    /* Process any queued callbacks before we go to sleep.  */
    res = process_callback();

    /* Did the callback want the event loop to stop?  */
    if (res == -1) return;

    if (res) continue;

    /* Wait for a new event.  If wait_for_event returns -1, we
       should get out because this means that there are no event
       sources left.  This will make the event loop stop, and the
       application exit.  */

    if (wait_for_event() < 0) return;
  }

  /* We are done with the event loop.  There are no more event sources
     to listen to.  So we exit .  */
}
