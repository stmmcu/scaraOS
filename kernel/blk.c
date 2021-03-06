/*
 * Kernel block device management routines.
 *
 * TODO:
 *   o Write support
 *   o Asynchronous API
*/

#include <scaraOS/kernel.h>
#include <scaraOS/semaphore.h>
#include <scaraOS/mm.h>
#include <scaraOS/blk.h>

static objcache_t bh_cache;

__init void blk_init(void)
{
	bh_cache = objcache_init(NULL, "buffer", sizeof(struct buffer));
	BUG_ON(NULL == bh_cache);
}

/* Read a block in to a buffer */
struct buffer *blk_read(struct blkdev *dev, int logical)
{
	struct buffer *bh;

	bh = objcache_alloc(bh_cache);
	if ( NULL == bh )
		return NULL;

	bh->b_dev = dev;
	bh->b_block = logical;
	bh->b_buf = kmalloc(dev->count * dev->sectsize);
	bh->b_len = dev->count * dev->sectsize;
	if ( NULL == bh->b_buf ) {
		kfree(bh);
		return NULL;
	}

	/* Synchronously read from the device */
	sem_P(&dev->blksem);
	if ( dev->ll_rw_blk(dev, 0, logical * dev->count,
				bh->b_buf, dev->count) ) {
		sem_V(&dev->blksem);
		kfree(bh->b_buf);
		objcache_free2(bh_cache, bh);
		return NULL;
	}

	sem_V(&dev->blksem);
	return bh;
}

void blk_free(struct buffer *bh)
{
	kfree(bh->b_buf);
	objcache_free2(bh_cache, bh);
}

/* Set blocksize of device (NOTE: Blocksize is not the same as sector size) */
int blk_set_blocksize(struct blkdev *dev, unsigned int size)
{
	if ( dev->sectsize > size )
		return -1;

	if ( size > PAGE_SIZE || size < 512 || (size & (size-1)) )
		return -1;

	/* dont need to change it */
	if ( dev->sectsize * dev->count == size )
		return 0;

	dev->count = size / dev->sectsize;
	return 0;
}
