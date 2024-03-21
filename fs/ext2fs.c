#include "ext2fs.h"
#include <device/blkbuf.h>
#include <device/virtio_disk.h>
#include <mm/vm.h>
#include <process/proc.h>
#include <uniks/kassert.h>
#include <uniks/kstdlib.h>
#include <uniks/kstring.h>


// === in-memory inode table module ===
struct m_super_block_t m_sb;
struct inode_table_t inode_table;
char group_descs_table[EXT2_GRPDESC_BLKNUM * BLKSIZE];
struct ext2_group_desc_t *group_descs =
	(struct ext2_group_desc_t *)group_descs_table;

// Read the SUPER BLOCK.
static void readsb(dev_t dev, struct ext2_super_block_t *sb)
{
	struct blkbuf_t *bb = blk_read(dev, EXT2_SB_BLKNO);
	// the SUPER BLOCK start at offset 1024 of disk
	memcpy(sb, bb->b_data + 1024, sizeof(*sb));
	blk_release(bb);
}

// Read the GDT.
static void readgdt(dev_t dev, char *gd)
{
	struct blkbuf_t *bb;
	for (int32_t i = 0; i < EXT2_GRPDESC_BLKNUM; i++) {
		bb = blk_read(dev, EXT2_SB_BLKNO + 1 + i);
		memcpy(gd + i * BLKSIZE, bb->b_data, BLKSIZE);
		blk_release(bb);
	}
}

void ext2fs_init(dev_t dev)
{
	readsb(dev, &m_sb.d_sb_ctnt);
	assert(m_sb.d_sb_ctnt.s_magic == EXT2_SUPER_MAGIC);
	assert(BLKSIZE == (1024 << m_sb.d_sb_ctnt.s_log_block_size));
	assert(sizeof(struct ext2_inode_t) == m_sb.d_sb_ctnt.s_inode_size);
	assert(EXT2_GRP_NUM == (1 + (m_sb.d_sb_ctnt.s_blocks_count - 1) /
					    m_sb.d_sb_ctnt.s_blocks_per_group));
	m_sb.sb_dev = dev;

	readgdt(dev, group_descs_table);
}

// this function's name is learned from Linux-0.11:fs/super.c
void inode_table_init()
{
	initlock(&inode_table.lock, "inode_table");
	for (int32_t i = 0; i < NINODE; i++) {
		mutex_init(&inode_table.m_inodes[i].i_mtx, "inode");
		inode_table.m_inodes[i].i_count =
			inode_table.m_inodes[i].i_dirty = 0;
	}
}

int64_t search_free_bit(char *start, int64_t size)
{
	static char mask[] = {
		0b00000001, 0b00000010, 0b00000100, 0b00001000,
		0b00010000, 0b00100000, 0b01000000, 0b10000000,
	};
	int64_t idx = 0;
	for (int64_t i = 0; i < size; i++, idx += 8) {
		for (int j = 0; j < sizeof(mask); j++) {
			if (get_var_bit(~start[i], mask[j]))
				return idx + j;
		}
	}
	return -1;
}

void set_allocated_bit(char *start, int64_t location)
{
	set_var_bit(start[location / (sizeof(char) * 8)],
		    1 << (location % (sizeof(char) * 8)));
}


// Blocks

// allocate in block bitmap in disk but don't read it to in-memory
static int64_t balloc(dev_t dev, uint32_t grp_no)
{
	int64_t bnum = 0, freeno;
	struct blkbuf_t *bb;

	// Allocate free-blocks in grp_no's group preferentially
	{
		bb = blk_read(dev, group_descs[grp_no].bg_block_bitmap);
		freeno = search_free_bit(bb->b_data, BLKSIZE);
		if (freeno != -1) {
			set_allocated_bit(bb->b_data, freeno);
			blk_write_over(bb);
			return freeno +
			       m_sb.d_sb_ctnt.s_blocks_per_group * grp_no;
		}
		blk_release(bb);
	}

	for (uint32_t g_idx = 0; g_idx < EXT2_GRP_NUM; g_idx++) {
		if (g_idx == grp_no)
			continue;
		bb = blk_read(dev, group_descs[g_idx].bg_block_bitmap);
		freeno = search_free_bit(bb->b_data, BLKSIZE);
		if (freeno != -1) {
			set_allocated_bit(bb->b_data, freeno);
			blk_write_over(bb);
			return freeno + bnum;
		}
		blk_release(bb);

		bnum += m_sb.d_sb_ctnt.s_blocks_per_group;
	}
	return -1;
}

