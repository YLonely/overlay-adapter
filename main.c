#define FUSE_USE_VERSION 29
#define _XOPEN_SOURCE 700
#include "config.h"
#include "copy_up.h"
#include "utils.h"
#include <dirent.h>
#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void *adapter_init(struct fuse_conn_info *conn) { return NULL; }

static int adapter_getattr(const char *path, struct stat *stbuf) {
    int res;
    char *t_path = source_file_path(path);
    res = lstat(t_path, stbuf);
    free(t_path);
    if (res == -1)
        return -errno;
    return 0;
}

static int adapter_access(const char *path, int mask) {
    int res;
    char *t_path = source_file_path(path);
    res = access(t_path, mask);
    free(t_path);
    if (res == -1)
        return -errno;
    return 0;
}

static int adapter_readlink(const char *path, char *buf, size_t size) {
    int res;
    char *t_path = source_file_path(path);
    res = readlink(t_path, buf, size - 1);
    free(t_path);
    if (res == -1)
        return -errno;
    buf[res] = 0;
    return 0;
}

static int adapter_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                           struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    (void)offset;
    (void)fi;

    char *t_path = source_file_path(path);
    dp = opendir(t_path);
    free(t_path);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int adapter_unlink(const char *path) { return -EOPNOTSUPP; }

static int adapter_symlink(const char *from, const char *to) {
    int res;
    res = symlink(from, to);
    if (res == -1)
        return -errno;
    return 0;
}

static int adapter_link(const char *from, const char *to) {
    int res;
    res = link(from, to);
    if (res == -1)
        return -errno;
    return 0;
}

static int adapter_open(const char *path, struct fuse_file_info *fi) {
    int res;
    char *source_path = source_file_path(path);
    char *upper_path = upper_file_path(path);
    char *t_path;
    if (exists(upper_path)) {
        t_path = upper_path;
    } else if (fi->flags & O_WRONLY || fi->flags & O_RDWR) {
        if (!exists(upper_path)) {
            if (copy_up(source_path, upper_path)) {
                free(source_path);
                free(upper_path);
                return -errno;
            }
        }
        t_path = upper_path;
    } else {
        t_path = source_path;
    }
    res = open(t_path, fi->flags);
    free(source_path);
    free(upper_path);
    if (res == -1)
        return -errno;
    fi->fh = res;
    return 0;
}

static int adapter_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd, res;
    char *t_path = source_file_path(path);
    char *upper_path = upper_file_path(path);
    if (exists(upper_path))
        t_path = upper_path;
    if (fi == NULL)
        fd = open(t_path, O_RDONLY);
    else
        fd = fi->fh;
    free(t_path);
    free(upper_path);
    if (fd == -1)
        return -errno;
    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    if (fi == NULL)
        close(fd);
    return res;
}

static int adapter_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char *upper_path = upper_file_path(path);
    int fd;
    if (!exists(upper_path)) {
        fprintf(stderr, "write file does not exist in upper");
        return -errno;
    }
    if (fi == NULL)
        fd = open(upper_path, O_WRONLY);
    else
        fd = fi->fh;
    if (fd == -1)
        return -errno;

    int res = pwrite(fd, buf, size, offset);
    if (res == -1)
        return -errno;
    if (fi == NULL)
        close(fd);
    return res;
}

static int adapter_statfs(const char *path, struct statvfs *stbuf) {
    int res;
    char *t_path = source_file_path(path);
    res = statvfs(t_path, stbuf);
    free(t_path);
    if (res == -1)
        return -errno;
    return 0;
}

static int adapter_release(const char *path, struct fuse_file_info *fi) {
    close(fi->fh);
    return 0;
}

static int adapter_truncate(const char *path, off_t size) {
    int res;
    char *upper_path = upper_file_path(path);
    char *source_path = NULL;
    if (!exists(upper_path)) {
        source_path = source_file_path(path);
        if (copy_up(source_path, upper_path)) {
            free(upper_path);
            free(source_path);
            return -errno;
        }
    }
    res = truncate(upper_path, size);
    free(upper_path);
    if (source_path)
        free(source_path);
    if (res == -1)
        return -errno;
    return 0;
}

