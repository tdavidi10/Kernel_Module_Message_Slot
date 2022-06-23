#include "message_slot.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */

int main(int argc, char *argv[]){
    if (argc != 4) { /* argc has to be 4 */
        fprintf(stderr, "wrong number of arguments\n"); /* wrong number of arguments */
        exit(1);
    }
    /* argumets: file_path[1], channel_id[2], message[3] */
    char* file_path = argv[1];
    unsigned int channel_id = atoi(argv[2]); /* target message channel id (>=0) */
    char* message = argv[3]; /* message to pass */
    int fd = open(file_path, O_WRONLY); /* open the file, write-only because we want do overwrite new messages */
    if (fd < 0) {
        fprintf(stderr, "error - failed openning message slot device file %s\n", file_path); /* failed openning message slot device file */
        exit(1);
    }
    int ioctl_ret = ioctl(fd, MSG_SLOT_CHANNEL, channel_id); /* set the channel id */
    if (ioctl_ret < 0) {
        fprintf(stderr, "error - failed setting channel id %d\n", channel_id); /* failed setting channel id */
        exit(1);
    }
    int bytes_written = write(fd, message, strlen(message)); /* write the message */
    if ((bytes_written < 0) || (bytes_written != strlen(message))) { /* error or wrote not enough */
        fprintf(stderr, "error - failed writing message %s\n", message); /* failed writing message */
        exit(1);
    }
    close(fd); /* close the file */
    exit(0); /* exit with value 0*/

}

























