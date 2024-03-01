#ifndef __KERNEL_FS_EXT2FS_H__
#define __KERNEL_FS_EXT2FS_H__


#include <sync/mutex.h>
#include <uniks/defs.h>
#include <uniks/param.h>


// EXT2FS basic info

#define EXT2_NAME_LEN	 (255)
#define EXT2_SB_BLKNO	 (0)
#define EXT2_SUPER_MAGIC (0xEF53)
#define EXT2_GRP_NUM	 1 + ((DISKSIZE * MiB - 1) / (BLKSIZE * 8 * BLKSIZE))
#define EXT2_GRPDESC_BLKNUM \
	(1 + (EXT2_GRP_NUM * sizeof(struct ext2_group_desc_t) - 1) / BLKSIZE)
#define EXT2_IOFFSET_OFGRP(i_no, sb) ((i_no - 1) % sb.s_inodes_per_group)
#define EXT2_IBLOCK_NO(i_no, sb) \
	(group_descs[((i_no - 1) / sb.s_inodes_per_group)].bg_inode_table + \
	 (EXT2_IOFFSET_OFGRP(i_no, sb) / IPB))

#define IPB (BLKSIZE / sizeof(struct ext2_inode_t))


// Special inode numbers
#define EXT2_BAD_INO	     1 /* Bad blocks inode */
#define EXT2_ROOT_INO	     2 /* Root inode */
#define EXT2_BOOT_LOADER_INO 5 /* Boot loader inode */
#define EXT2_UNDEL_DIR_INO   6 /* Undelete directory inode */

// File system states
#define EXT2_VALID_FS 0x0001 /* Unmounted cleanly */
#define EXT2_ERROR_FS 0x0002 /* Errors detected */

// Behaviour when detecting errors
#define EXT2_ERRORS_CONTINUE 1 /* Continue execution */
#define EXT2_ERRORS_RO	     2 /* Remount fs read-only */
#define EXT2_ERRORS_PANIC    3 /* Panic */

// Codes for operating systems
#define EXT2_OS_LINUX	0
#define EXT2_OS_HURD	1
#define EXT2_OS_MASIX	2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES	4

// Revision levels
#define EXT2_GOOD_OLD_REV 0 /* The good old (original) format */
#define EXT2_DYNAMIC_REV  1 /* V2 format dynamic inode sizes */

// Structure of the super block
struct ext2_super_block_t {
	uint32_t s_inodes_count;      /* Inodes count */
	uint32_t s_blocks_count;      /* Blocks count */
	uint32_t s_r_blocks_count;    /* Reserved blocks count */
	uint32_t s_free_blocks_count; /* Free blocks count */
	uint32_t s_free_inodes_count; /* Free inodes count */
	uint32_t s_first_data_block;  /* First Data Block */
	uint32_t s_log_block_size;    /* Block size */
	uint32_t s_log_frag_size;     /* Fragment size */
	uint32_t s_blocks_per_group;  /* # Blocks per group */
	uint32_t s_frags_per_group;   /* # Fragments per group */
	uint32_t s_inodes_per_group;  /* # Inodes per group */
	uint32_t s_mtime;	      /* Mount time */
	uint32_t s_wtime;	      /* Write time */
	uint16_t s_mnt_count;	      /* Mount count */
	uint16_t s_max_mnt_count;     /* Maximal mount count */
	uint16_t s_magic;	      /* Magic signature */
	uint16_t s_state;	      /* File system state */
	uint16_t s_errors;	      /* Behaviour when detecting errors */
	uint16_t s_minor_rev_level;   /* minor revision level */
	uint32_t s_lastcheck;	      /* time of last check */
	uint32_t s_checkinterval;     /* max. time between checks */
	uint32_t s_creator_os;	      /* OS */
	uint32_t s_rev_level;	      /* Revision level */
	uint16_t s_def_resuid;	      /* Default uid for reserved blocks */
	uint16_t s_def_resgid;	      /* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 *
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	uint32_t s_first_ino;	      /* First non-reserved inode */
	uint16_t s_inode_size;	      /* size of inode structure */
	uint16_t s_block_group_nr;    /* block group # of this superblock */
	uint32_t s_feature_compat;    /* compatible feature set */
	uint32_t s_feature_incompat;  /* incompatible feature set */
	uint32_t s_feature_ro_compat; /* readonly-compatible feature set */
	uint8_t s_uuid[16];	      /* 128-bit uuid for volume */
	char s_volume_name[16];	      /* volume name */
	char s_last_mounted[64];      /* directory where last mounted */
	uint32_t s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the EXT2_COMPAT_PREALLOC flag is on.
	 */
	uint8_t s_prealloc_blocks;     /* Nr of blocks to try to preallocate*/
	uint8_t s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
	uint16_t s_padding1;
	/*
	 * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	uint8_t s_journal_uuid[16]; /* uuid of journal superblock */
	uint32_t s_journal_inum;    /* inode number of journal file */
	uint32_t s_journal_dev;	    /* device number of journal file */
	uint32_t s_last_orphan;	    /* start of list of inodes to delete */
	/* -- Directory Indexing Support -- */
	uint32_t s_hash_seed[4];    /* HTREE hash seed */
	uint8_t s_def_hash_version; /* Default hash version to use */
	uint8_t s_reserved_pad[3];
	/* -- Other options -- */
	uint32_t s_default_mount_opts;
	uint32_t s_first_meta_bg; /* First metablock block group */
	uint8_t s_reserved[760];  /* Padding to the end of the block */
} __packed;

