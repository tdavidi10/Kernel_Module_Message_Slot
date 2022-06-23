#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "message_slot.h"
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");


/* channel has id, next channel, file */
typedef struct channel_node {
    unsigned int channel_id;
    char message[BUF_LEN]; /* message itself */
    unsigned int message_len; /* length of message */
    struct channel_node *next;
} channel_node;

/* each message slot has a linked list of channels and minor number */
typedef struct message_slot {
    channel_node* channel_list;
} message_slot;

static message_slot* slots_arr[MAX_SLOTS];

/* data we want the device to remember */
typedef struct data {
    unsigned int minor;
    unsigned int channel_id;
} data;

static int created_m_slots = 0; /* indicated if we created already the m_slot data structure */


static int device_open( struct inode* inode, struct file*  file )
{   
    int i;
    channel_node* new_channel;
    unsigned int new_minor;
    data* data_ptr;
    data_ptr = (data*)kmalloc(sizeof(data), GFP_KERNEL);

    if(data_ptr == NULL)
    {
        return 1;
    }
    data_ptr->minor = (unsigned int)iminor(inode);
    data_ptr->channel_id = 0; /* given that there is no channel id 0, not defined by ioctl yet */ 
    file->private_data = (void*)data_ptr;
    
    if (created_m_slots == 0) { /* if we have not created the m_slots data structure yet */

        for (i = 0; i < MAX_SLOTS; i++) {
            slots_arr[i] = (message_slot*)kmalloc(sizeof(message_slot), GFP_KERNEL); /* allocate memory for the message slot */
            if (slots_arr[i] == NULL) { /* if kmalloc failed */
                return 1;
            }
            slots_arr[i]->channel_list = NULL; /* set the channel list to NULL */
        }
        created_m_slots = 1;
    }
    /* create the channel in rellevant minor slot*/
    new_channel = (channel_node*)kmalloc(sizeof(channel_node), GFP_KERNEL);
    if (new_channel == NULL) { /* if kmalloc failed */
        return 1;
    }
    new_channel->message_len = 0;
    new_minor = (unsigned int)iminor(inode);
    new_channel->channel_id = 0; /* set the channel id to 0, not defined by ioctl yet*/
    new_channel->next = (channel_node*)slots_arr[new_minor]->channel_list; /* set the next channel to the channel list head*/
    slots_arr[new_minor]->channel_list = new_channel; /* set the channel list head to the new channel */
    return 0;
}

static ssize_t device_read( struct file* file, char __user* buffer, size_t length, loff_t* offset )
{
        

    unsigned int channel_id = ((data*)(file->private_data))->channel_id; /* get the channel id from the data structure we remember */
    unsigned int minor = ((data*)(file->private_data))->minor; /* get the minor number from the */
    
    channel_node* channel = slots_arr[minor]->channel_list; /* get the channel list head */
    if (channel == NULL) { /* if the channel list is empty */
        return -EINVAL; 
    }
    if (channel_id == 0){
        return -EINVAL; 
    }
    while (channel->next!= NULL) { /* while we have not reached the end of the channel list */
        if (channel->channel_id == channel_id) { /* if we found the channel */
            break; /* break and now channel is the right one */
        }
        /* else */
        channel = channel->next; /* go to the next channel */
    }
    if (channel->channel_id!= channel_id) { /* if we did not find the channel - this is the last channel and is wrong*/
        return -EINVAL;  
    }
    /* now we have the channel we want to read from */
    if ((channel->message_len == 0) || (channel->message == NULL)) { /* if the channel is empty */
        return -EWOULDBLOCK; 
    }
    if (length < channel->message_len) { /* if the buffer provided is too small for the message we want to read*/
        return -ENOSPC; 
    }
    /* now we have the channel we want to read from and the buffer is big enough */
    if (copy_to_user(buffer, channel->message, channel->message_len)!= 0) { /* if we failed to copy the message to the buffer */
        return -EIO;         /* EIO-5-Input/output error */

    }
    return channel->message_len; /* return the message length - which is the num of bytes we read */
}

