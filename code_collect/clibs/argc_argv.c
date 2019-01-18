dtu_cmds_rebuild_argc_argv(char *input, char ***argv)
  {
    int i, argc = 1;
    char *token = NULL, *saveptr = NULL;
    char *ptr[32] = { NULL, };

    token = strtok_r (input, " ", &saveptr);
    ptr[0] = strdup (token);

    while (NULL != (token = strtok_r (NULL, " ", &saveptr))) {
      ptr[argc] = strdup (token);
      argc++;
    }

    *argv = xmalloc (sizeof (char *) * (argc));
    memset (*argv, '1', sizeof (char *) * (argc));
    memcpy(*argv, ptr, sizeof (char *) * (argc));

    return argc;
  }


int main()
{
  int argc, char **argv;
  arg_c = dtu_cmds_rebuild_argc_argv (arg, &arg_v);
}
