
int server_main(int argc, char *argv[])
{
    remote_prepare(port);
    while(1) {
        remote_open(port);
    }

    remote_close();
}

