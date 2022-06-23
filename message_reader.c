#include "message_slot.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */





int main(int argc, char *argv[]){
    if (argc != 3) { /* argc has to be 3 */
        fprintf(stderr, "wrong number of arguments \n"); /* wrong number of arguments */
        exit(1);
    }
    /* argumets: file_path[1], channel_id[2] */
    char* file_path = argv[1];
    unsigned int channel_id = atoi(argv[2]); /* target message channel id (>=0) */
    int fd = open(file_path, O_RDONLY); /* open the file, read-only - we want read messages */
    if (fd < 0) {
        fprintf(stderr, "error - failed openning message slot device file %s\n", file_path); /* failed openning message slot device file */
        exit(1);
    }
    int ioctl_ret = ioctl(fd, MSG_SLOT_CHANNEL, channel_id); /* set the channel id */
    if (ioctl_ret < 0) {
        fprintf(stderr, "error - failed setting channel id %d\n", channel_id); /* failed setting channel id */
        exit(1);
    }
    char message_buffer[BUF_LEN]; /* message is up to 128- bytes */
    int bytes_read = read(fd, message_buffer, BUF_LEN); /* read the message */
    if (bytes_read < 0) { /* error */
        fprintf(stderr, "error - failed reading message\n"); /* failed reading message */
        exit(1);
    }
    close(fd); /* close the device */
    int bytes_written = write(1, message_buffer, bytes_read); /* write the message to 1-"standart output" */
    if ((bytes_written < 0) || (bytes_written != bytes_read)) { /* error or wrote more/less than the message */
        fprintf(stderr, "error - failed writing message %s\n", message_buffer); /* failed writing message */
        exit(1);
    }
    exit(0); /* exit with value 0*/
    










}



























