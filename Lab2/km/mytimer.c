// Pree Simphliphan
// U01702082

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

// Initialize all variable max_timers is for storing current maximum timer while MAX_TIMERS is 5 based on instruction.
#define DEVICE_NAME "mytimer"
#define MAJOR_NUM 61
#define MAX_MSG_LEN 128
#define MAX_TIMERS 5
static int max_timers = 1;
char mode='l';
char response[MAX_MSG_LEN];

// object stucture for store each timer detail and data
struct ktimer {
    struct timer_list timer;
    char message[MAX_MSG_LEN];
    int duration;
  struct list_head list; // create list to store all timer in order
};

// Declare number of active number as global
static LIST_HEAD(timer_list);
static int active_timers = 0;

// Callback when timer ends
static void my_timer_callback(struct timer_list *t) {
    struct ktimer *entry = from_timer(entry, t, timer);

    printk(KERN_INFO "%s\n", entry->message);

    // Free memory and decrement number of active timer
    list_del(&entry->list);
    kfree(entry);
    active_timers--;
}

static ssize_t my_timer_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
    int sec, count;
    char input[MAX_MSG_LEN], msg[MAX_MSG_LEN];
   

    struct ktimer *entry = NULL;

    if (len > MAX_MSG_LEN - 1) return -EINVAL;
    if (copy_from_user(input, buffer, len)) return -EFAULT;

    input[len] = '\0';

    // Check if it's the "MAX" command when user do -m to change maximum number of timer
    if (sscanf(input, "MAX %d", &count) == 1) {
        // Handle if count is not in valiud range
        if (count < 1 || count > MAX_TIMERS) {
            return -EINVAL;
        }
        max_timers = count;
	// At first I have this to check and debug
        //printk(KERN_INFO "Max timers set to %d\n", count);
        return len;
    }

    if (strncmp(input, "l", 1) == 0 && strlen(input) == 1) {
        mode = 'l';
        //printk("Mode changed to list\n");
        return len;
    }


    // [^\n] include everything to msg except new line for some cases that have whitespace such as "Hello World!" 128 to avoid buffer problem I encountered before
    int parsed = sscanf(input, "%d %128[^\n]", &sec, msg);

    if (parsed == 2) {
        // Handle the "-s" option: Set a timer

      // P: to indicate the user to print this
        mode = 's';
	list_for_each_entry(entry, &timer_list, list) {
        if (strcmp(entry->message, msg) == 0) {
             // Remove the existing entry from the list
             list_del(&entry->list);

            // Reset the timer
            mod_timer(&entry->timer, jiffies + msecs_to_jiffies(sec * 1000));
            entry->duration = sec;

           // Find the correct position to reinsert in sorted order
           struct ktimer *pos;
           struct list_head *insert_pos = &timer_list; // Default to insert at the end

           list_for_each_entry(pos, &timer_list, list) {
            if (entry->timer.expires < pos->timer.expires) {
                insert_pos = &pos->list;
                break;
               }  
            }

        // Insert before `insert_pos` to maintain sorted order
         list_add_tail(&entry->list, insert_pos);

        // Use scnprintf
        memset(response, 0, sizeof(response));
        scnprintf(response, sizeof(response), "P:The timer \"%s\" was updated!\n", msg);

        return len;
          }
        }       



        // Check if we can create a new timer or not
        if (active_timers >= max_timers) {
	  if (max_timers == 1) {
	    memset(response, 0, sizeof(response));
	    scnprintf(response, sizeof(response), "P:A timer already exists!\n", msg);
	  } else {
	    memset(response, 0, sizeof(response));
	    scnprintf(response, sizeof(response), "P:%d timers already exist!\n", max_timers);
	  }
	  
	  // buffer  = scnprintf(response, sizeof(response), "A timer already exists!\n");
	    //if (copy_to_user(buffer, response, strlen(response) + 1)) {
            //       return -EFAULT;
            //}
            return -EINVAL;
        }

        // Allocate and set up a new timer if valid
        entry = kmalloc(sizeof(struct ktimer), GFP_KERNEL);
        if (!entry) return -ENOMEM;

        strncpy(entry->message, msg, MAX_MSG_LEN);
        entry->duration = sec;
        timer_setup(&entry->timer, my_timer_callback, 0);
        entry->timer.expires = jiffies + msecs_to_jiffies(sec * 1000);
        add_timer(&entry->timer);
        //list_add(&entry->list, &timer_list);
	
	
        // Insert into the list in earliest expiration first
        struct ktimer *pos;
        struct list_head *insert_pos = &timer_list; // Default to insert at the end

        list_for_each_entry(pos, &timer_list, list) {
        if (entry->timer.expires < pos->timer.expires) {
	  insert_pos = &pos->list;
             break;
           }
        }

        // Insert before `insert_pos` to maintain sorted order
        list_add_tail(&entry->list, insert_pos);


	// Update number of active timer
        active_timers++;

        return len;
    } 

    // If none of the above conditions are met
    return -EINVAL;
}

static ssize_t my_timer_read(struct file *filp, char __user *buffer, size_t len, loff_t *offset) {
    struct ktimer *entry;
    char output[512];  // Buffer to store the output
    int msg_len = 0;
    long time_left;
    
    memset(output, 0, sizeof(output));  // Ensure output buffer is clean

    if (list_empty(&timer_list)) {
        return 0;  // No timers to print exit function (and nothing to print)
    }

    // Handle reading based on mode
    if (mode == 'l') {
        // Loop through the timer list and construct output
        list_for_each_entry(entry, &timer_list, list) {
            time_left = (entry->timer.expires - jiffies) / HZ;
            if (time_left < 0) time_left = 0; // Avoid negative values

            msg_len += snprintf(output + msg_len, sizeof(output) - msg_len, "%s %ld\n", entry->message, time_left);
            
            // Ensure we don't write past the buffer limit
            if (msg_len >= sizeof(output)) {
                return -EFAULT;
            }
        }

        // Copy the data to userspace
        if (copy_to_user(buffer, output, msg_len)) {
            return -EFAULT;
        }

    } else if (mode == 's') {
        // Copy response to userspace
        if (copy_to_user(buffer, response, strlen(response))) {
            return -EFAULT;
        }

        // Clear response after printing
        memset(response, 0, sizeof(response));
    }

    return msg_len;
}



static int my_timer_open(struct inode *inode, struct file *file) {
    return 0;
}

static int my_timer_release(struct inode *inode, struct file *file) {
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = my_timer_write,
    .read = my_timer_read,
    .open = my_timer_open,
    .release = my_timer_release,
};

static int __init timer_init(void) {
  // Register device
    int result = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
    if (result < 0) {
        printk(KERN_ALERT "mytimer: cannot obtain major number %d\n", MAJOR_NUM);
        return result;
    }
    // Message to print
    printk(KERN_INFO "Inserted mytimer module\n");
    return 0;
}

static void __exit timer_exit(void) {
    struct ktimer *entry, *tmp;

    // Clean up all active timers and free memory
    list_for_each_entry_safe(entry, tmp, &timer_list, list) {
        del_timer_sync(&entry->timer);
        list_del(&entry->list);
        kfree(entry);
    }

    // unregister device
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk(KERN_INFO "Unloaded mytimer module\n");
}


module_init(timer_init);
module_exit(timer_exit);