// Structure of a blocks group descriptor
struct ext2_group_desc_t {
	uint32_t bg_block_bitmap;      /* Blocks bitmap block */
	uint32_t bg_inode_bitmap;      /* Inodes bitmap block */
	uint32_t bg_inode_table;       /* Inodes table block */
	uint16_t bg_free_blocks_count; /* Free blocks count */
	uint16_t bg_free_inodes_count; /* Free inodes count */
	uint16_t bg_used_dirs_count;   /* Directories count */
	uint16_t bg_pad;
	uint8_t bg_reserved[12];
} __packed;

// I-mode Values

// -- file format --
#define EXT2_S_IFSOCK	       0xC000 /*socket*/
#define EXT2_S_IFLNK	       0xA000 /*symbolic link*/
#define EXT2_S_IFREG	       0x8000 /*regular file*/
#define EXT2_S_IFBLK	       0x6000 /*block device*/
#define EXT2_S_IFDIR	       0x4000 /*directory*/
#define EXT2_S_IFCHR	       0x2000 /*character device*/
#define EXT2_S_IFIFO	       0x1000 /*fifo*/
/* Test macros for file types.	*/
#define __S_ISTYPE(mode, mask) (((mode) & 0xF000) == (mask))
#define S_ISSOCK(mode)	       __S_ISTYPE((mode), EXT2_S_IFSOCK)
#define S_ISLNK(mode)	       __S_ISTYPE((mode), EXT2_S_IFLNK)
#define S_ISREG(mode)	       __S_ISTYPE((mode), EXT2_S_IFREG)
#define S_ISBLK(mode)	       __S_ISTYPE((mode), EXT2_S_IFBLK)
#define S_ISDIR(mode)	       __S_ISTYPE((mode), EXT2_S_IFDIR)
#define S_ISCHR(mode)	       __S_ISTYPE((mode), EXT2_S_IFCHR)
#define S_ISFIFO(mode)	       __S_ISTYPE((mode), EXT2_S_IFIFO)

// -- process execution user/group override --
#define EXT2_S_ISUID 0x0800 /*Set process User ID*/
#define EXT2_S_ISGID 0x0400 /*Set process Group ID*/
#define EXT2_S_ISVTX 0x0200 /*sticky bit*/

// -- access rights --
#define EXT2_S_IRUSR  0x0100 /*user read*/
#define EXT2_S_IWUSR  0x0080 /*user write*/
#define EXT2_S_IXUSR  0x0040 /*user execute*/
#define EXT2_S_IRGRP  0x0020 /*group read*/
#define EXT2_S_IWGRP  0x0010 /*group write*/
#define EXT2_S_IXGRP  0x0008 /*group execute*/
#define EXT2_S_IROTH  0x0004 /*others read*/
#define EXT2_S_IWOTH  0x0002 /*others write*/
#define EXT2_S_IXOTH  0x0001 /*others execute*/
/* Test macros for file permission.	*/
#define S_IRUSR(mode) get_var_bit(mode, EXT2_S_IRUSR) /* Read by owner. */
#define S_IWUSR(mode) get_var_bit(mode, EXT2_S_IWUSR) /* Write by owner. */
#define S_IXUSR(mode) get_var_bit(mode, EXT2_S_IXUSR) /* Execute by owner. */

// -- multi-level index meta data --
#define EXT2_NDIR_BLOCKS 12		       /* number of direct blocks */
#define EXT2_IND_BLOCK	 EXT2_NDIR_BLOCKS      /* single indirect block   */
#define EXT2_DIND_BLOCK	 (EXT2_IND_BLOCK + 1)  /* double indirect block   */
#define EXT2_TIND_BLOCK	 (EXT2_DIND_BLOCK + 1) /* trible indirect block   */
#define EXT2_N_BLOCKS	 (EXT2_TIND_BLOCK + 1) /* total number of blocks  */
#define EXT2_IND_PER_BLK (BLKSIZE / sizeof(uint32_t))
#define EXT2_IND_LIMIT	 (EXT2_NDIR_BLOCKS + EXT2_IND_PER_BLK)
#define EXT2_DIND_LIMIT	 (EXT2_IND_LIMIT + EXT2_IND_PER_BLK * EXT2_IND_PER_BLK)
#define EXT2_TIND_LIMIT \
	(EXT2_DIND_LIMIT + \
	 EXT2_IND_PER_BLK * EXT2_IND_PER_BLK * EXT2_IND_PER_BLK)

