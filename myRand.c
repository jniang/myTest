
//Should be all that's needed
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h> //device stuff
#include <linux/cdev.h> 
#include <linux/uaccess.h>  //for put_user, get_user, etc
#include <linux/slab.h>  //for kmalloc/kfree

MODULE_AUTHOR("Ben Jones + MSD");
MODULE_LICENSE("GPL");  //the kernel cares a lot whether modules are open source

//basically the file name that will be created in /dev
#define MY_DEVICE_NAME "myRand"

//Kernel stuff for keeping track of the device
static unsigned int myRand_major = 0;
static struct class *myRand_class = 0;
static struct cdev cdev; //the device
/*

DEFINE ALL YOUR RC4 STUFF HERE

 */


/*
  called when opening a device.  We won't do anything
 */
int myRand_open(struct inode *inode, struct file *filp){
  return 0; //nothing to do here
}

/* called when closing a device, we won't do anything */
int myRand_release(struct inode *inode, struct file *filp){
  return 0; //nothing to do here
}

ssize_t myRand_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){

  /* 
	 FILL THE USER'S BUFFER WITH BYTES FROM YOUR RC4 GENERATOR 
	 BE SURE NOT TO DIRECTLY DEREFERENCE A USER POINTER!

*/
  return 0;
}

ssize_t myRand_write(struct file*filp, const char __user *buf, size_t count, loff_t *fpos){
  /*
	USE THE USER's BUFFER TO RE-INITIALIZE YOUR RC4 GENERATOR
	BE SURE NOT TO DIRECTLY DEREFERENCE A USER POINTER!
   */
  return 0;
}

/* respond to seek() syscalls... by ignoring them */
loff_t myRand_llseek(struct file *rilp, loff_t off, int whence){
  return 0; //ignore seeks
}

/* register these functions with the kernel so it knows to call them in response to
   read, write, open, close, seek, etc */
struct file_operations myRand_fops = {
  .owner = THIS_MODULE,
  .read = myRand_read,
  .write = myRand_write,
  .open = myRand_open,
  .release = myRand_release,
  .llseek = myRand_llseek
};

/* this function makes it so that this device is readable/writable by normal users.
   Without this, only root can read/write this by default */
static int myRand_uevent(struct device* dev, struct kobj_uevent_env *env){
  add_uevent_var(env, "DEVMODE=%#o", 0666);
  return 0; 
}

/* Called when the module is loaded.  Do all our initialization stuff here */
static int __init
myRand_init_module(void){
  printk("Loading my random module");

  /*
	INITIALIZE YOUR RC4 GENERATOR WITH A SINGLE 0 BYTE
   */
  

  /*  This allocates necessary kernel data structures and plumbs everything together */
  dev_t dev = 0;
  int err = 0;
  err = alloc_chrdev_region(&dev, 0, 1, MY_DEVICE_NAME);
  if(err < 0){
    printk(KERN_WARNING "[target] alloc_chrdev_region() failed\n");
  }
  myRand_major = MAJOR(dev);
  myRand_class = class_create(THIS_MODULE, MY_DEVICE_NAME);
  if(IS_ERR(myRand_class)) {
    err = PTR_ERR(myRand_class);
    goto fail;
  }

  /* this code uses the uevent function above to make our device user readable */
  myRand_class->dev_uevent = myRand_uevent;
  int minor = 0;
  dev_t devno = MKDEV(myRand_major, minor);
  struct device *device = NULL;

  cdev_init(&cdev, &myRand_fops);
  cdev.owner = THIS_MODULE;

  err = cdev_add(&cdev, devno, 1);
  if(err){
    printk(KERN_WARNING "[target] Error trying to add device: %d", err);
    return err;
  }
  device = device_create(myRand_class, NULL, devno, NULL, MY_DEVICE_NAME);

  if(IS_ERR(device)) {
    err = PTR_ERR(device);
    printk(KERN_WARNING "[target error while creating device: %d", err);
    cdev_del(&cdev); //clean up dev
    return err;
  }
  printk("module loaded successfully\n");
  return 0;

 fail:
  printk("something bad happened!\n");
  return -1;
}

/* This is called when our module is unloaded */
static void __exit
myRand_exit_module(void){
  device_destroy(myRand_class, MKDEV(myRand_major, 0));
  cdev_del(&cdev);
  if(myRand_class){
    class_destroy(myRand_class);
  }
  unregister_chrdev_region(MKDEV(myRand_major, 0), 1);
  printk("Unloaded my random module");
}

module_init(myRand_init_module);
module_exit(myRand_exit_module);
