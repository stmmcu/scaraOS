/*
 * The scaraOS dentry cache.
 *
 * The purpose of this code is to perform VFS name lookups ie: to
 * convert from file-paths/file-names to actual inode objects. The
 * dcache exists to speedup this lookup.
 *
 * TODO
 *  o Actually cache items
 *  o Implement cache shrinkage routines
*/
#include <kernel.h>
#include <blk.h>
#include <vfs.h>
#include <mm.h>
#include <task.h>

void _dentry_cache_init(void)
{
}

/* Resolve a name to an inode */
struct inode *namei(const char *name)
{
	struct inode *i;
	const char *n;
	int state, end;

	if ( NULL == name )
		return NULL;

	if ( *name == '/' ) {
		name++;
		i = iref(__this_task->root);
	}else{
		i = iref(__this_task->cwd);
	}

	/* Check for some bogus bugs */
	BUG_ON(NULL == i);
	BUG_ON(NULL == i->i_iop);
	BUG_ON(NULL == i->i_iop->lookup);

	/* Iterate the components of the filename
	 * looking up each one as we go */
	for(n = name, state = 1, end = 0; ;n++) {
		if ( state == 0 ) {
			struct inode *itmp;
			switch( *n ) {
			case '\0':
				end = 1;
				/* fall through */
			case '/':
				state = 1;
				itmp = i;
				dprintk("Parsing name: %.*s\n", n - name, name);
				if ( NULL == i->i_iop->lookup )
					return NULL; /* ENOTDIR */
				i = i->i_iop->lookup(itmp, name, n - name);
				if ( NULL == i )
					return NULL; /* ENOENT */
				if ( end )
					break;
				iput(itmp);
				continue;
			default:
				continue;
			}
			break;
		}else if ( state == 1 ) {
			if ( *n == '\0' )
				break;
			if ( *n != '/' ) {
				state = 0;
				name = n;
			}
		}
	}

	return i;
}
