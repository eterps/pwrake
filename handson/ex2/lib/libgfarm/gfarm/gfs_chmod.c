#include <stdio.h>	/* config.h needs FILE */
#include <stdlib.h>

#define GFARM_INTERNAL_USE
#include <gfarm/gfarm.h>

#include "gfutil.h"
#include "timer.h"

#include "gfs_profile.h"
#include "gfm_client.h"
#include "config.h"
#include "lookup.h"

struct gfm_chmod_closure {
	gfarm_mode_t mode;
};

static gfarm_error_t
gfm_chmod_request(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_chmod_closure *c = closure;
	gfarm_error_t e = gfm_client_fchmod_request(gfm_server, c->mode);

	if (e != GFARM_ERR_NO_ERROR)
		gflog_warning(GFARM_MSG_1000114,
		    "fchmod request; %s", gfarm_error_string(e));
	return (e);
}

static gfarm_error_t
gfm_chmod_result(struct gfm_connection *gfm_server, void *closure)
{
	gfarm_error_t e = gfm_client_fchmod_result(gfm_server);

#if 0 /* DEBUG */
	if (e != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1000115,
		    "fchmod result; %s", gfarm_error_string(e));
#endif
	return (e);
}

gfarm_error_t
gfs_chmod(const char *path, gfarm_mode_t mode)
{
	struct gfm_chmod_closure closure;

	closure.mode = mode;
	return (gfm_inode_op(path, GFARM_FILE_LOOKUP,
	    gfm_chmod_request,
	    gfm_chmod_result,
	    gfm_inode_success_op_connection_free,
	    NULL,
	    &closure));
}
