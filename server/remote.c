
int transport_is_reliable = 0;

void remote_prepare(char *name)
{
    int port;
    char *port_str;
    struct sockaddr_in sockaddr;
    socklen_t tmp;
    char *port_end;
    
    /* get port string */
    port_str = strchr(name, ':');
    if(port_str == NULL)
    {
        transport_is_reliable = 0;
        return;
    }

    port = strtoul(port_str + 1, &port_end, 10);
    if(port_str[1] == '\0' || *port_end != '\0') {
        printf("Bad port argument: %s", name);
        exit(-1);
    }

    listen_desc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listen_desc == -1)
        perror("Can't open socket");

    /* Allow rapid reuse of this port. */
    tmp = 1;
    setsockopt(listen_desc, SOL_SOCKET, SO_REUSEADDR,(char *) &tmp, sizeof(tmp));

    sockaddr.sin_family = PF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listen_desc,(struct sockaddr *) &sockaddr, sizeof(sockaddr)) || listen(listen_desc, 1))
        perror("Can't bind address");

    transport_is_reliable = 1;
}

void remote_open(char *name)
{
    char *port_str;

    port_str = strchr(name, ':');
    if(port_str == NULL)
    {
        struct stat statbuf;

        if(stat(name, &statbuf) == 0 &&(S_ISCHR(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode)))
            remote_desc = open(name, O_RDWR);
        else
        {
            errno = EINVAL;
            remote_desc = -1;
        }

        if(remote_desc < 0)
            perror("Could not open remote device");

#ifdef HAVE_TERMIOS
        {
            struct termios termios;
            tcgetattr(remote_desc, &termios);

            termios.c_iflag = 0;
            termios.c_oflag = 0;
            termios.c_lflag = 0;
            termios.c_cflag &= ~(CSIZE | PARENB);
            termios.c_cflag |= CLOCAL | CS8;
            termios.c_cc[VMIN] = 1;
            termios.c_cc[VTIME] = 0;

            tcsetattr(remote_desc, TCSANOW, &termios);
        }
#endif

#ifdef HAVE_TERMIO
        {
            struct termio termio;
            ioctl(remote_desc, TCGETA, &termio);

            termio.c_iflag = 0;
            termio.c_oflag = 0;
            termio.c_lflag = 0;
            termio.c_cflag &= ~(CSIZE | PARENB);
            termio.c_cflag |= CLOCAL | CS8;
            termio.c_cc[VMIN] = 1;
            termio.c_cc[VTIME] = 0;

            ioctl(remote_desc, TCSETA, &termio);
        }
#endif
        fprintf(stderr, "Remote debugging using %s\n", name);

        enable_async_notification(remote_desc);

        /* Register the event loop handler.  */
        add_file_handler(remote_desc, handle_serial_event, NULL);
    }
    else
    {
        int port;
        socklen_t len;
        struct sockaddr_in sockaddr;

        len = sizeof(sockaddr);
        if(getsockname(listen_desc,(struct sockaddr *) &sockaddr, &len) < 0 || len < sizeof(sockaddr))
            perror("Can't determine port");
        port = ntohs(sockaddr.sin_port);

        fprintf(stderr, "Listening on port %d\n", port);
        fflush(stderr);

        /* Register the event loop handler.  */
        add_file_handler(listen_desc, handle_accept_event, NULL);
    }
}

void remote_close(void)
{
    /* logoff handler */
    delete_file_handler(remote_desc);
    close(remote_desc);
    remote_desc = INVALID_DESCRIPTOR;
    reset_readchar();
}