// Free a disk block.
static void bfree(dev_t dev, uint32_t blk_no)
{
	uint32_t g_idx = blk_no / m_sb.d_sb_ctnt.s_blocks_per_group;
	uint32_t blk_offset = blk_no % m_sb.d_sb_ctnt.s_blocks_per_group;
	struct blkbuf_t *bb = blk_read(dev, group_descs[g_idx].bg_block_bitmap);
	clear_var_bit(bb->b_data[blk_offset / (sizeof(char) * 8)],
		      1 << (blk_offset % (sizeof(char) * 8)));
	blk_write_over(bb);
}


// Inodes

/**
 * @brief Find the inode with number inum on device dev and return the in-memory
 * copy. Does not lock the inode and does not read it from disk.
 * @param dev
 * @param i_no start from 1 but not 0 (according to ext2fs manual)
 * @param clean
 * @return struct m_inode_t*
 */
static struct m_inode_t *iget(uint32_t dev, uint32_t i_no, int32_t clean)
{
	struct m_inode_t *ip, *empty = NULL;

	acquire(&inode_table.lock);

	// wait for another process iput() one.
	while (empty == NULL) {
		// Is the inode already in the table?
		for (ip = inode_table.m_inodes;
		     ip < &inode_table.m_inodes[NINODE]; ip++) {
			if (ip->i_count > 0 and ip->i_dev == dev and
			    ip->i_no == i_no) {
				ip->i_count++;
				release(&inode_table.lock);
				return ip;
			}
			if (empty == NULL and ip->i_count == 0)
				// Remember the 1st empty slot.
				empty = ip;
		}
		if (empty == NULL)
			proc_block(&inode_table.wait_list, &inode_table.lock);
	}
	assert(empty != NULL);
	assert(empty->i_count == 0);

	empty->i_dev = dev, empty->i_no = i_no;
	empty->i_count++;
	empty->i_valid = clean;
	release(&inode_table.lock);

	return empty;
}

/**
 * @brief Allocate an inode on device dev (marked in inode bitmap). Mark it as
 * allocated by modifying the inode bitmap. Returns an unlocked but allocated
 * and referenced inode, or NULL if there is no free inode.
 *
 * @param dev
 * @return struct m_inode_t*
 */
struct m_inode_t *ialloc(dev_t dev)
{
	int64_t inum = 0;
	struct blkbuf_t *bb;

	for (uint32_t g_idx = 0; g_idx < EXT2_GRP_NUM; g_idx++) {
		bb = blk_read(dev, group_descs[g_idx].bg_inode_bitmap);
		int64_t freeno = search_free_bit(bb->b_data, BLKSIZE);
		if (freeno != -1) {
			set_allocated_bit(bb->b_data, freeno);
			blk_write_over(bb);
			return iget(dev, inum + freeno + 1, 1);
		}
		blk_release(bb);

		inum += m_sb.d_sb_ctnt.s_inodes_per_group;
	}
	return NULL;
}

// Free a inode in inode table
static void ifree(dev_t dev, uint32_t i_no)
{
	uint32_t g_idx = i_no / m_sb.d_sb_ctnt.s_inodes_per_group;
	uint32_t i_offset = EXT2_IOFFSET_OFGRP(i_no, m_sb.d_sb_ctnt);
	struct blkbuf_t *bb = blk_read(dev, group_descs[g_idx].bg_inode_bitmap);
	clear_var_bit(bb->b_data[i_offset / (sizeof(char) * 8)],
		      1 << (i_offset % (sizeof(char) * 8)));
	blk_write_over(bb);
}

