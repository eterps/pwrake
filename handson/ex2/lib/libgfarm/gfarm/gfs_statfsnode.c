/*
 * $Id$
 */

#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <gfarm/gfarm.h>

#include "config.h"
#include "gfm_client.h"
#include "gfs_client.h"

/*
 * XXX FIXME
 * a pathname argument is necessary to determine the gfm_server
 * (from multiple metadata servers).
 */

gfarm_error_t
gfs_statfsnode(char *host, int port,
	gfarm_int32_t *bsize, gfarm_off_t *blocks, gfarm_off_t *bfree,
	gfarm_off_t *bavail, gfarm_off_t *files, gfarm_off_t *ffree,
	gfarm_off_t *favail)
{
	gfarm_error_t e;
	struct gfm_connection *gfm_server;
	struct gfs_connection *gfs_server;
	int retry = 0;

	for (;;) {
		if ((e = gfm_client_connection_and_process_acquire(
		    gfarm_metadb_server_name, gfarm_metadb_server_port,
		    &gfm_server)) != GFARM_ERR_NO_ERROR)
			return (e);

		if ((e = gfs_client_connection_acquire_by_host(gfm_server,
		    host, port, &gfs_server, NULL))!= GFARM_ERR_NO_ERROR)
			goto free_gfm_connection;

		if (gfs_client_pid(gfs_server) == 0)
			e = gfarm_client_process_set(gfs_server, gfm_server);
		if (e == GFARM_ERR_NO_ERROR) {
			/* "/" is actually not used */
			e = gfs_client_statfs(gfs_server, "/", bsize, blocks,
				bfree, bavail, files, ffree, favail);
			if (gfs_client_is_connection_error(e) && ++retry<=1) {
				gfs_client_connection_free(gfs_server);
				continue;
			}
		}
		break;
	}
	gfs_client_connection_free(gfs_server);
 free_gfm_connection:
	gfm_client_connection_free(gfm_server);
	return (e);
}
