/**
 * @file    lkm_xoro.c
 * @author  Dave McEwan
 * @date    2019-11-13
 * @version 0.1
 * @brief   Provide character device which returns bytes from a PRNG in the
 *          "xorshiro" family.
 *          Just an example LKM.
 */

// class_create, class_destroy, class_unregister, device_create, device_destroy
#include <linux/device.h>

// file_operations, register_chrdev, unregister_chrdev
#include <linux/fs.h>

// __init, __exit, module_init, module_exit
#include <linux/init.h>

// printk (<linux/printk.h>), uint64_t (<linux/types.h>)
#include <linux/kernel.h>

// MODULE_AUTHOR, MODULE_DESCRIPTION, MODULE_LICENSE, MODULE_VERSION,
// module_init, module_exit
#include <linux/module.h>

// DEFINE_MUTEX, mutex_init, mutex_destroy
#include <linux/mutex.h>

// copy_to_user
#include <linux/uaccess.h>

#define  DEVICE_NAME "xoroshiro128p"  // /dev/xoroshiro128p
#define  CLASS_NAME  "xoro"           // Character device class

MODULE_AUTHOR("Dave McEwan");
MODULE_DESCRIPTION("Xoroshiro128p PRNG");
MODULE_LICENSE("GPL"); // NOTE: Affects functionallity!
MODULE_VERSION("0.1");

// Symbols from xoroshiro128plus.o
extern void seed(uint64_t, uint64_t);
extern void jump(void);
extern uint64_t next(void);

static int major_number; // Character device major number.
static struct class* dev_class  = NULL;
static struct device* dev_device = NULL;

static int n_opens = 0; // Count the number of times device is opened.

// Mutex to allow only one userspace program to read at once.
static DEFINE_MUTEX(xoroshiro128p_mutex);

/**
 * Devices are represented as file structure in the kernel.
 * The file_operations structure from linux/fs.h lists the callback functions
 * with a C99 struct.
 * Function prototypes must come before struct.
 */
static int      dev_open    (struct inode*, struct file*);
static int      dev_release (struct inode*, struct file*);
static ssize_t  dev_read    (struct file*, char*, size_t, loff_t*);
static struct file_operations fops = {
  .open = dev_open,
  .read = dev_read,
  .release = dev_release,
};

/** @brief Initialize /dev/xoroshiro128p.
 *  @return Returns 0 if successful.
 */
static int __init
xoro_init(void)
{
  printk(KERN_INFO "XORO: Initializing...\n");

  major_number = register_chrdev(0,
                                 DEVICE_NAME,
                                 &fops);
  if (0 > major_number) {
    printk(KERN_ALERT "XORO: Failed to register major_number\n");
    return major_number;
  }
  printk(KERN_INFO "XORO:   major_number=%d\n", major_number);

  dev_class = class_create(THIS_MODULE,
                           CLASS_NAME);
  if (IS_ERR(dev_class)) {
     unregister_chrdev(major_number, DEVICE_NAME); // backout
     printk(KERN_ALERT "XORO: Failed to create dev_class\n");
     return PTR_ERR(dev_class);
  }
  printk(KERN_INFO "XORO:   dev_class[name]=%s\n", CLASS_NAME);

  // Register the device driver
  dev_device = device_create(dev_class,
                             NULL,
                             MKDEV(major_number, 0),
                             NULL,
                             DEVICE_NAME);
  if (IS_ERR(dev_device)) {
     class_destroy(dev_class); // backout
     unregister_chrdev(major_number, DEVICE_NAME); // backout
     printk(KERN_ALERT "XORO: Failed to create dev_device\n");
     return PTR_ERR(dev_device);
  }
  printk(KERN_INFO "XORO:   dev_device[name]=%s\n", DEVICE_NAME);

  mutex_init(&xoroshiro128p_mutex);

  seed(314159265, 1618033989); // Initialize PRNG with pi and phi.

  printk(KERN_INFO "XORO:   Initialized\n");
  return 0;
}

/** @brief Free all module resources.
 *         Not used if part of a built-in driver rather than a LKM.
 */
static void __exit
xoro_exit(void)
{
   mutex_destroy(&xoroshiro128p_mutex);

   device_destroy(dev_class, MKDEV(major_number, 0));

   class_unregister(dev_class);
   class_destroy(dev_class);

   unregister_chrdev(major_number, DEVICE_NAME);

   printk(KERN_INFO "XORO: Exit\n");
}

/** @brief open() syscall.
 *         Increment counter, perform another jump to effectively give each
 *         reader a separate PRNG.
 *  @param inodep Pointer to an inode object (defined in linux/fs.h)
 *  @param filep Pointer to a file object (defined in linux/fs.h)
 */
static int
dev_open(struct inode *inodep, struct file *filep)
{
  // Try to acquire the mutex (returns 0 on fail)
  if (!mutex_trylock(&xoroshiro128p_mutex)) {
    printk(KERN_INFO "XORO: %s busy\n", DEVICE_NAME);
    return -EBUSY;
  }

  jump(); // xoroshiro128plus.c

  printk(KERN_INFO "XORO: %s opened. n_opens=%d\n", DEVICE_NAME, n_opens++);

  return 0;
}

/** @brief Called whenever device is read from user space.
 *  @param filep Pointer to a file object (defined in linux/fs.h).
 *  @param buffer Pointer to the buffer to which this function may write data.
 *  @param len Number of bytes requested.
 *  @param offset Unused.
 *  @return Returns number of bytes successfully read. Negative on error.
 */
static ssize_t
dev_read(struct file* filep, char* buffer, size_t len, loff_t* offset)
{

  // Give at most 8 bytes per read.
  size_t len_ = (len > 8) ? 8 : len;

  uint64_t value = next(); // xoroshiro128plus.c

  // copy_to_user has the format ( * to, *from, size) and returns 0 on success
  int n_notcopied = copy_to_user(buffer, (char*)(&value), len_);

  if (0 != n_notcopied) {
     printk(KERN_ALERT "XORO: Failed to read %d/%ld bytes\n", n_notcopied, len_);
     return -EFAULT;
  } else {
     printk(KERN_INFO "XORO: read %ld bytes\n", len_);
     return len_;
  }
}

/** @brief Called when the userspace program calls close().
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int
dev_release(struct inode *inodep, struct file *filep)
{
   mutex_unlock(&xoroshiro128p_mutex);
   printk(KERN_INFO "XORO: %s closed\n", DEVICE_NAME);
   return 0;
}

/** @brief Identify the initialization function at insertion time and the
 *  cleanup function defined in <linux/init.h>
 */
module_init(xoro_init);
module_exit(xoro_exit);
