```
#include <linux/module.h>     // Required for all kernel modules
#include <linux/kernel.h>     // KERN_INFO
#include <linux/init.h>       // __init, __exit macros
#include <linux/kernfs.h>     // For kernfs_node, kernfs_path_from_name, kernfs_for_each_entry, kernfs_put
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/fs.h>         // For S_ISDIR, S_ISREG (file type macros)
#include <linux/slab.h> // For kzalloc
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>

// Define a kernfs directory path you want to list.
// IMPORTANT: This path points to a kernfs filesystem.
// For example, /sys/kernel/debug requires 'debugfs' to be mounted,
// typically done via 'sudo mount -t debugfs none /sys/kernel/debug'.
#define TARGET_KERNFS_DIR_PATH "/sys/kernel/debug"
#define DIR_TYPE 4
#define FILE_TYPE 8

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name"); // Replace with your name
MODULE_DESCRIPTION("A kernel module to list entries in a kernfs directory.");
MODULE_VERSION("0.2"); // Updated version to reflect changes

static char __path[521] = {0};
static char __prev_path[521] = {0};
static unsigned long __tot_sz = 0;
static void itterate_rec(const char* dir);

static struct task_struct *my_thread;

static int my_filldir(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned int d_type)
{
    // Process the directory entry here
    // For example, print the name:
	char local[512];
	
    if(!strcmp(name,"dev")) return 0;
    if(!strcmp(name,"tmp")) return 0;
    if(!strcmp(name,"run")) return 0;
    if(!strcmp(name,"srv")) return 0;
    if(!strcmp(name,"sys")) return 0;
    if(!strcmp(name,"proc")) return 0;
    if(!strcmp(name,"kernel")) return 0;
    if(!strcmp(name,"mnt")) return 0;
    if(!strcmp(name,".")) return 0;
    if(!strcmp(name,"..")) return 0;
    if(strstr(name,"cache")) return 0;
    if(strstr(name,"arpd")) return 0;
    
    strcpy(local,__path);	
	strcat(__path,"/");
	strcat(__path,name);

	if(!strcmp(__path, "/data")) return 0;
	if(!strcmp(__path, "/var/lib/sudo")) return 0;
    if(strstr(__path, "/var/lib/sudo/misc")) return 0;
    if(!strcmp(__path, "/data/home")) return 0;
 
    if(d_type==DIR_TYPE) // file
    {
	    printk(KERN_INFO "D->: %s\n", __path);
    	itterate_rec(__path);
    }
    else if(d_type==FILE_TYPE && !S_ISLNK(d_type) ) 
    {
    	printk(KERN_INFO "	F: %s\n", __path);
    	
    	struct path p;
		struct kstat ks;

		kern_path(__path, 0, &p);
		vfs_getattr(&p, &ks,  STATX_SIZE, AT_STATX_SYNC_AS_STAT);
		__tot_sz += ks.size;
		
    }
    
    strcpy(__path,local);
    
    
    return 0; // Return 0 for success, non-zero to stop iteration
}


static void itterate_rec(const char* dir)
{
	struct file *dir_file;
    struct dir_context ctx = {
        .actor = my_filldir,
        .pos = 0, // Current position in the directory, typically starts at 0
    };
    
    



    // Open the root directory
    dir_file = filp_open(dir, O_RDONLY, 0);
    if (IS_ERR(dir_file)) {
        printk(KERN_ERR "Failed  D: %s ", dir);
        return ;
    }

    // Check if it's a directory
    if (!S_ISDIR(dir_file->f_inode->i_mode)) 
    {
        printk(KERN_ERR "/ is not a D!\n");
        filp_close(dir_file, NULL);
        return ;
    }

    // printk(KERN_INFO  ">>>>> going to path: %s\n",dir);
    iterate_dir(dir_file, &ctx);
   
    // Close the file
    filp_close(dir_file, NULL);
    
    return;
}


static int check_ro(void *data) 
{

    struct path path;
    int error;
	allow_signal(SIGTERM);

    while (!kthread_should_stop()) {
        // Do some work here
        printk(KERN_INFO "Kernel thread is running\n");

        // Sleep for some time
        set_current_state(TASK_INTERRUPTIBLE);


		error = kern_path("/", LOOKUP_FOLLOW, &path);
		if (error) {
		    printk(KERN_ERR "Failed to get path for /\n");
		    return error;
		}

		if (path.dentry && path.dentry->d_sb) {
		    struct super_block *sb = path.dentry->d_sb;
		    if (sb->s_flags & MS_RDONLY) {
		        printk(KERN_INFO "Rootfs is mounted read-only\n");
		    } else {
		        printk(KERN_INFO "Rootfs is not mounted read-only\n");
		    }
		}

		path_put(&path);

        schedule_timeout(HZ); // Sleep for 1 second
    }

    return 0;
}

/**
 * @brief The initialization function for the kernel module.
 * This function is called when the module is loaded into the kernel.
 * It uses kernfs-specific APIs to list entries in a specified kernfs directory.
 * @return 0 on success, or a negative error code on failure.
 */
static int __init list_files_init(void)
{
	//__tot_sz = 0;
	//__depth = 0;
	// itterate_rec("/");
	
	//printk(KERN_ERR "SUM=%ll\n", __tot_sz);
    
    my_thread = kthread_run(check_ro, NULL, "my_kernel_thread");
    if (IS_ERR(my_thread)) {
        printk(KERN_ERR "Failed to create kernel thread\n");
        return PTR_ERR(my_thread);
    }

    printk(KERN_INFO "Kernel thread started\n");
    
	return 0;
}

/**
 * @brief The cleanup function for the kernel module.
 * This function is called when the module is unloaded from the kernel.
 * It simply logs a message to the kernel buffer.
 */
static void __exit list_files_exit(void)
{
 	if (my_thread) {
        kthread_stop(my_thread);
    }

    printk(KERN_INFO "Module exiting\n");
}

// Register the module's initialization and cleanup functions with the kernel.
// module_init: Specifies the function to call when the module is loaded.
// module_exit: Specifies the function to call when the module is unloaded.
module_init(list_files_init);
module_exit(list_files_exit);

```
