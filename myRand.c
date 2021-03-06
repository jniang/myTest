
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
static unsigned char state[256];
static int ri;
static int rj;
static spinlock_t my_lock =__SPIN_LOCK_UNLOCKED();//make an unlock spinlock 
/*

DEFINE ALL YOUR RC4 STUFF HERE

 */


/*
  called when opening a device.  We won't do anything
 */
//Add Two functions here: rc4Init and rc4Next
void rc4Init(unsigned char *key, int length){
	int i;
	for(i = 0; i < 256; i++){
		state[i] = i;
	}
	ri = rj = 0;

	for(ri = 0; ri < 256; ri++){
		rj = (rj + state[ri] + key[ri % length]) % 256;
		swap(state[ri], state[rj]);
	}
	ri = rj = 0;
}
unsigned char rc4Next(void){
	ri = (ri + 1) % 256;
	rj = (rj + state[ri]) % 256;
	swap(state[ri], state[rj]);
	return state[(state[ri] + state[rj]) % 256];
}



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
	unsigned char * keystream;
	int i; 
	// spinlock locking 
	spin_lock(&my_lock);
	//Keystream- allocating memory from the kernel space
	keystream = (unsigned char*) kmalloc(sizeof(char*) * count, GFP_KERNEL);

	//Keystream going to store the rc4 value
	for(i = 0; i < count; i++){
		keystream[i] = rc4Next();
	}
	//User Space Buffer: copying data from the kernel space
	//buf used to the read the file
	if(copy_to_user(buf, keystream, count)){
		kfree(keystream);
		return EFAULT;//bad bad bad address!!!
	}
	//buffer content needs to be printk
	printk("Buffer Data Read: ");
	for(i=0; i < count; i++){
		printk("%d", buf[i]);
	}
	printk("\n");
	spin_unlock(&my_lock);
  return 0;
}

ssize_t myRand_write(struct file*filp, const char __user *buf, size_t count, loff_t *fpos){
  /*
	USE THE USER's BUFFER TO RE-INITIALIZE YOUR RC4 GENERATOR
	BE SURE NOT TO DIRECTLY DEREFERENCE A USER POINTER!
   */

	//Kernel Memory Allocation
	//ENOMEM - means out of memory 
	unsigned char* key;
	key = (unsigned char*) kmalloc(count, GFP_KERNEL);
	spin_lock(&my_lock);
	//Null Pointer Check
	if(!key){
		return ENOMEM;
	}
	//copying chucnks of data to the kernel space from user space
	if(copy_from_user(key, buf, count)){
		kfree(key);
		return EFAULT;
	}
	//Taking the rc4 generator going to reinitialize from user input
	printk("Write RC4 generator: \n");
	rc4Init(key,count);
	//free memory
	kfree(key);
	//return # of bytes to the file
	spin_unlock(&my_lock);

  return count;
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
