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
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/dcache.h>

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

static struct task_struct *my_thread;


static int check_ro(void *data) 
{

    struct path path;
    int error;
	allow_signal(SIGTERM);
	unsigned long s_flags=0;
	unsigned long s_blocksize=0;
	int 		  s_readonly_remount=0;
	
	printk(KERN_INFO "crc thread enter\n");
	
    while (!kthread_should_stop()) {
        // Do some work here
       
        // Sleep for some time
        set_current_state(TASK_INTERRUPTIBLE);


		error = kern_path("/", LOOKUP_FOLLOW, &path);
		if (error) 
		{
		    printk(KERN_ERR "Failed to get path for /\n");
		    return error;
		}

		if (path.dentry && path.dentry->d_sb) 
		{
		    struct super_block *sb = path.dentry->d_sb;
		
			if(s_blocksize != sb->s_blocksize ||
			   s_flags != sb->s_flags ||
			   s_readonly_remount != sb->s_readonly_remount)
			{
				if (sb->s_flags & 0x1) 
				{
				    printk(KERN_INFO "RO %X sz:%d bits:%d ror:%d\n", sb->s_flags, sb->s_blocksize, sb->s_readonly_remount);
				} 
				else 
				{
				    printk(KERN_INFO "RW %X sz:%d bits:%d ror:%d\n", sb->s_flags, sb->s_blocksize, sb->s_readonly_remount);
				}
				s_readonly_remount=sb->s_readonly_remount;
				s_flags=sb->s_flags;
				s_blocksize=sb->s_blocksize;
		    }
		}
		

		path_put(&path);

        schedule_timeout(HZ); // Sleep for 5 seconds
    }

	printk(KERN_INFO "crc thread exit\n");

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


#if 0
struct super_block {
	struct list_head	s_list;		/* Keep this first */
	dev_t			s_dev;		/* search index; _not_ kdev_t */
	unsigned char		s_blocksize_bits;
	unsigned long		s_blocksize;
	loff_t			s_maxbytes;	/* Max file size */
	struct file_system_type	*s_type;
	const struct super_operations	*s_op;
	const struct dquot_operations	*dq_op;
	const struct quotactl_ops	*s_qcop;
	const struct export_operations *s_export_op;
	unsigned long		s_flags;
	unsigned long		s_iflags;	/* internal SB_I_* flags */
	unsigned long		s_magic;
	struct dentry		*s_root;
	struct rw_semaphore	s_umount;
	int			s_count;
	atomic_t		s_active;
#ifdef CONFIG_SECURITY
	void                    *s_security;
#endif
	const struct xattr_handler **s_xattr;
#ifdef CONFIG_FS_ENCRYPTION
	const struct fscrypt_operations	*s_cop;
	struct key		*s_master_keys; /* master crypto keys in use */
#endif
#ifdef CONFIG_FS_VERITY
	const struct fsverity_operations *s_vop;
#endif
#ifdef CONFIG_UNICODE
	struct unicode_map *s_encoding;
	__u16 s_encoding_flags;
#endif
	struct hlist_bl_head	s_roots;	/* alternate root dentries for NFS */
	struct list_head	s_mounts;	/* list of mounts; _not_ for fs use */
	struct block_device	*s_bdev;
	struct backing_dev_info *s_bdi;
	struct mtd_info		*s_mtd;
	struct hlist_node	s_instances;
	unsigned int		s_quota_types;	/* Bitmask of supported quota types */
	struct quota_info	s_dquot;	/* Diskquota specific options */

	struct sb_writers	s_writers;

	/*
	 * Keep s_fs_info, s_time_gran, s_fsnotify_mask, and
	 * s_fsnotify_marks together for cache efficiency. They are frequently
	 * accessed and rarely modified.
	 */
	void			*s_fs_info;	/* Filesystem private info */

	/* Granularity of c/m/atime in ns (cannot be worse than a second) */
	u32			s_time_gran;
	/* Time limits for c/m/atime in seconds */
	time64_t		   s_time_min;
	time64_t		   s_time_max;
#ifdef CONFIG_FSNOTIFY
	__u32			s_fsnotify_mask;
	struct fsnotify_mark_connector __rcu	*s_fsnotify_marks;
#endif

	char			s_id[32];	/* Informational name */
	uuid_t			s_uuid;		/* UUID */

	unsigned int		s_max_links;
	fmode_t			s_mode;

	/*
	 * The next field is for VFS *only*. No filesystems have any business
	 * even looking at it. You had been warned.
	 */
	struct mutex s_vfs_rename_mutex;	/* Kludge */

	/*
	 * Filesystem subtype.  If non-empty the filesystem type field
	 * in /proc/mounts will be "type.subtype"
	 */
	const char *s_subtype;

	const struct dentry_operations *s_d_op; /* default d_op for dentries */

	/*
	 * Saved pool identifier for cleancache (-1 means none)
	 */
	int cleancache_poolid;

	struct shrinker s_shrink;	/* per-sb shrinker handle */

	/* Number of inodes with nlink == 0 but still referenced */
	atomic_long_t s_remove_count;

	/*
	 * Number of inode/mount/sb objects that are being watched, note that
	 * inodes objects are currently double-accounted.
	 */
	atomic_long_t s_fsnotify_connectors;

	/* Being remounted read-only */
	int s_readonly_remount;

	/* per-sb errseq_t for reporting writeback errors via syncfs */
	errseq_t s_wb_err;

	/* AIO completions deferred from interrupt context */
	struct workqueue_struct *s_dio_done_wq;
	struct hlist_head s_pins;

	/*
	 * Owning user namespace and default context in which to
	 * interpret filesystem uids, gids, quotas, device nodes,
	 * xattrs and security labels.
	 */
	struct user_namespace *s_user_ns;

	/*
	 * The list_lru structure is essentially just a pointer to a table
	 * of per-node lru lists, each of which has its own spinlock.
	 * There is no need to put them into separate cachelines.
	 */
	struct list_lru		s_dentry_lru;
	struct list_lru		s_inode_lru;
	struct rcu_head		rcu;
	struct work_struct	destroy_work;

	struct mutex		s_sync_lock;	/* sync serialisation lock */

	/*
	 * Indicates how deep in a filesystem stack this SB is
	 */
	int s_stack_depth;

	/* s_inode_list_lock protects s_inodes */
	spinlock_t		s_inode_list_lock ____cacheline_aligned_in_smp;
	struct list_head	s_inodes;	/* all inodes */

	spinlock_t		s_inode_wblist_lock;
	struct list_head	s_inodes_wb;	/* writeback inodes */
} __randomize_layout;

#endif


```
