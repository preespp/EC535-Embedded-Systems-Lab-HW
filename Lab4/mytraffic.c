/***************************************************
 * mytraffic.c (Hardcoded GPIO version, no DT)
 *
 * A kernel module for controlling a "traffic light"
 * on the BeagleBone Black (or similar) by using
 * fixed GPIO numbers for LEDs & buttons.
 *
 * Now with "lightbulb check" extra-credit feature:
 * If both BTN0 and BTN1 are held simultaneously,
 * all lights are on. Releasing both resets to
 * normal mode at 1Hz, with no pedestrian waiting.
 *
 * Author: Xingjian Jiang/Pree Simphliphan
 * License: GPL
 ***************************************************/

 #include <linux/module.h>
 #include <linux/init.h>
 #include <linux/kernel.h>
 #include <linux/fs.h>
 #include <linux/device.h>
 #include <linux/cdev.h>
 #include <linux/errno.h>
 #include <linux/uaccess.h>
 
 #include <linux/gpio.h>            // for gpio_request, gpio_free, etc.
 #include <linux/gpio/consumer.h>   // for gpio_to_desc, gpiod_*
 #include <linux/interrupt.h>
 #include <linux/irq.h>
 #include <linux/hrtimer.h>
 #include <linux/ktime.h>
 
 /* --- HARDCODED GPIO NUMBERS (Adjust for your wiring) --- */
 // Example: Red LED = 67, Yellow = 68, Green = 44, BTN0 = 26, BTN1 = 46
 #define RED_LED_GPIO   67
 #define YEL_LED_GPIO   68
 #define GRN_LED_GPIO   44
 #define BTN0_GPIO      26
 #define BTN1_GPIO      46
 
 /* ---------- INTERNAL STRUCT ---------- */
 struct mytraffic_state {
     // Character device stuff
     struct class   *dev_class;
     struct cdev     cdev;
     dev_t           devt;
 
     // Descriptors for LEDs & buttons
     struct gpio_desc *red_led;
     struct gpio_desc *yel_led;
     struct gpio_desc *grn_led;
     struct gpio_desc *btn0;
     struct gpio_desc *btn1;
 
     // IRQs
     int btn0_irq;
     int btn1_irq;
 
     // Traffic light modes
     enum { MODE_NORMAL = 0, MODE_FLASH_RED, MODE_FLASH_YEL } mode;
     bool pedestrian_waiting;
 
     // The extra-credit "lightbulb check" flag
     bool all_on_check;
 
     // Timer for controlling cycle rate
     struct hrtimer cycle_timer;
     unsigned int cycle_rate;   // 1..9 Hz
     unsigned int cycle_count;  // used in NORMAL mode
 };
 
 static struct mytraffic_state *mtstate;
 
 /* FORWARD DECLARATIONS */
 static enum hrtimer_restart mytraffic_timer_cb(struct hrtimer *timer);
 static irqreturn_t btn0_isr(int irq, void *dev_id);
 static irqreturn_t btn1_isr(int irq, void *dev_id);
 
 /* Helper: set LEDs */
 static void set_leds(bool r, bool y, bool g)
 {
     if (!mtstate) return;
     gpiod_set_value(mtstate->red_led, r);
     gpiod_set_value(mtstate->yel_led, y);
     gpiod_set_value(mtstate->grn_led, g);
 }
 
 /* Timer callback */
 static enum hrtimer_restart mytraffic_timer_cb(struct hrtimer *timer)
 {
     ktime_t now, interval;
     unsigned long ns_period;
 
     if (!mtstate)
         return HRTIMER_NORESTART;
 
     now = ktime_get();
     ns_period = (1000000000UL / mtstate->cycle_rate);
     interval = ktime_set(0, ns_period);
     hrtimer_forward(timer, now, interval);
 
     // If "lightbulb check" is active => keep all lights on
     if (mtstate->all_on_check) {
         set_leds(true, true, true);
         return HRTIMER_RESTART;
     }
 
     switch (mtstate->mode) {
     case MODE_NORMAL:
     {
         static bool ped_seq_active = false;
         static int ped_seq_count = 0;
         const int total_steps = 6; // G=3, Y=1, R=2 => 6 steps
 
         mtstate->cycle_count++;
         {
             int step = mtstate->cycle_count % total_steps;
 
             if (ped_seq_active) {
                 // Red+Yellow for 5 cycles
                 set_leds(true, true, false);
                 ped_seq_count++;
                 if (ped_seq_count >= 5) {
                     ped_seq_active = false;
                     ped_seq_count = 0;
                     mtstate->cycle_count = 0; // reset to green
                 }
             } else {
                 // normal cycle: step 0..2=Green, 3=Yellow, 4..5=Red
                 if (step < 3) {
                     // green
                     set_leds(false, false, true);
                 } else if (step == 3) {
                     // yellow
                     set_leds(false, true, false);
                 } else {
                     // red
                     set_leds(true, false, false);
                     // if step==4 => next 2 cycles are red
                     // if pedestrian waiting, do 5 cycles red+yellow
                     if ((step == 4) && mtstate->pedestrian_waiting) {
                         ped_seq_active = true;
                         mtstate->pedestrian_waiting = false;
                         ped_seq_count = 0;
                     }
                 }
             }
         }
         break;
     }
     case MODE_FLASH_RED:
     {
         static bool red_on;
         red_on = !red_on;
         set_leds(red_on, false, false);
         break;
     }
     case MODE_FLASH_YEL:
     {
         static bool yel_on;
         yel_on = !yel_on;
         set_leds(false, yel_on, false);
         break;
     }
     default:
         break;
     }
 
     return HRTIMER_RESTART;
 }
 
 /* BTN0 ISR => mode switch or "lightbulb check" */
 static irqreturn_t btn0_isr(int irq, void *dev_id)
 {
     if (!mtstate)
         return IRQ_HANDLED;
 
     // Check if both buttons are currently pressed
     {
         bool val0 = gpiod_get_value(mtstate->btn0);
         bool val1 = gpiod_get_value(mtstate->btn1);
 
         if (val0 && val1) {
             // Both pressed => "lightbulb check"
             mtstate->all_on_check = true;
             return IRQ_HANDLED;
         } else {
             // Not both pressed
             if (mtstate->all_on_check) {
                 // We were in all_on_check => now released => reset everything
                 mtstate->all_on_check = false;
                 mtstate->mode = MODE_NORMAL;
                 mtstate->cycle_rate = 1; // back to 1 Hz
                 mtstate->cycle_count = 0;
                 mtstate->pedestrian_waiting = false;
                 return IRQ_HANDLED;
             }
             // Otherwise, do the normal mode cycle
         }
     }
 
     // Round-robin mode switching
     switch (mtstate->mode) {
     case MODE_NORMAL:
         mtstate->mode = MODE_FLASH_RED;
         break;
     case MODE_FLASH_RED:
         mtstate->mode = MODE_FLASH_YEL;
         break;
     case MODE_FLASH_YEL:
         mtstate->mode = MODE_NORMAL;
         break;
     }
     mtstate->cycle_count = 0;
     return IRQ_HANDLED;
 }
 
 /* BTN1 ISR => set pedestrian_waiting in normal mode, or "lightbulb check" */
 static irqreturn_t btn1_isr(int irq, void *dev_id)
 {
     if (!mtstate)
         return IRQ_HANDLED;
 
     // Check if both buttons are currently pressed
     {
         bool val0 = gpiod_get_value(mtstate->btn0);
         bool val1 = gpiod_get_value(mtstate->btn1);
 
         if (val0 && val1) {
             // Both pressed => "lightbulb check"
             mtstate->all_on_check = true;
             return IRQ_HANDLED;
         } else {
             // Not both pressed
             if (mtstate->all_on_check) {
                 // We were in all_on_check => now released => reset everything
                 mtstate->all_on_check = false;
                 mtstate->mode = MODE_NORMAL;
                 mtstate->cycle_rate = 1; // back to 1 Hz
                 mtstate->cycle_count = 0;
                 mtstate->pedestrian_waiting = false;
                 return IRQ_HANDLED;
             }
             // Otherwise, do normal pedestrian logic
         }
     }
 
     // If in normal mode, set pedestrian waiting
     if (mtstate->mode == MODE_NORMAL) {
         mtstate->pedestrian_waiting = true;
     }
     return IRQ_HANDLED;
 }
 
 /* --------------- CHAR DEVICE OPS --------------- */
 static ssize_t mytraffic_read(struct file *f, char __user *buf, size_t len, loff_t *off)
 {
     char kbuf[128];
     int r, y, g;
 
     if (!mtstate) return 0;
     if (*off > 0) return 0; // single read
 
     r = gpiod_get_value(mtstate->red_led);
     y = gpiod_get_value(mtstate->yel_led);
     g = gpiod_get_value(mtstate->grn_led);
 
     snprintf(kbuf, sizeof(kbuf),
              "mode: %s\n"
              "rate: %u Hz\n"
              "lights: red=%s, yellow=%s, green=%s\n"
              "pedestrian: %s\n",
              (mtstate->mode == MODE_NORMAL)     ? "normal" :
              (mtstate->mode == MODE_FLASH_RED)  ? "flash-red" : "flash-yellow",
              mtstate->cycle_rate,
              r ? "on" : "off",
              y ? "on" : "off",
              g ? "on" : "off",
              mtstate->pedestrian_waiting ? "waiting" : "none"
     );
 
     if (len < strlen(kbuf) + 1)
         return -ENOSPC;
     if (copy_to_user(buf, kbuf, strlen(kbuf) + 1))
         return -EFAULT;
 
     *off += (strlen(kbuf) + 1);
     return (strlen(kbuf) + 1);
 }
 
 static ssize_t mytraffic_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
 {
     char ubuf[32];
     unsigned int val;
 
     if (!mtstate) return -ENODEV;
     if (len > 31) return -EINVAL;
 
     if (copy_from_user(ubuf, buf, len))
         return -EFAULT;
     ubuf[len] = '\0';
 
     if (sscanf(ubuf, "%u", &val) == 1) {
         if (val >= 1 && val <= 9)
             mtstate->cycle_rate = val;
     }
     return len;
 }
 
 static int mytraffic_open(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 static int mytraffic_release(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 static struct file_operations mytraffic_fops = {
     .owner   = THIS_MODULE,
     .read    = mytraffic_read,
     .write   = mytraffic_write,
     .open    = mytraffic_open,
     .release = mytraffic_release,
 };
 
 /* ================= MODULE INIT ================= */
 static int __init mytraffic_init(void)
 {
     ktime_t ktime_interval;
     unsigned long ns_period;
     int ret;
 
     mtstate = kzalloc(sizeof(*mtstate), GFP_KERNEL);
     if (!mtstate)
         return -ENOMEM;
 
     // 1) register char device region
     ret = alloc_chrdev_region(&mtstate->devt, 0, 1, "mytraffic");
     if (ret) {
         pr_err("alloc_chrdev_region failed\n");
         goto fail_alloc;
     }
 
     // 2) cdev init + add
     cdev_init(&mtstate->cdev, &mytraffic_fops);
     mtstate->cdev.owner = THIS_MODULE;
     ret = cdev_add(&mtstate->cdev, mtstate->devt, 1);
     if (ret) {
         pr_err("cdev_add failed\n");
         goto fail_cdev_add;
     }
 
     // 3) create class + device => /dev/mytraffic
     mtstate->dev_class = class_create(THIS_MODULE, "mytraffic_class");
     if (IS_ERR(mtstate->dev_class)) {
         pr_err("class_create failed\n");
         ret = PTR_ERR(mtstate->dev_class);
         goto fail_class;
     }
     device_create(mtstate->dev_class, NULL, mtstate->devt, NULL, "mytraffic");
 
     // 4) request + map the 5 GPIOS
     ret = gpio_request(RED_LED_GPIO,  "red-led");
     if (ret) { pr_err("Failed request red %d\n", RED_LED_GPIO); goto fail_gpio; }
     mtstate->red_led = gpio_to_desc(RED_LED_GPIO);
     if (!mtstate->red_led) { ret = -ENODEV; pr_err("gpio_to_desc red fail\n"); goto fail_gpio; }
     gpiod_direction_output(mtstate->red_led, 0);
 
     ret = gpio_request(YEL_LED_GPIO, "yel-led");
     if (ret) { pr_err("Failed request yel %d\n", YEL_LED_GPIO); goto fail_gpio; }
     mtstate->yel_led = gpio_to_desc(YEL_LED_GPIO);
     if (!mtstate->yel_led) { ret = -ENODEV; pr_err("gpio_to_desc yel fail\n"); goto fail_gpio; }
     gpiod_direction_output(mtstate->yel_led, 0);
 
     ret = gpio_request(GRN_LED_GPIO, "grn-led");
     if (ret) { pr_err("Failed request grn %d\n", GRN_LED_GPIO); goto fail_gpio; }
     mtstate->grn_led = gpio_to_desc(GRN_LED_GPIO);
     if (!mtstate->grn_led) { ret = -ENODEV; pr_err("gpio_to_desc grn fail\n"); goto fail_gpio; }
     gpiod_direction_output(mtstate->grn_led, 0);
 
     ret = gpio_request(BTN0_GPIO, "btn0");
     if (ret) { pr_err("Failed request btn0 %d\n", BTN0_GPIO); goto fail_gpio; }
     mtstate->btn0 = gpio_to_desc(BTN0_GPIO);
     if (!mtstate->btn0) { ret = -ENODEV; pr_err("gpio_to_desc btn0 fail\n"); goto fail_gpio; }
     gpiod_direction_input(mtstate->btn0);
 
     ret = gpio_request(BTN1_GPIO, "btn1");
     if (ret) { pr_err("Failed request btn1 %d\n", BTN1_GPIO); goto fail_gpio; }
     mtstate->btn1 = gpio_to_desc(BTN1_GPIO);
     if (!mtstate->btn1) { ret = -ENODEV; pr_err("gpio_to_desc btn1 fail\n"); goto fail_gpio; }
     gpiod_direction_input(mtstate->btn1);
 
     // 5) set up IRQs
     mtstate->btn0_irq = gpiod_to_irq(mtstate->btn0);
     mtstate->btn1_irq = gpiod_to_irq(mtstate->btn1);
     if (mtstate->btn0_irq < 0 || mtstate->btn1_irq < 0) {
         pr_err("gpiod_to_irq failed\n");
         ret = -EINVAL;
         goto fail_gpio;
     }
     ret = request_irq(mtstate->btn0_irq, btn0_isr,
                       IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                       "mytraffic_btn0", NULL);
     if (ret) {
         pr_err("request_irq btn0 failed\n");
         goto fail_gpio;
     }
     ret = request_irq(mtstate->btn1_irq, btn1_isr,
                       IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                       "mytraffic_btn1", NULL);
     if (ret) {
         pr_err("request_irq btn1 failed\n");
         free_irq(mtstate->btn0_irq, NULL);
         goto fail_gpio;
     }
 
     // 6) init state + start hrtimer
     mtstate->mode = MODE_NORMAL;
     mtstate->cycle_rate = 1; // default 1 Hz
     mtstate->cycle_count = 0;
     mtstate->pedestrian_waiting = false;
     mtstate->all_on_check = false;
 
     ns_period = (1000000000UL / mtstate->cycle_rate);
     ktime_interval = ktime_set(0, ns_period);
     hrtimer_init(&mtstate->cycle_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
     mtstate->cycle_timer.function = mytraffic_timer_cb;
     hrtimer_start(&mtstate->cycle_timer, ktime_interval, HRTIMER_MODE_REL);
 
     pr_info("mytraffic: module loaded (hardcoded GPIO, with lightbulb check)\n");
     return 0;
 
 fail_gpio:
     pr_err("GPIO or IRQ setup error => cleaning up...\n");
     // free requested IRQs if any
     if (mtstate->btn0_irq >= 0) free_irq(mtstate->btn0_irq, NULL);
     if (mtstate->btn1_irq >= 0) free_irq(mtstate->btn1_irq, NULL);
 
     // free gpios
     gpio_free(RED_LED_GPIO);
     gpio_free(YEL_LED_GPIO);
     gpio_free(GRN_LED_GPIO);
     gpio_free(BTN0_GPIO);
     gpio_free(BTN1_GPIO);
 
     device_destroy(mtstate->dev_class, mtstate->devt);
     class_destroy(mtstate->dev_class);
 fail_class:
     cdev_del(&mtstate->cdev);
 fail_cdev_add:
     unregister_chrdev_region(mtstate->devt, 1);
 fail_alloc:
     kfree(mtstate);
     return ret;
 }
 
 static void __exit mytraffic_exit(void)
 {
     if (!mtstate) return;
 
     // Cancel timer
     hrtimer_cancel(&mtstate->cycle_timer);
 
     // free IRQs
     free_irq(mtstate->btn0_irq, NULL);
     free_irq(mtstate->btn1_irq, NULL);
 
     // turn off LEDs
     set_leds(false, false, false);
 
     // free gpios
     gpio_free(RED_LED_GPIO);
     gpio_free(YEL_LED_GPIO);
     gpio_free(GRN_LED_GPIO);
     gpio_free(BTN0_GPIO);
     gpio_free(BTN1_GPIO);
 
     // remove device node /dev/mytraffic
     device_destroy(mtstate->dev_class, mtstate->devt);
     class_destroy(mtstate->dev_class);
     cdev_del(&mtstate->cdev);
     unregister_chrdev_region(mtstate->devt, 1);
 
     kfree(mtstate);
     pr_info("mytraffic: module unloaded\n");
 }
 
 module_init(mytraffic_init);
 module_exit(mytraffic_exit);
 
 MODULE_DESCRIPTION("Hardcoded GPIO Traffic Light + Lightbulb Check Extra-credit");
 MODULE_AUTHOR("Xingjian Jiang/Pree Simphliphan");
 MODULE_LICENSE("GPL");
 