static int adapter_create(const char *path, mode_t mode, struct fuse_file_info *fi) { return -EOPNOTSUPP; }

static int adapter_fsync(const char *path, int isdatasync, struct fuse_file_info *fi) { return 0; }

static int adapter_chown(const char *path, uid_t uid, gid_t gid) { return 0; }
static int adapter_chmod(const char *path, mode_t mode) { return 0; }
static int adapter_rename(const char *from, const char *to) { return -EOPNOTSUPP; }
static int adapter_mknod(const char *path, mode_t mode, dev_t rdev) { return -EOPNOTSUPP; }
static int adapter_mkdir(const char *path, mode_t mode) { return -EOPNOTSUPP; }
static int adapter_rmdir(const char *path) { return -EOPNOTSUPP; }
#ifdef HAVE_UTIMENSAT
static int adapter_utimens(const char *path, const struct timespec ts[2], struct fuse_file_info *fi) {
    return -EOPNOTSUPP;
}
#endif
#ifdef HAVE_POSIX_FALLOCATE
static int adapter_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi) {
    return -EOPNOTSUPP;
}
#endif

static struct fuse_operations adapter_oper = {
    .init = adapter_init,
    .getattr = adapter_getattr,
    .access = adapter_access,
    .readlink = adapter_readlink,
    .readdir = adapter_readdir,
    .mknod = adapter_mknod,
    .mkdir = adapter_mkdir,
    .symlink = adapter_symlink,
    .unlink = adapter_unlink,
    .rmdir = adapter_rmdir,
    .rename = adapter_rename,
    .link = adapter_link,
    .chmod = adapter_chmod,
    .chown = adapter_chown,
    .truncate = adapter_truncate,
#ifdef HAVE_UTIMENSAT
    .utimens = adapter_utimens,
#endif
    .open = adapter_open,
    .create = adapter_create,
    .read = adapter_read,
    .write = adapter_write,
    .statfs = adapter_statfs,
    .release = adapter_release,
    .fsync = adapter_fsync,
#ifdef HAVE_POSIX_FALLOCATE
    .fallocate = adapter_fallocate,
#endif
};

enum {
    KEY_SOURCE_DIR,
    KEY_UPPER_DIR,
};

static struct fuse_opt adapter_opts[] = {FUSE_OPT_KEY("--source=%s", KEY_SOURCE_DIR),
                                         FUSE_OPT_KEY("--upper=%s", KEY_UPPER_DIR), FUSE_OPT_END};

static int parse_opt(void *data, const char *arg, int key, struct fuse_args *outargs) {
    switch (key) {
    case KEY_SOURCE_DIR:
        if (!adapter_config.source_dir) {
            arg += strlen("--source=");
            adapter_config.source_dir = abs_dir_path(arg);
            adapter_config.source_dir_len = strlen(adapter_config.source_dir);
            return 0;
        }
        break;
    case KEY_UPPER_DIR:
        if (!adapter_config.upper_dir) {
            arg += strlen("--upper=");
            adapter_config.upper_dir = abs_dir_path(arg);
            adapter_config.upper_dir_len = strlen(adapter_config.upper_dir);
            return 0;
        }
        break;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &adapter_config, adapter_opts, parse_opt) == -1)
        exit(1);
    if (!adapter_config.source_dir) {
        fprintf(stderr, "missing source directory\n");
        exit(1);
    }
    // adapter_config.source_dir = "/root/adapter-test/source/";
    // adapter_config.source_dir_len = strlen(adapter_config.source_dir);
    // adapter_config.upper_dir = "/root/adapter-test/upper/";
    // adapter_config.upper_dir_len = strlen(adapter_config.upper_dir);
    umask(0);
    return fuse_main(args.argc, args.argv, &adapter_oper, NULL);
}