/**
 * @brief Increment reference count for ip. Returns ip to enable ip = idup(ip1)
 * idiom.
 * @param ip
 * @return struct m_inode_t*
 */
struct m_inode_t *idup(struct m_inode_t *ip)
{
	if (ip == NULL)
		goto ret;
	acquire(&inode_table.lock);
	assert(ip->i_count >= 1);
	ip->i_count++;
	release(&inode_table.lock);

ret:
	return ip;
}

// Lock the given inode. Reads the inode from disk if necessary.
void ilock(struct m_inode_t *ip)
{
	assert(ip != NULL);
	assert(ip->i_count >= 1);

	mutex_acquire(&ip->i_mtx);
	if (ip->i_valid == 0) {
		struct blkbuf_t *bb = blk_read(
			ip->i_dev, EXT2_IBLOCK_NO(ip->i_no, m_sb.d_sb_ctnt));
		void *tar_ip =
			((struct ext2_inode_t *)bb->b_data +
			 EXT2_IOFFSET_OFGRP(ip->i_no, m_sb.d_sb_ctnt) % IPB);
		memcpy(ip, tar_ip, sizeof(ip->d_inode_ctnt));
		blk_release(bb);
		ip->i_valid = 1;
	}
}

// Unlock the given inode.
void iunlock(struct m_inode_t *ip)
{
	assert(ip != NULL);
	assert(ip->i_count >= 1);
	assert(mutex_holding(&ip->i_mtx));

	mutex_release(&ip->i_mtx);
}

/**
 * @brief Drop a reference to an in-memory inode. If that was the last
 * reference, the inode table entry can be recycled. If that was the last
 * reference and the inode has no links to it, free the inode (and its content)
 * on disk.
 * @param ip
 */
void iput(struct m_inode_t *ip)
{
	if (ip == NULL)
		return;
	acquire(&inode_table.lock);

	if (ip->i_count == 1 and ip->i_valid and
	    ip->d_inode_ctnt.i_links_count == 0) {
		/**
		 * @brief ip->i_count == 1 means no other process can have ip
		 * locked, so this mutex_acquire() won't block (or deadlock).
		 */
		mutex_acquire(&ip->i_mtx);

		release(&inode_table.lock);

		/**
		 * @brief inode has no links and no other references: truncate
		 * and mark it as free in inode bitmap.
		 */
		ifree(ip->i_dev, ip->i_no);
		itruncate(ip);
		ip->i_valid = 0;

		mutex_release(&ip->i_mtx);

		acquire(&inode_table.lock);
	}

	ip->i_count--;
	release(&inode_table.lock);
}

// Common idiom: unlock, then put.
void iunlockput(struct m_inode_t *ip)
{
	iunlock(ip);
	iput(ip);
}

/**
 * @brief Copy a modified in-memory inode to disk block. Must be called after
 * every change to an ip->xxx field that lives on corresponding disk block.
 * Caller must hold ip->lock.
 * @param ip
 */
void iupdate(struct m_inode_t *ip)
{
	assert(ip->i_valid == 1);

	struct blkbuf_t *bb =
		blk_read(ip->i_dev, EXT2_IBLOCK_NO(ip->i_no, m_sb.d_sb_ctnt));
	void *tar_ip = ((struct ext2_inode_t *)bb->b_data +
			EXT2_IOFFSET_OFGRP(ip->i_no, m_sb.d_sb_ctnt) % IPB);
	memcpy(tar_ip, ip, sizeof(ip->d_inode_ctnt));
	blk_release(bb);
	ip->i_valid = 0;
}


// Inode content

/**
 * @brief Return the disk block address of the nth block in inode ip. If there
 * is no such block, bmap allocates one. returns 0 if out of disk space.
 * @param ip
 * @param blk_no
 * @return uint32_t
 */
