/*
 * $Id$
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <gfarm/gfarm.h>

#include "config.h"
#include "host.h"
#include "gfm_client.h"
#include "gfs_client.h"
#include "schedule.h"
#include "gfs_misc.h"

static gfarm_error_t
gfs_replicate_from_to_internal(GFS_File gf, char *srchost, int srcport,
	char *dsthost, int dstport)
{
	gfarm_error_t e;
	struct gfm_connection *gfm_server = gfs_pio_metadb(gf);
	struct gfs_connection *gfs_server;
	int retry = 0;

	for (;;) {
		if ((e = gfs_client_connection_acquire_by_host(gfm_server,
		    dsthost, dstport, &gfs_server, NULL))!= GFARM_ERR_NO_ERROR)
			return (e);

		if (gfs_client_pid(gfs_server) == 0)
			e = gfarm_client_process_set(gfs_server, gfm_server);
		if (e == GFARM_ERR_NO_ERROR) {
			e = gfs_client_replica_add_from(gfs_server,
			    srchost, srcport, gfs_pio_fileno(gf));
			if (gfs_client_is_connection_error(e) && ++retry<=1) {
				gfs_client_connection_free(gfs_server);
				continue;
			}
		}

		break;
	}
	gfs_client_connection_free(gfs_server);
	return (e);
}

static gfarm_error_t
gfs_replicate_to_internal(char *file, char *dsthost, int dstport, int migrate)
{
	char *srchost;
	int srcport;
	gfarm_error_t e, e2;
	GFS_File gf;

	e = gfs_pio_open(file, GFARM_FILE_RDONLY, &gf);
	if (e != GFARM_ERR_NO_ERROR)
		return (e);

	e = gfarm_schedule_file(gf, &srchost, &srcport);
	if (e != GFARM_ERR_NO_ERROR)
		goto close;
	e = gfs_replicate_from_to_internal(gf, srchost, srcport,
		dsthost, dstport);
	if (e == GFARM_ERR_NO_ERROR && migrate)
		e = gfs_replica_remove_by_file(file, srchost);
	free(srchost);
 close:
	e2 = gfs_pio_close(gf);

	return (e != GFARM_ERR_NO_ERROR ? e : e2);
}

gfarm_error_t
gfs_replicate_to_local(GFS_File gf, char *srchost, int srcport)
{
	gfarm_error_t e;
	struct gfm_connection *gfm_server = gfs_pio_metadb(gf);
	char *self;
	int port;

	e = gfm_host_get_canonical_self_name(gfm_server, &self, &port);
	if (e == GFARM_ERR_NO_ERROR) {
		e = gfs_replicate_from_to_internal(gf, srchost, srcport,
		    self, port);
	}

	return (e);
}

gfarm_error_t
gfs_replicate_to(char *file, char *dsthost, int dstport)
{
	return (gfs_replicate_to_internal(file, dsthost, dstport, 0));
}

gfarm_error_t
gfs_migrate_to(char *file, char *dsthost, int dstport)
{
	return (gfs_replicate_to_internal(file, dsthost, dstport, 1));
}

gfarm_error_t
gfs_replicate_from_to(char *file, char *srchost, int srcport,
	char *dsthost, int dstport)
{
	gfarm_error_t e, e2;
	GFS_File gf;

	if (srchost == NULL)
		return (gfs_replicate_to(file, dsthost, dstport));

	e = gfs_pio_open(file, GFARM_FILE_RDONLY, &gf);
	if (e != GFARM_ERR_NO_ERROR)
		return (e);

	e = gfs_replicate_from_to_internal(gf, srchost, srcport,
		dsthost, dstport);
	e2 = gfs_pio_close(gf);

	return (e != GFARM_ERR_NO_ERROR ? e : e2);
}

gfarm_error_t
gfs_migrate_from_to(char *file, char *srchost, int srcport,
	char *dsthost, int dstport)
{
	gfarm_error_t e;

	e = gfs_replicate_from_to(file, srchost, srcport, dsthost, dstport);
	return (e != GFARM_ERR_NO_ERROR ? e :
		gfs_replica_remove_by_file(file, srchost));
}