// Inode flags
#define EXT2_SECRM_FL	  0x00000001 /* Secure deletion */
#define EXT2_UNRM_FL	  0x00000002 /* Undelete */
#define EXT2_COMPR_FL	  0x00000004 /* Compress file */
#define EXT2_SYNC_FL	  0x00000008 /* Synchronous updates */
#define EXT2_IMMUTABLE_FL 0x00000010 /* Immutable file */
#define EXT2_APPEND_FL	  0x00000020 /* writes to file may only append */
#define EXT2_NODUMP_FL	  0x00000040 /* do not dump file */
#define EXT2_NOATIME_FL	  0x00000080 /* do not update atime */

// Structure of an inode on the disk
struct ext2_inode_t {
	uint16_t i_mode;		 /* File mode */
	uint16_t i_uid;			 /* Low 16 bits of Owner Uid */
	uint32_t i_size;		 /* Size in bytes */
	uint32_t i_atime;		 /* Access time */
	uint32_t i_ctime;		 /* Creation time */
	uint32_t i_mtime;		 /* Modification time */
	uint32_t i_dtime;		 /* Deletion Time */
	uint16_t i_gid;			 /* Low 16 bits of Group Id */
	uint16_t i_links_count;		 /* Links count */
	uint32_t i_blocks;		 /* Blocks count of 512-bytes blocks */
	uint32_t i_flags;		 /* File flags */
	uint32_t i_osd1;		 /* OS dependent 1 */
	uint32_t i_block[EXT2_N_BLOCKS]; /* Pointers to blocks */
	uint32_t i_generation;		 /* File version (for NFS) */
	uint32_t i_file_acl;		 /* File ACL */
	uint32_t i_dir_acl;		 /* Directory ACL */
	uint32_t i_faddr;		 /* Fragment address */
	uint32_t i_osd2[3];		 /* OS dependent 2 */
} __packed;

// File_type: this value must match the type defined in the related inode entry
#define EXT2_FT_UNKNOWN	 0 /*Unknown File Type*/
#define EXT2_FT_REG_FILE 1 /*Regular File*/
#define EXT2_FT_DIR	 2 /*Directory File*/
#define EXT2_FT_CHRDEV	 3 /*Character Device*/
#define EXT2_FT_BLKDEV	 4 /*Block Device*/
#define EXT2_FT_FIFO	 5 /* Buffer File*/
#define EXT2_FT_SOCK	 6 /*Socket File*/
#define EXT2_FT_SYMLINK	 7 /* Symbolic Lin*/

// Structure of a directory entry
struct ext2_dir_entry_t {
	uint32_t inode;	   /* Inode number */
	uint16_t rec_len;  /* Directory entry length */
	uint16_t name_len; /* Name length */

	// file's name doesn't contain '\0' terminate
	char name[]; /* File name, up to EXT2_NAME_LEN */
};

/*
 * The new version of the directory entry. Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct ext2_dir_entry_2_t {
	uint32_t inode;	  /* Inode number */
	uint16_t rec_len; /* Directory entry length */
	uint8_t name_len; /* Name length */
	uint8_t file_type;
	char name[]; /* File name, up to EXT2_NAME_LEN */
} __packed;
#define EXT2_DIRENTRY_MAXSIZE (sizeof(struct ext2_dir_entry_t) + EXT2_NAME_LEN)


// === in-memory data structure module ===

// in-memory copy of super_block
struct m_super_block_t {
	struct ext2_super_block_t d_sb_content;

	// Below are only in memory
	dev_t sb_dev;	// the device number that the sb lay
};

// in-memory copy of an inode
struct m_inode_t {
	struct ext2_inode_t d_inode_content;

	// protects everything above here
	struct mutex_t i_mtx;

	// Below are only in memory
	dev_t i_dev;
	uint32_t i_no;
	/*
	 * i_block_group is the number of the block group which contains
	 * this file's inode.  Constant across the lifetime of the inode,
	 * it is used for making block allocation decisions - we try to
	 * place a file's data blocks near its inode block, and new inodes
	 * near to their parent directory's inode.
	 */
	uint32_t i_block_group;
	int8_t i_dirty;
	int8_t i_valid;	  // inode has been read from disk?
	uint16_t i_count;
};

struct inode_table_t {
	struct spinlock_t lock;
	struct list_node_t wait_list;
	struct m_inode_t m_inodes[NINODE];
};

extern struct m_super_block_t m_sb;
extern struct inode_table_t inode_table;


void inode_table_init();
void ext2fs_init(dev_t dev);

// Inodes
struct m_inode_t *ialloc(dev_t dev);
struct m_inode_t *idup(struct m_inode_t *ip);
void ilock(struct m_inode_t *ip);
void iunlock(struct m_inode_t *ip);
void iput(struct m_inode_t *ip);
void iunlockput(struct m_inode_t *ip);
void iupdate(struct m_inode_t *ip);

// Inode content
void itruncate(struct m_inode_t *ip);
int64_t readi(struct m_inode_t *ip, int32_t user_dst, char *dst, uint64_t off,
	      uint64_t n);

// Paths
struct m_inode_t *namei(char *path);
struct m_inode_t *nameiparent(char *path, char *name);


uint32_t bmap(struct m_inode_t *ip, uint32_t blk_no);

#endif /* !__KERNEL_FS_EXT2FS_H__ */