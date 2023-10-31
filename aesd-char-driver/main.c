/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 * Modified by Tim Bailey, tiba6275@colorado.edu, 2023-29-10 for
 * AESD5713, Assignment 8
 * 
 * Referenced course material as well as ChatGPT for some code structure,
 * lib function explanations, logic questions, code cleanup, and commenting.
 *
 * For educational use only.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Tim Bailey");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    struct aesd_dev *dev;
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
    struct aesd_circular_buffer *circular_buffer = &dev->circular_buffer;
    struct aesd_buffer_entry *entry = NULL;
    size_t bytes_to_read;
    size_t bytes_read = 0;
    size_t entry_offset_byte;
    
    if (buf == NULL) {
        return -EFAULT; //Check for invalid user buffer
    }

    mutex_lock(&dev->lock); //Lock for synchronized access

    //Find entry in circular buffer at file position
    entry = aesd_circular_buffer_find_entry_offset_for_fpos(circular_buffer, *f_pos, &entry_offset_byte);
    
    if(entry == NULL) {
        mutex_unlock(&dev->lock); //Unlock and return if no entry found
        return 0;
    }

    bytes_to_read = entry->size - entry_offset_byte; //Calculate bytes to read

    if (bytes_to_read > count) {
        bytes_to_read = count; //Limit to requested count
    }

    //Copy data to user buffer
    if (copy_to_user(buf, entry->buffptr + entry_offset_byte, bytes_to_read)) {
        mutex_unlock(&dev->lock); //Unlock and return error if copy fails
        return -EFAULT;
    }

    *f_pos += bytes_to_read; //Update file position
    bytes_read = bytes_to_read; //Record bytes read

    mutex_unlock(&dev->lock); //Unlock after operation
    return bytes_read; //Return bytes read
}



ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                   loff_t *f_pos)
{
    struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
    struct aesd_circular_buffer *circular_buffer = &dev->circular_buffer;
    struct aesd_buffer_entry new_entry;
    ssize_t bytes_written = 0;
    size_t bytes_missing = 0;
    uint8_t in_pos = 0;
    char *tmp_buf = NULL;

    if (buf == NULL) {
        return -EFAULT; //Check for invalid user buffer
    }

    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS; //Return if lock interrupted
    }

    //Append to existing buffer if exists
    if (dev->partial_entry.buffptr != NULL) {
        tmp_buf = krealloc(dev->partial_entry.buffptr, dev->partial_entry.size + count, GFP_KERNEL);
        if (!tmp_buf) {
            bytes_written = -ENOMEM; //Return error if allocation fails
            mutex_unlock(&dev->lock);
            return bytes_written;
        }
        dev->partial_entry.buffptr = tmp_buf; //Update partial entry buffer

        //Copy user data to buffer and update size
        bytes_missing = copy_from_user(&dev->partial_entry.buffptr[dev->partial_entry.size], buf, count);
        bytes_written = count - bytes_missing;
        dev->partial_entry.size += bytes_written;
    } else {
        //Allocate new buffer for partial entry
        tmp_buf = kzalloc(count, GFP_KERNEL);
        if (!tmp_buf) {
            bytes_written = -ENOMEM; //Return error if allocation fails
            mutex_unlock(&dev->lock);
            return bytes_written;
        }

        //Copy user data to buffer
        bytes_missing = copy_from_user(tmp_buf, buf, count);
        bytes_written = count - bytes_missing;

        dev->partial_entry.buffptr = tmp_buf; //Set partial entry buffer
        dev->partial_entry.size = bytes_written; //Set partial entry size
    }

    //Check for newline in partial entry
    if (memchr(dev->partial_entry.buffptr, '\n', dev->partial_entry.size)) {
        //Handle circular buffer full condition
        if (circular_buffer->full) {
            in_pos = circular_buffer->in_offs;
            if (circular_buffer->entry[in_pos].buffptr != NULL) {
                kfree(circular_buffer->entry[in_pos].buffptr); //Free old buffer
            }
            circular_buffer->entry[in_pos].size = 0;
        }

        new_entry.buffptr = dev->partial_entry.buffptr; //Set new entry buffer
        new_entry.size = dev->partial_entry.size; //Set new entry size
        aesd_circular_buffer_add_entry(circular_buffer, &new_entry); //Add to circular buffer

        //Clear partial entry
        dev->partial_entry.buffptr = NULL;
        dev->partial_entry.size = 0;
    }

    mutex_unlock(&dev->lock); //Unlock after operation
    *f_pos += bytes_written; //Update file position
    return bytes_written; //Return bytes written
}





struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    aesd_circular_buffer_init(&aesd_device.circular_buffer);
    mutex_init(&aesd_device.lock);


    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    struct aesd_buffer_entry *entryptr;
    struct aesd_circular_buffer *buffer = &aesd_device.circular_buffer;
    uint8_t index;
     
    AESD_CIRCULAR_BUFFER_FOREACH(entryptr,buffer,index){
        if (entryptr->buffptr != NULL) {
            kfree(entryptr->buffptr);
            entryptr->size = 0;
        }
    }
    mutex_destroy(&aesd_device.lock);
     

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
