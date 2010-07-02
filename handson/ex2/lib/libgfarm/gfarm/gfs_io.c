#include <assert.h>
#include <stdio.h>	/* config.h needs FILE */
#include <unistd.h>

#define GFARM_INTERNAL_USE /* GFARM_FILE_LOOKUP */
#include <gfarm/gfarm.h>

#include "gfutil.h"

#include "gfm_client.h"
#include "config.h"
#include "lookup.h"
#include "gfs_io.h"

static gfarm_error_t
gfm_open_flag_check(int flag)
{
	if (flag & ~GFARM_FILE_USER_MODE)
		return (GFARM_ERR_INVALID_ARGUMENT);
	if ((flag & GFARM_FILE_ACCMODE) == GFARM_FILE_LOOKUP)
		return (GFARM_ERR_INVALID_ARGUMENT);
	return (GFARM_ERR_NO_ERROR);
}

/*
 * gfm_create_fd()
 */

struct gfm_create_fd_closure {
	/* input */
	int flags;
	gfarm_mode_t mode_to_create;

	/* work */
	gfarm_mode_t mode_created;
	int fd;

	/* output */
	struct gfm_connection **gfm_serverp;
	int *fdp;
	int *typep;
};

static gfarm_error_t
gfm_create_fd_request(struct gfm_connection *gfm_server, void *closure,
	const char *base)
{
	struct gfm_create_fd_closure *c = closure;
	gfarm_error_t e;

	if ((e = gfm_client_create_request(gfm_server, base,
	    c->flags, c->mode_to_create)) != GFARM_ERR_NO_ERROR) {
		gflog_warning(GFARM_MSG_1000080, "create(%s) request: %s",
		    base, gfarm_error_string(e));
	} else if ((e = gfm_client_get_fd_request(gfm_server))
	    != GFARM_ERR_NO_ERROR) {
		gflog_warning(GFARM_MSG_1000081, "get_fd(%s) request: %s",
		    base, gfarm_error_string(e));
	}
	return (e);
}

static gfarm_error_t
gfm_create_fd_result(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_create_fd_closure *c = closure;
	gfarm_error_t e;
	gfarm_ino_t inum;
	gfarm_uint64_t gen;

	if ((e = gfm_client_create_result(gfm_server,
	    &inum, &gen, &c->mode_created)) != GFARM_ERR_NO_ERROR) {
#if 0 /* DEBUG */
		gflog_debug(GFARM_MSG_1000082,
		    "create() result: %s", gfarm_error_string(e));
#endif
	} else if ((e = gfm_client_get_fd_result(gfm_server, &c->fd))
	    != GFARM_ERR_NO_ERROR) {
		gflog_warning(GFARM_MSG_1000083,
		    "get_fd() result: %s", gfarm_error_string(e));
	}
	return (e);
}

static gfarm_error_t
gfm_create_fd_success(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_create_fd_closure *c = closure;

	*c->gfm_serverp = gfm_server;
	*c->fdp = c->fd;
	if (c->typep != NULL)
		*c->typep = gfs_mode_to_type(c->mode_created);;
	return (GFARM_ERR_NO_ERROR);
}

gfarm_error_t
gfm_create_fd(const char *path, int flags, gfarm_mode_t mode,
	struct gfm_connection **gfm_serverp, int *fdp, int *typep)
{
	gfarm_error_t e;
	struct gfm_create_fd_closure closure;

#if 0 /* not yet in gfarm v2 */
	/* GFARM_FILE_EXCLUSIVE is a NOP with gfm_open_fd(). */
	flags &= ~GFARM_FILE_EXCLUSIVE;
#endif /* not yet in gfarm v2 */

	if ((e = gfm_open_flag_check(flags)) != GFARM_ERR_NO_ERROR)
		return (e);

	closure.flags = flags;
	closure.mode_to_create = mode;
	closure.gfm_serverp = gfm_serverp;
	closure.fdp = fdp;
	closure.typep = typep;
	return (gfm_name_op(path, GFARM_ERR_IS_A_DIRECTORY,
	    gfm_create_fd_request,
	    gfm_create_fd_result,
	    gfm_create_fd_success,
	    &closure));
}

/*
 * gfm_open_fd()
 */

struct gfm_open_fd_closure {
	int fd;
	struct gfm_connection **gfm_serverp;
	int *fdp;
	int *typep;
};

static gfarm_error_t
gfm_open_fd_request(struct gfm_connection *gfm_server, void *closure)
{
	gfarm_error_t e = gfm_client_get_fd_request(gfm_server);

	if (e != GFARM_ERR_NO_ERROR)
		gflog_warning(GFARM_MSG_1000084,
		    "get_fd request; %s", gfarm_error_string(e));
	return (e);
}

static gfarm_error_t
gfm_open_fd_result(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_open_fd_closure *c = closure;
	gfarm_error_t e = gfm_client_get_fd_result(gfm_server, &c->fd);

#if 0 /* DEBUG */
	if (e != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1000085,
		    "get_fd result; %s", gfarm_error_string(e));
#endif
	return (e);
}

static gfarm_error_t
gfm_open_fd_success(struct gfm_connection *gfm_server, void *closure, int type)
{
	struct gfm_open_fd_closure *c = closure;

	*c->gfm_serverp = gfm_server;
	*c->fdp = c->fd;
	if (c->typep != NULL)
		*c->typep = type;
	return (GFARM_ERR_NO_ERROR);
}

gfarm_error_t
gfm_open_fd(const char *path, int flags,
	struct gfm_connection **gfm_serverp, int *fdp, int *typep)
{
	gfarm_error_t e;
	struct gfm_open_fd_closure closure;

#if 0 /* not yet in gfarm v2 */
	/* GFARM_FILE_EXCLUSIVE is a NOP with gfm_open_fd(). */
	flags &= ~GFARM_FILE_EXCLUSIVE;
#endif /* not yet in gfarm v2 */

	if ((e = gfm_open_flag_check(flags)) != GFARM_ERR_NO_ERROR)
		return (e);

	closure.gfm_serverp = gfm_serverp;
	closure.fdp = fdp;
	closure.typep = typep;
	return (gfm_inode_op(path, flags,
	    gfm_open_fd_request,
	    gfm_open_fd_result,
	    gfm_open_fd_success,
	    NULL,
	    &closure));
}

static gfarm_error_t
gfm_close_request(struct gfm_connection *gfm_server, void *closure)
{
	gfarm_error_t e = gfm_client_close_request(gfm_server);

	if (e != GFARM_ERR_NO_ERROR)
		gflog_warning(GFARM_MSG_1000086,
		    "close request: %s", gfarm_error_string(e));
	return (e);
}

static gfarm_error_t
gfm_close_result(struct gfm_connection *gfm_server, void *closure)
{
	gfarm_error_t e = gfm_client_close_result(gfm_server);

#if 1 /* DEBUG */
	if (e != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1000087,
		    "close result: %s", gfarm_error_string(e));
#endif
	return (e);
}

/*
 * gfm_close_fd()
 *
 * NOTE:
 * gfm_server is not freed by this function.
 * callers of this function should free it.
 */
gfarm_error_t
gfm_close_fd(struct gfm_connection *gfm_server, int fd)
{
	return (gfm_client_compound_fd_op(gfm_server, fd,
	    gfm_close_request, gfm_close_result, NULL, NULL));
}
