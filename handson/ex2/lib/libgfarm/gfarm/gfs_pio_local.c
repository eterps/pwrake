/*
 * pio operations for local section
 *
 * $Id: gfs_pio_local.c 3811 2007-09-06 09:18:41Z tatebe $
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h> /* gfs_client.h needs socklen_t */
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <openssl/evp.h>

#include <gfarm/gfarm.h>

#include "gfs_proto.h"	/* GFS_PROTO_FSYNC_* */
#include "gfs_client.h"
#include "gfs_io.h"
#include "gfs_pio.h"

#if 0 /* not yet in gfarm v2 */

int gfarm_node = -1;
int gfarm_nnode = -1;

gfarm_error_t
gfs_pio_set_local(int node, int nnode)
{
	if (node < 0 || node >= nnode || nnode < 0)
		return (GFARM_ERR_INVALID_ARGUMENT);

	gfarm_node = node;
	gfarm_nnode = nnode;
	return (GFARM_ERR_NO_ERROR);
}

gfarm_error_t
gfs_pio_set_local_check(void)
{
	if (gfarm_node < 0 || gfarm_nnode <= 0)
		return ("gfs_pio_set_local() is not correctly called");
	return (GFARM_ERR_NO_ERROR);
}

gfarm_error_t
gfs_pio_get_node_rank(int *node)
{
	gfarm_error_t e = gfs_pio_set_local_check();

	if (e != GFARM_ERR_NO_ERROR)
		return (e);
	*node = gfarm_node;
	return (GFARM_ERR_NO_ERROR);
}

gfarm_error_t
gfs_pio_get_node_size(int *nnode)
{
	gfarm_error_t e = gfs_pio_set_local_check();

	if (e != GFARM_ERR_NO_ERROR)
		return (e);
	*nnode = gfarm_nnode;
	return (GFARM_ERR_NO_ERROR);
}

gfarm_error_t
gfs_pio_set_view_local(GFS_File gf, int flags)
{
	gfarm_error_t e;
	char *arch;

	if (GFS_FILE_IS_PROGRAM(gf)) {
		e = gfarm_host_get_self_architecture(&arch);
		if (e != GFARM_ERR_NO_ERROR)
			return (gf->error = e);
		return (gfs_pio_set_view_section(gf, arch, NULL, flags));
	}
	e = gfs_pio_set_local_check();
	if (e != GFARM_ERR_NO_ERROR)
		return (gf->error = e);
	return (gfs_pio_set_view_index(gf, gfarm_nnode, gfarm_node,
				       NULL, flags));
}

#endif /* not yet in gfarm v2 */

/***********************************************************************/

static gfarm_error_t
gfs_pio_local_storage_close(GFS_File gf)
{
	gfarm_error_t e, e2;
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;

	if (close(vc->fd) == -1)
		e = gfarm_errno_to_error(errno);
	else
		e = GFARM_ERR_NO_ERROR;
	/*
	 * Do not close remote file from a child process because its
	 * open file count is not incremented.
	 * XXX - This behavior is not the same as expected, but better
	 * than closing the remote file.
	 */
	if (vc->pid != getpid())
		return (e);
	e2 = gfs_client_close(gfs_server, gf->fd);
	gfs_client_connection_free(gfs_server);
	return (e != GFARM_ERR_NO_ERROR ? e : e2);
}

static gfarm_error_t
gfs_pio_local_storage_pwrite(GFS_File gf,
	const char *buffer, size_t size, gfarm_off_t offset, size_t *lengthp)
{
	struct gfs_file_section_context *vc = gf->view_context;
#if 0 /* XXX FIXME: pwrite(2) on NetBSD-3.0_BETA is broken */
	int rv = pwrite(vc->fd, buffer, offset, size);
#else
	int rv;

	if (lseek(vc->fd, offset, SEEK_SET) == -1)
		return (gfarm_errno_to_error(errno));
	rv = write(vc->fd, buffer, size);
#endif

	if (rv == -1)
		return (gfarm_errno_to_error(errno));
	*lengthp = rv;
	return (GFARM_ERR_NO_ERROR);
}

