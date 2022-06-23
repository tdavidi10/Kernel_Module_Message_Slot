#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H
#include <linux/ioctl.h>

#define MAX_SLOTS 256 /* The major device number. */
#define MAJOR_NUM 235 /* The major device number. */
#define BUF_LEN 128 /* message is up to 128-BUF_LEN bytes */
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int) /* support ioctl command */
#define DEVICE_NAME "message_slot"


#endif