uint32_t bmap(struct m_inode_t *ip, uint32_t blk_no)
{
	uint32_t baddr;
	struct blkbuf_t *bb;

	assert(blk_no < EXT2_TIND_LIMIT);

	if (blk_no < EXT2_NDIR_BLOCKS) {
		if ((baddr = ip->d_inode_ctnt.i_block[blk_no]) == 0) {
			baddr = balloc(ip->i_dev, ip->i_block_group);
			if (baddr == 0)
				return 0;
			ip->d_inode_ctnt.i_block[blk_no] = baddr;
		}
		return baddr;
	}

	// Multi-level indirect index, allocating if necessary.
	uint32_t divisor, primary_layer;
	if (blk_no < EXT2_IND_LIMIT) {
		blk_no -= EXT2_NDIR_BLOCKS;
		divisor = 1;
		primary_layer = EXT2_IND_BLOCK;
	} else if (blk_no < EXT2_DIND_LIMIT) {
		blk_no -= EXT2_IND_LIMIT;
		divisor = EXT2_IND_PER_BLK;
		primary_layer = EXT2_DIND_BLOCK;
	} else {
		blk_no -= EXT2_DIND_LIMIT;
		divisor = EXT2_IND_PER_BLK * EXT2_IND_PER_BLK;
		primary_layer = EXT2_TIND_BLOCK;
	}
	if ((baddr = ip->d_inode_ctnt.i_block[primary_layer]) == 0) {
		baddr = balloc(ip->i_dev, ip->i_block_group);
		if (baddr == 0)
			return 0;
		ip->d_inode_ctnt.i_block[primary_layer] = baddr;
	}
	while (divisor != 0) {
		bb = blk_read(ip->i_dev, baddr);
		uint32_t *index = (uint32_t *)bb->b_data;
		if ((baddr = index[blk_no / divisor]) == 0) {
			baddr = balloc(ip->i_dev, ip->i_block_group);
			if (baddr == 0) {
				blk_release(bb);
				return 0;
			}
			index[blk_no / divisor] = baddr;
			blk_write_over(bb);
		} else
			blk_release(bb);
		blk_no %= divisor;
		divisor /= 1024;
	}

	return baddr;
}

static void recursively_release(int32_t layer, struct m_inode_t *ip,
				uint32_t blk_no)
{
	struct blkbuf_t *bb = blk_read(ip->i_dev, blk_no);
	uint32_t *index = (uint32_t *)bb->b_data;
	for (int32_t i = 0; i < EXT2_IND_PER_BLK; i++) {
		// hint: don't support file hole now
		if (index[i] == 0)   // DFS pruning
			break;
		if (layer != 0)
			recursively_release(layer - 1, ip, index[i]);
		else {
			bfree(ip->i_dev, index[i]);
			ip->d_inode_ctnt.i_blocks -= BLKSIZE / SECTORSIZE;
		}
	}
	bfree(ip->i_dev, blk_no);
	ip->d_inode_ctnt.i_blocks -= BLKSIZE / SECTORSIZE;
	blk_release(bb);
}

// Truncate inode (discard contents). Caller must hold ip->lock.
void itruncate(struct m_inode_t *ip)
{
	for (int32_t i = 0; i < EXT2_NDIR_BLOCKS; i++) {
		if (ip->d_inode_ctnt.i_block[i]) {
			bfree(ip->i_dev, ip->d_inode_ctnt.i_block[i]);
			ip->d_inode_ctnt.i_block[i] = 0;
		}
	}

	// multi-level indirect index
	for (int32_t i = 0; i < 3; i++) {
		uint32_t blk_no =
			ip->d_inode_ctnt.i_block[EXT2_NDIR_BLOCKS + i];
		if (blk_no != 0)
			recursively_release(i, ip, blk_no);
	}

	assert(ip->d_inode_ctnt.i_blocks == 0);
	ip->d_inode_ctnt.i_size = 0;
	iupdate(ip);
}

/**
 * @brief Read data from inode. Caller must hold ip->lock. If user_dst==1, then
 * dst is a user virtual address; otherwise, dst is a kernel address.
 * @param ip
 * @param user_dst
 * @param dst
 * @param off
 * @param n
 * @return int64_t
 */