static gfarm_error_t
gfs_pio_local_storage_pread(GFS_File gf,
	char *buffer, size_t size, gfarm_off_t offset, size_t *lengthp)
{
	struct gfs_file_section_context *vc = gf->view_context;
#if 0 /* XXX FIXME: pwrite(2) on NetBSD-3.0_BETA is broken */
	int rv = pread(vc->fd, buffer, offset, size);
#else
	int rv;

	if (lseek(vc->fd, offset, SEEK_SET) == -1)
		return (gfarm_errno_to_error(errno));
	rv = read(vc->fd, buffer, size);
#endif

	if (rv == -1)
		return (gfarm_errno_to_error(errno));
	*lengthp = rv;
	return (GFARM_ERR_NO_ERROR);
}

static gfarm_error_t
gfs_pio_local_storage_ftruncate(GFS_File gf, gfarm_off_t length)
{
	struct gfs_file_section_context *vc = gf->view_context;
	int rv;

	rv = ftruncate(vc->fd, length);
	if (rv == -1)
		return (gfarm_errno_to_error(errno));
	return (GFARM_ERR_NO_ERROR);
}

static gfarm_error_t
gfs_pio_local_storage_fsync(GFS_File gf, int operation)
{
	struct gfs_file_section_context *vc = gf->view_context;
	int rv;

	switch (operation) {
	case GFS_PROTO_FSYNC_WITHOUT_METADATA:
#ifdef HAVE_FDATASYNC
		rv = fdatasync(vc->fd);
		break;
#else
		/*FALLTHROUGH*/
#endif
	case GFS_PROTO_FSYNC_WITH_METADATA:
		rv = fsync(vc->fd);
		break;
	default:
		return (GFARM_ERR_INVALID_ARGUMENT);
	}

	if (rv == -1)
		return (gfarm_errno_to_error(errno));
	return (GFARM_ERR_NO_ERROR);
}

static gfarm_error_t
gfs_pio_local_storage_fstat(GFS_File gf, struct gfs_stat *st)
{
	struct gfs_file_section_context *vc = gf->view_context;
	struct stat sb;

	if (fstat(vc->fd, &sb) == 0) {
		st->st_size = sb.st_size;
		st->st_atimespec.tv_sec = sb.st_atime;
		st->st_atimespec.tv_nsec = 0; /* XXX */
		st->st_mtimespec.tv_sec = sb.st_mtime;
		st->st_mtimespec.tv_nsec = 0; /* XXX */
	}
	else
		return (gfarm_errno_to_error(errno));

	return (GFARM_ERR_NO_ERROR);
}

static int
gfs_pio_local_storage_fd(GFS_File gf)
{
	struct gfs_file_section_context *vc = gf->view_context;

	return (vc->fd);
}

struct gfs_storage_ops gfs_pio_local_storage_ops = {
	gfs_pio_local_storage_close,
	gfs_pio_local_storage_fd,
	gfs_pio_local_storage_pread,
	gfs_pio_local_storage_pwrite,
	gfs_pio_local_storage_ftruncate,
	gfs_pio_local_storage_fsync,
	gfs_pio_local_storage_fstat,
};

gfarm_error_t
gfs_pio_open_local_section(GFS_File gf, struct gfs_connection *gfs_server)
{
	struct gfs_file_section_context *vc = gf->view_context;
	gfarm_error_t e;

	e = gfs_client_open_local(gfs_server, gf->fd, &vc->fd);
	if (e != GFARM_ERR_NO_ERROR)
		return (e);

	vc->ops = &gfs_pio_local_storage_ops;
	vc->storage_context = gfs_server;
	vc->pid = getpid();
	return (GFARM_ERR_NO_ERROR);
}
