// Pree Simphliphan
// U01702082

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/signal.h>
#include <linux/pid.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/jiffies.h> /* jiffies */
#include <linux/sched.h> 
#include <linux/seq_file.h>
#include <linux/mutex.h>


#define DEVICE_NAME "mytimer"
#define PROC_NAME "mytimer"
#define MAJOR_NUM 61
#define MAX_TIMERS 1

static struct proc_dir_entry *proc_file;

static struct timer_list my_timer;
static struct pid *pid_struct = NULL;
static pid_t pid;
static struct fasync_struct *async_queue = NULL;

static char message[128] = "";
char mode='l';
char response[128];
static int active_timer = 0;
static int max_timers = 1;

static DEFINE_MUTEX(timer_mutex);

MODULE_LICENSE("Dual BSD/GPL");

static void mytimer_callback(struct timer_list *t) {
    struct siginfo info;
    struct task_struct *task;
    int ret = 0;
    static char proc_buffer[128];  // Buffer to store data for proc file
    static size_t proc_buffer_size = 0;


    // Ensure response is set correctly
    strcpy(response, message);

    // Free memory and decrement the number of active timers
    active_timer = 0;

    if (timer_pending(&my_timer)) {
        del_timer_sync(&my_timer);
    }

    // kill signal
    mode = 'k';
    // Send a signal to the user-space process

    if (async_queue)
        kill_fasync(&async_queue, SIGIO, POLL_IN);
    // memset(&info, 0, sizeof(struct siginfo));
    //info.si_signo = SIGIO;
    //info.si_code = SI_KERNEL;  // Indicate signal is from the kernel

    //ret = do_send_sig_info(SIGIO, &info, task, 0);
    //if (ret < 0) {
    //  printk(KERN_ERR "Failed to send SIGIO to user process\n");
    //}

    //printk(KERN_INFO "Checkpoint5\n");

    //get_task_cred(task);
}


static int mytimer_fasync(int fd, struct file *filp, int mode) {
    return fasync_helper(fd, filp, mode, &async_queue);
}

static int mytimer_release(struct inode *inode, struct file *filp) {
    mytimer_fasync(-1, filp, 0);
    return 0;
}

static ssize_t mytimer_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
  char input[128], msg[128];
  int sec, count;

    if (len > 127) return -EINVAL;
    if (copy_from_user(input, buf, len)) return -EFAULT;

    input[len] = '\0';

    // Check if it's the "MAX" command when user do -m to change maximum number of timer
    if (sscanf(input, "MAX %d", &count) == 1) {
        // Handle if count is not in valid range
        mode = 'm';
        if (count != 1) {
	  strcpy(response, "P:Error: multiple timers not supported\n");
        } else {
	  max_timers = count;
	}
	// At first I have this to check and debug
        //printk(KERN_INFO "Max timers set to %d\n", count);
        return len;
    }

    if (strncmp(input, "REMOVE", 6) == 0) {
      mode = 'r';
      if (timer_pending(&my_timer)) {
            del_timer_sync(&my_timer);
            active_timer = 0;
	    // For debugging
            //strcpy(response, "P:Timer removed!\n");
      } //else {
      //strcpy(response, "P:No active timer to remove!\n");
      //  }
      mutex_unlock(&timer_mutex);
        return len;
    }
    
    if (strncmp(input, "l", 1) == 0 && strlen(input) == 1) {
        mode = 'l';
        return len;
    }


    // [^\n] include everything to msg except new line for some cases that have whitespace such as "Hello World!" 128 to avoid buffer problem I encountered before
        // Handle setting a timer
    if (sscanf(input, "%d %128[^\n]", &sec, msg) == 2) {
        if (sec < 1 || sec > 86400)
            return -EINVAL;
       // Handle the "-s" option: Set a timer

      // P: to indicate the user to print this
        mode = 's';

	// If a timer is already active, check if the message is the same
        if (active_timer) {
            if (strcmp(message, msg) == 0) {
	      // to clear process that already run
	          if (timer_pending(&my_timer)) {
		    del_timer_sync(&my_timer);  // Remove any pending timers before resetting
		  }
                // Update the timer
                mod_timer(&my_timer, jiffies + msecs_to_jiffies(sec * 1000));
                scnprintf(response, sizeof(response), "P:The timer \"%s\" was updated!\n", msg);
		// get pid number
		pid = task_pid(current);
            } else {
                // Reject setting a new message when a timer is active
                scnprintf(response, sizeof(response), "P:Cannot add another timer!\n");
		mutex_unlock(&timer_mutex);
                return -EINVAL;
            }
        } else {
            // Start a new timer
            strcpy(message, msg);
            mod_timer(&my_timer, jiffies + msecs_to_jiffies(sec * 1000));
            active_timer = 1;
	    // get pid number
	    pid = task_pid(current);
        }
	mutex_unlock(&timer_mutex);
        return len;
    }

    // If none of the above conditions are met
    return -EINVAL;
}

static ssize_t mytimer_read(struct file *filp, char __user *buffer, size_t len, loff_t *offset) {
    char output[128];  // Buffer to store the output
    long time_left;
    int msg_len = 0;
    //printk("%s", response);
    memset(output, 0, sizeof(output));  // Ensure output buffer is clean

    // Handle reading based on mode
    if (mode == 'l' && active_timer) {
        // Calculate remaining time
        time_left = (my_timer.expires - jiffies) / HZ;
        if (time_left < 0) time_left = 0; // Avoid negative values

        // Format the response
        msg_len = scnprintf(output, sizeof(output), "%s %ld\n", message, time_left);

        // Copy to userspace
        if (copy_to_user(buffer, output, msg_len)) {
            return -EFAULT;
        }

    } else if ((mode == 's' && active_timer) || mode == 'm') {
        // Return the latest timer update response
        if (strlen(response) > 0) {
            if (copy_to_user(buffer, response, strlen(response))) {
                return -EFAULT;
            }
            memset(response, 0, sizeof(response)); // Clear response after printing
            return strlen(response);
        }
    } else if (mode == 'k') {
      //printk("%c %s", mode, response);
      if (copy_to_user(buffer, response, strlen(response))) {
                return -EFAULT;
      }
            memset(response, 0, sizeof(response)); // Clear response after printing
            return strlen(response);
    }

    return msg_len;
}


static int mytimer_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "[MODULE_NAME]: %s\n[MSEC]: %u\n", DEVICE_NAME, jiffies_to_msecs(jiffies));
    if (active_timer){
        long time_left = (my_timer.expires - jiffies) / HZ;
	seq_printf(m, "[PID]: %d\n[CMD]: %s\n[SEC]: %ld\n", pid_vnr(pid), message, time_left);
	}
	return 0;
}

static int mytimer_proc_open(struct inode *inode, struct file *filp) {
    return single_open(filp, mytimer_proc_show, NULL);
    // return 0;
}

static const struct file_operations mytimer_proc_ops = {
    .open = mytimer_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = mytimer_write,
    .open = mytimer_proc_open,
    .release = mytimer_release,
    .read = mytimer_read,
    .fasync = mytimer_fasync,
};

static int __init mytimer_init(void) {
    proc_file = proc_create(PROC_NAME, 0, NULL, &mytimer_proc_ops);
    register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
    timer_setup(&my_timer,mytimer_callback, 0);
  
        // Message to print
    printk(KERN_INFO "Inserted mytimer module\n");
    return 0;
}

static void __exit  mytimer_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    if (timer_pending(&my_timer)) {
        del_timer_sync(&my_timer);
    }

}

module_init(mytimer_init);
module_exit(mytimer_exit);