static ssize_t device_write( struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    
    /* defined the chennael node in open, now add data to it */
    unsigned int channel_id = ((data*)(file->private_data))->channel_id; /* get the channel id from the data structure we remember */
    unsigned int minor = ((data*)(file->private_data))->minor; /* get the minor number from the */
    channel_node* channel = slots_arr[minor]->channel_list; /* get the channel list head */

    if (channel == NULL) { /* If no channel has been set on the file descriptor - channel list is empty */
        return -EINVAL; 
    }
    if (channel_id == 0) { /* if the channel id is 0, not defined by ioctl yet */
        return -EINVAL; 
    }
    while (channel->next!= NULL) { /* while we have not reached the end of the channel list */
        if (channel->channel_id == channel_id) { /* if we found the channel */
            break; /* break and now channel is the right one */
        }
        /* else */
        channel = channel->next; /* go to the next channel */
    }
    
    if (channel->channel_id!= channel_id) { /* there is no channel with channel_id -> need to create channel with channel_id id */
        /* now we have the channel we want to write to */
        /* need to create new channel in the end of the list with channel_id id */
        channel_node* new_channel = (channel_node*)kmalloc(sizeof(channel_node), GFP_KERNEL);
        if (new_channel == NULL) { /* if kmalloc failed */
            return -ENOMEM;
        }
        channel->next = new_channel; /* set the next channel to the new channel */
        new_channel->channel_id = channel_id; /* set the channel id to the new channel */
        new_channel->message_len = 0; /* set the message length to 0 */
        new_channel->next = NULL; /* set the next channel to NULL */
        channel = new_channel; /* set the channel to the new channel */
 
    }
    /* now we have the channel we want to write to */
    if ((length > BUF_LEN) || (length == 0)) { /* if the message is too long or no message at all*/
        return -EMSGSIZE; 
    }
    /* now we have the channel we want to write to and the message is good */
    if (copy_from_user(channel->message, buffer, length)!= 0) { /* if we failed to copy the message from the buffer */
        /* EIO-5-Input/output error */
        return -EIO; 
    }
    channel->message_len = length; /* set the message length */
    return length; /* return the message length - which is the num of bytes we wrote */
}


/* define channel_id in data in private_data of file (channel_id) */ 
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param)
{

    if ((ioctl_command_id != MSG_SLOT_CHANNEL)||(ioctl_param == 0)) /* if its not a legit command of us or channel is 0*/
    {
        return -EINVAL;
    }
    ((data*)(file->private_data))->channel_id = ioctl_param; /* set the channel id in the data structure so we will remember */
    return 0;
}


/* close the device */
/* & freeing al memory allocated in private_data of dile*/
static int device_release(struct inode *inode, struct file *file) {
    kfree(file->private_data);
    return 0;
}

/* defining the functions of the device */
struct file_operations Fops =
{
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

/* INIT - so the kernel recognizes the device/module */
static int __init init_m(void){
    int res;
    res = -42;
    res = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);
    if (res < 0) {
        return res;
    }
    return 0;
}

/* EXIT - so the kernel can unload the module */
static void __exit cleanup_m(void){
    int i;
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    /* free al memory allocated */
    
    for (i = 0; i < MAX_SLOTS; i++) { /* for every message slot */
        channel_node* curr_channel = slots_arr[i]->channel_list; /* get the channel list head */
        while (curr_channel!= NULL) { /* while we have a channel */
            channel_node* next_channel = curr_channel->next; /* get the next channel */
            kfree(curr_channel); /* free the current channel */
            curr_channel = next_channel; /* set the current channel to the next channel */
        }
        kfree(slots_arr[i]); /* free the message slot */
    }
    //kfree(ms); /* free the m_slots */
    created_m_slots = 0; /* set the created_m_slots flag to 0 */
}

module_init(init_m);
module_exit(cleanup_m);