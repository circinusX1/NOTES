
///////////////////////////////////////////////////////////////////////////////
static int get_file_mtime(struct timespec64 *mtime, int findex)
{
    struct path path;
    struct kstat statx;
    int err = -ENOENT;

    if (kern_path(FILES[findex], LOOKUP_FOLLOW, &path) == 0)
    {
        err = vfs_getattr(&path, &statx, ATTR_MTIME, 0);

        if (err == 0)
        {
            mtime->tv_sec = statx.mtime.tv_sec;
            mtime->tv_nsec = statx.mtime.tv_nsec;
        }
        else
        {
            pr_err( "%sFailed to get attributes for file %s (Error: %d)\n", log_prefix, FILES[findex], err);
        }

        path_put(&path);
    } else {
        err = -ENOENT;
    }

    return err;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static int monitor_thread_fn(void *data)
{
    struct timespec64 current_mtime;
    int err,i;
    const char* fname;

    pr_info( "%sMonitor thread started. Watching: %s\n", log_prefix, BASH_FILE);
    pr_info( "%sChecking every %dms.\n", log_prefix, MONITOR_INTERVAL_MS);

    // Initial check to set the baseline mtime
    for(i=0; i<FILESNUMS; i++)
    {
        if(FILES[i]==NULL)
            continue;
        err = get_file_mtime(&current_mtime, i);
        if (err == 0)
        {
            last_mtime_sec[i] = current_mtime.tv_sec;
            last_mtime_nsec[i] = current_mtime.tv_nsec;
            pr_info( "%s Initial timestamp set: %lld.%09llu\n",
                   FILES[i], last_mtime_sec, last_mtime_nsec);
        }
        else
        {
            pr_err( "Target file %s not found at startup. Will wait for creation.\n",
                   log_prefix, FILES[i]);
        }
    }

    while (!kthread_should_stop())
    {
        for(i=0;i<FILESNUMS;i++)
        {
            if(FILES[i]==NULL)
              continue;

            msleep(1000);
            err = get_file_mtime(&current_mtime, i);

            if (err == 0)
            {
                if (current_mtime.tv_sec != last_mtime_sec[i] ||
                    current_mtime.tv_nsec != last_mtime_nsec[i])
                {

                    pr_err( "FILE %s CHANGED\n", FILES[i]);
                    last_mtime_sec[i] = current_mtime.tv_sec;
                    last_mtime_nsec[i] = current_mtime.tv_nsec;
                }
            }
            else if (err == -ENOENT && last_mtime_sec != 0)
            {
                pr_err( "File %s was DELETED.\n", FILES[i]);
                last_mtime_sec[i] = 0;
                last_mtime_nsec[i] = 0;
            }
            if(kthread_should_stop())
              break;
        }
    }

    pr_info( "%sMonitor thread terminated.\n", log_prefix);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
static int delete_file(char *path)
{
    char *argv[] = { "/bin/unlink", path, NULL };
    return call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static int is_file(const char *path)
{
    struct file *filp;
    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp))
    {
        pr_err("Cannot open file: %s\n", path);
        return 1;
    }
    filp_close(filp, NULL);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
static int write_file_buff(const char* filename, const char* content)
{
    struct file *file = NULL;
    loff_t pos = 0; // File position offset
    ssize_t written;
    size_t len = strlen(content);
    int rv = 0;

    file = filp_open(filename, O_CREAT | O_RDWR, 0777);

    if (IS_ERR(file)) {
        pr_err("open %s. Error: %ld\n", filename, PTR_ERR(file));
        return -EIO;
    }

    written += kernel_write(file, content, len, &pos);
    msleep(100);

    if (written != (len))
    {
        rv = -2;
        pr_err("wrote %zd of %zu bytes.\n", written, len);
    }

    filp_close(file, NULL);
    pr_info(">>>> %s written with: %s", filename, content);
    return rv;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
static ssize_t read_file_buff(char* file, char* local, int size)
{
    ssize_t read = 0;
    loff_t pos = 0;

    struct file *filp = filp_open(file, O_RDONLY, 0);

    if (IS_ERR(filp))
    {
        pr_err("Cannot open file: %s\n", file);
        return 0;
    }
    read = kernel_read(filp, local, size, &pos);
    filp_close(filp, NULL);
    if (read < 0)
    {
        pr_err("kernel_read() error: %zd\n", read);
        return 0;
    }
    return read;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static int kexec_proc(const char *path, ...)
{
    va_list ap;
    char    *argv[MAX_ARGS];
    int     i = 0,rv;
    char    *p;
    char    trace[256] = {0};

    va_start(ap, path);
    argv[i++] = (char *)path;
    strcat(trace,path);
    strcat(trace," ");

    while (i < (MAX_ARGS - 1))
    {
        p = va_arg(ap, char *);
        if (!p)
            break;
        argv[i++] = p;
        strcat(trace,p);
        strcat(trace," ");
    }
    argv[i] = NULL;
    va_end(ap);

    pr_info("Exec: %s", trace);
    int rve =  call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    pr_info("Returned %d\n", rve);
    return rve;
}

static int kexec_proc2(const char *path, ...)
{
    va_list ap;
    int     i = 0,rv;
    char    *p;
    char    trace[256] = {0};

    va_start(ap, path);

    strcat(trace,"#!/bin/bash\n");
    strcat(trace,path);
    strcat(trace," ");

    while (i < (MAX_ARGS - 1))
    {
        p = va_arg(ap, char *);
        if (!p)
            break;
        strcat(trace,p);
        strcat(trace," ");
    }
    va_end(ap);
    strcat(trace,"\n");
    pr_info("Exec2: %s", trace);

    write_file_buff(TMP_BASH, trace);

    return kexec_proc("/bin/bash",TMP_BASH,NULL);

}
