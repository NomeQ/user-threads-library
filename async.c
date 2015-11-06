/* CS 533 Project
 * Asynchronous I/O
 * Naomi Dickerson
 */

#include <aio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "scheduler.h" 

ssize_t read_wrap(int fd, void * buf, size_t count) {
    struct aiocb io_ctl;
    // man page recommends we 0 out the aiocb buffer
    // as well as the read buffer up to count. Not sure 
    // how else to ensure the buffer doesn't contain 
    // stale data from within this function
    memset(&io_ctl, 0, sizeof(struct aiocb));
    memset(buf, 0, count);
    // if file is seekable, set offset value
    off_t offset;
    offset = lseek(fd, 0, SEEK_CUR);
    // if there is an error seeking, set offset to 0. Don't
    // bother returning this error, as it's perfectly acceptable
    // to read from stdin or a pipe, which doesn't allow seeking
    if (offset == -1) {
        offset = 0;
    }
    // initialize the fields of the aiocb
    io_ctl.aio_fildes = fd;
    io_ctl.aio_buf = buf;
    io_ctl.aio_nbytes = count;
    io_ctl.aio_offset = offset;
    io_ctl.aio_reqprio = 0;
    io_ctl.aio_sigevent.sigev_notify = SIGEV_NONE;

    // now perform read
    int ret;
    ret = aio_read(&io_ctl);
    if (ret == -1) {
       // Leaving it up to the program to handle a read failure
       return ret; 
    }    
    
    // Poll until read is complete, yielding if still in progress
    int status;
    while ((status = aio_error(&io_ctl)) == EINPROGRESS) {
        yield();
    }
     
    // At this point the read has either completed successfully or
    // has an error to return
    ret = aio_return(&io_ctl);
   
    // Update the offset in the file descriptor if the read 
    // terminated successfully
    if (ret >= 0) {
        lseek(fd, ret, SEEK_CUR);
    }
    return ret;
}
