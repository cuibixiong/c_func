void scan_dir (char *dir, char *key)
{
  DIR *dir;
  struct dirent *e;
  char dirname[PATH_MAX + 1];

  dir = opendir(dir);
  if (!dir) {
    return -1;
  }

  while ((e = readdir(dir)) != NULL)
  {
    // scan all dump_xx file.
    if (e->d_name[0] == '.')
      continue;

    if (!strstr(e->d_name, key))
      continue;

    snprintf(dirname, sizeof(dirname), "%s/%s",
             dir, e->d_name);

    fprintf_filtered (out, "[%s]\n", e->d_name);
  }

  closedir(dir);
  return 0;
}
