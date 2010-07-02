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

struct gfm_chown_closure {
	const char *username, *groupname;
};

static gfarm_error_t
gfm_chown_request(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_chown_closure *c = closure;
	gfarm_error_t e = gfm_client_fchown_request(gfm_server,
	    c->username, c->groupname);

	if (e != GFARM_ERR_NO_ERROR)
		gflog_warning(GFARM_MSG_1000116,
		    "fchown_fd request; %s", gfarm_error_string(e));
	return (e);
}

static gfarm_error_t
gfm_chown_result(struct gfm_connection *gfm_server, void *closure)
{
	gfarm_error_t e = gfm_client_fchown_result(gfm_server);

#if 0 /* DEBUG */
	if (e != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1000117,
		    "fchown result; %s", gfarm_error_string(e));
#endif
	return (e);
}

gfarm_error_t
gfs_chown(const char *path, const char *username, const char *groupname)
{
	struct gfm_chown_closure closure;

	closure.username = username;
	closure.groupname = groupname;
	return (gfm_inode_op(path, GFARM_FILE_LOOKUP,
	    gfm_chown_request,
	    gfm_chown_result,
	    gfm_inode_success_op_connection_free,
	    NULL,
	    &closure));
}
