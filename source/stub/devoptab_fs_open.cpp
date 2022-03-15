#include "devoptab_fs.h"

int32_t __wut_fs_open(struct _reent* r, void* fileStruct, const char* path, int32_t flags, int32_t mode) {
    FSFileHandle fd;
    FSStatus status;
    FSCmdBlock cmd;
    const char* fsMode;
    __wut_fs_file_t* file;

    if (r->deviceData == NULL || !((devoptab_data_t*)r->deviceData)->initialized) {
        r->_errno = EINVAL;
        return -1;
    }
    devoptab_data_t* data = ((devoptab_data_t*)r->deviceData);

    if (!fileStruct || !path) {
        r->_errno = EINVAL;
        return -1;
    }

    // Map flags to open modes
    if (flags == 0) {
        fsMode = "r";
    }
    else if (flags == 2) {
        fsMode = "r+";
    }
    else if (flags == 0x601) {
        fsMode = "w";
    }
    else if (flags == 0x602) {
        fsMode = "w+";
    }
    else if (flags == 0x209) {
        fsMode = "a";
    }
    else if (flags == 0x20A) {
        fsMode = "a+";
    }
    else {
        r->_errno = EINVAL;
        return -1;
    }

    char* fixedPath = __wut_fs_fixpath(r, path);
    if (!fixedPath) {
        return -1;
    }

    // Open the file
    FSInitCmdBlock(&cmd);
    status = FSOpenFile(data->client, &cmd, fixedPath, fsMode, &fd,
        FS_ERROR_FLAG_ALL);
    free(fixedPath);
    if (status < 0) {
        r->_errno = __wut_fs_translate_error(status);
        return -1;
    }

    file = (__wut_fs_file_t*)fileStruct;
    file->fd = fd;
    file->flags = (flags & (O_ACCMODE | O_APPEND | O_SYNC));
    FSGetPosFile(data->client, &cmd, fd, &file->offset, FS_ERROR_FLAG_ALL);
    return 0;
}