int64_t readi(struct m_inode_t *ip, int32_t user_dst, char *dst, uint64_t off,
	      size_t n)
{
	size_t tot, m;
	struct blkbuf_t *bb;

	if (off > ip->d_inode_ctnt.i_size or off + n < off)
		return 0;
	if (off + n > ip->d_inode_ctnt.i_size)
		n = ip->d_inode_ctnt.i_size - off;

	for (tot = 0; tot < n; tot += m, off += m, dst += m) {
		uint64_t addr = bmap(ip, off / BLKSIZE);
		if (addr == 0)
			break;
		bb = blk_read(ip->i_dev, addr);
		m = MIN(n - tot, BLKSIZE - off % BLKSIZE);
		assert(either_copyout(user_dst, dst,
				      bb->b_data + (off % BLKSIZE), m) != -1);
		blk_release(bb);
	}

	return tot;
}


// Directories

/**
 * @brief Look for a directory entry in a directory. If found, set *poff to byte
 * offset of entry.
 * @param ip
 * @param name
 * @param poff
 * @return struct m_inode_t*
 */
struct m_inode_t *dirlookup(struct m_inode_t *ip, char *name, uint64_t *poff)
{
	uint64_t off, namelen = strlen(name);
	assert(namelen <= EXT2_NAME_LEN);
	char tmp_de[EXT2_DIRENTRY_MAXSIZE];
	struct ext2_dir_entry_t *de = (struct ext2_dir_entry_t *)tmp_de;

	assert(S_ISDIR(ip->d_inode_ctnt.i_mode));

	for (off = 0; off < ip->d_inode_ctnt.i_size; off += de->rec_len) {
		readi(ip, 0, tmp_de, off, EXT2_DIRENTRY_MAXSIZE);
		if (de->inode == 0)
			continue;
		if (strncmp(name, de->name, MAX(namelen, de->name_len)) == 0) {
			// entry matches path element
			if (poff)
				*poff = off;
			return iget(ip->i_dev, de->inode, 0);
		}
	}

	return NULL;
}


// Paths

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = NULL
//
static char *skipelem(char *path, char *name)
{
	char *s;
	int32_t len;

	while (*path == '/')
		path++;
	if (*path == 0)
		return NULL;
	s = path;
	while (*path != '/' and *path != 0)
		path++;
	len = path - s;
	if (len >= EXT2_NAME_LEN) {
		memcpy(name, s, EXT2_NAME_LEN);
		name[EXT2_NAME_LEN] = 0;
	} else {
		memcpy(name, s, len);
		name[len] = 0;
	}
	while (*path == '/')
		path++;
	return path;
}

/**
 * @brief Look up and return the inode for a path name. If parent != 0, return
 * the inode for the parent and copy the final path element into name, which
 * must have room for EXT2_NAME_LEN bytes.
 * @param path
 * @param nameiparent
 * @param name
 * @return struct m_inode_t*
 */
static struct m_inode_t *namex(char *path, int nameiparent, char *name)
{
	struct m_inode_t *ip, *next;

	if (*path == '/')
		ip = iget(m_sb.sb_dev, EXT2_ROOT_INO, 0);
	else
		ip = idup(myproc()->cwd);

	while ((path = skipelem(path, name)) != NULL) {
		if (strncmp(name, ".", 1) == 0)
			continue;
		ilock(ip);
		assert(S_ISDIR(ip->d_inode_ctnt.i_mode));
		if (nameiparent and *path == '\0') {
			// Stop one level early.
			iunlock(ip);
			return ip;
		}
		if ((next = dirlookup(ip, name, 0)) == NULL) {
			iunlockput(ip);
			return NULL;
		}
		iunlockput(ip);
		ip = next;
	}
	if (nameiparent) {
		iput(ip);
		return NULL;
	}
	return ip;
}

struct m_inode_t *namei(char *path)
{
	char name[EXT2_NAME_LEN + 1];
	return namex(path, 0, name);
}

struct m_inode_t *nameiparent(char *path, char *name)
{
	return namex(path, 1, name);
}
