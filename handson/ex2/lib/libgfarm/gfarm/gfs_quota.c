/*
 * $Id: gfs_quota.c 4312 2010-01-14 23:26:49Z takuya-i $
 */
#include <stdio.h>
#include <stdlib.h>

#define GFARM_INTERNAL_USE
#include <gfarm/gfarm.h>

#include "config.h"
#include "quota_info.h"

void
gfarm_quota_info_free(struct gfarm_quota_info *qi)
{
	if (qi->name != NULL)
		free(qi->name);
}

void
gfarm_quota_get_info_free(struct gfarm_quota_get_info *qi)
{
	if (qi->name != NULL)
		free(qi->name);
}

void
gfarm_quota_set_info_free(struct gfarm_quota_set_info *qi)
{
	if (qi->name != NULL)
		free(qi->name);
}
