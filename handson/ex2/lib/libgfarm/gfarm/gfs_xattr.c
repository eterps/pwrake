/*
 * Copyright (c) 2009 National Institute of Informatics in Japan.
 * All rights reserved.
 */

#include <stdio.h>	/* config.h needs FILE */
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define GFARM_INTERNAL_USE
#include <gfarm/gfarm.h>

#include "gfutil.h"
#include "timer.h"

#include "gfm_client.h"
#include "lookup.h"
#include "gfs_io.h"
#include "gfs_misc.h"

#include "config.h"
#include "gfs_profile.h"

#include "xattr_info.h"


static double gfs_xattr_time = 0.0;

struct gfm_setxattr0_closure {
	int xmlMode;
	const char *name;
	const void *value;
	size_t size;
	int flags;
};

static gfarm_error_t
gfm_setxattr0_request(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_setxattr0_closure *c = closure;
	gfarm_error_t e = gfm_client_setxattr_request(gfm_server,
	    c->xmlMode, c->name, c->value, c->size, c->flags);

	if (e != GFARM_ERR_NO_ERROR)
		gflog_warning(GFARM_MSG_1000160,
		    "setxattr request: %s", gfarm_error_string(e));
	return (e);
}

static gfarm_error_t
gfm_setxattr0_result(struct gfm_connection *gfm_server, void *closure)
{
	gfarm_error_t e = gfm_client_setxattr_result(gfm_server);

#if 1 /* DEBUG */
	if (e != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1000161,
		    "setxattr result: %s", gfarm_error_string(e));
#endif
	return (e);
}

static gfarm_error_t
gfs_setxattr0(int xmlMode, const char *path, const char *name,
	const void *value, size_t size, int flags)
{
	gfarm_timerval_t t1, t2;
	struct gfm_setxattr0_closure closure;
	gfarm_error_t e;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	closure.xmlMode = xmlMode;
	closure.name = name;
	closure.value = value;
	closure.size = size;
	closure.flags = flags;
	e = gfm_inode_op(path, GFARM_FILE_LOOKUP,
	    gfm_setxattr0_request,
	    gfm_setxattr0_result,
	    gfm_inode_success_op_connection_free,
	    NULL,
	    &closure);

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return (e);
}

gfarm_error_t
gfs_setxattr(const char *path, const char *name,
	const void *value, size_t size, int flags)
{
	return gfs_setxattr0(0, path, name, value, size, flags);
}

gfarm_error_t
gfs_setxmlattr(const char *path, const char *name,
	const void *value, size_t size, int flags)
{
	return gfs_setxattr0(1, path, name, value, size, flags);
}

gfarm_error_t
gfs_fsetxattr(GFS_File gf, const char *name, const void *value,
	size_t size, int flags)
{
	gfarm_timerval_t t1, t2;
	struct gfm_setxattr0_closure closure;
	gfarm_error_t e;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	closure.xmlMode = 0;
	closure.name = name;
	closure.value = value;
	closure.size = size;
	closure.flags = flags;
	e = gfm_client_compound_fd_op(gfs_pio_metadb(gf), gfs_pio_fileno(gf),
	    gfm_setxattr0_request,
	    gfm_setxattr0_result,
	    NULL,
	    &closure);

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return (e);
}

struct gfm_getxattr_proccall_closure {
	int xmlMode;
	const char *name;
	void **valuep;
	size_t *sizep;
};

static gfarm_error_t
gfm_getxattr_proccall_request(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_getxattr_proccall_closure *c = closure;
	gfarm_error_t e = gfm_client_getxattr_request(gfm_server,
	    c->xmlMode, c->name);

	if (e != GFARM_ERR_NO_ERROR)
		gflog_warning(GFARM_MSG_1000162,
		    "getxattr request: %s", gfarm_error_string(e));
	return (e);
}

static gfarm_error_t
gfm_getxattr_proccall_result(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_getxattr_proccall_closure *c = closure;
	gfarm_error_t e = gfm_client_getxattr_result(gfm_server,
	    c->xmlMode, c->valuep, c->sizep);

#if 1 /* DEBUG */
	if (e != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1000163,
		    "getxattr result: %s", gfarm_error_string(e));

#endif
	return (e);
}

static gfarm_error_t
gfs_getxattr_proccall(int xmlMode, const char *path, const char *name,
	void **valuep, size_t *sizep)
{
	gfarm_timerval_t t1, t2;
	struct gfm_getxattr_proccall_closure closure;
	gfarm_error_t e;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	closure.xmlMode = xmlMode;
	closure.name = name;
	closure.valuep = valuep;
	closure.sizep = sizep;
	e = gfm_inode_op(path, GFARM_FILE_LOOKUP,
	    gfm_getxattr_proccall_request,
	    gfm_getxattr_proccall_result,
	    gfm_inode_success_op_connection_free,
	    NULL,
	    &closure);

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return (e);
}

static gfarm_error_t
gfs_fgetxattr_proccall(int xmlMode, GFS_File gf, const char *name,
	void **valuep, size_t *sizep)
{
	gfarm_timerval_t t1, t2;
	struct gfm_getxattr_proccall_closure closure;
	gfarm_error_t e;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	closure.xmlMode = xmlMode;
	closure.name = name;
	closure.valuep = valuep;
	closure.sizep = sizep;
	e = gfm_client_compound_fd_op(gfs_pio_metadb(gf), gfs_pio_fileno(gf),
	    gfm_getxattr_proccall_request,
	    gfm_getxattr_proccall_result,
	    NULL,
	    &closure);

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return (e);
}

static gfarm_error_t
gfs_getxattr0(int xmlMode, const char *path, GFS_File gf,
		const char *name, void *value, size_t *size)
{
	gfarm_error_t e;
	void *v;
	size_t s;

	if (path != NULL)
		e = gfs_getxattr_proccall(xmlMode, path, name, &v, &s);
	else
		e = gfs_fgetxattr_proccall(xmlMode, gf, name, &v, &s);
	if (e != GFARM_ERR_NO_ERROR)
		return e;

	if (*size >= s)
		memcpy(value, v, s);
	else if (*size != 0)
		e = GFARM_ERR_RESULT_OUT_OF_RANGE;
	*size = s;
	free(v);
	return e;
}

gfarm_error_t
gfs_getxattr(const char *path, const char *name, void *value, size_t *size)
{
	return gfs_getxattr0(0, path, NULL, name, value, size);
}

gfarm_error_t
gfs_getxmlattr(const char *path, const char *name, void *value, size_t *size)
{
	return gfs_getxattr0(1, path, NULL, name, value, size);
}

gfarm_error_t
gfs_fgetxattr(GFS_File gf, const char *name, void *value, size_t *size)
{
	return gfs_getxattr0(0, NULL, gf, name, value, size);
}


struct gfm_listxattr_proccall_closure {
	int xmlMode;
	char **listp;
	size_t *sizep;
};

static gfarm_error_t
gfm_listxattr_proccall_request(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_listxattr_proccall_closure *c = closure;
	gfarm_error_t e = gfm_client_listxattr_request(gfm_server, c->xmlMode);

	if (e != GFARM_ERR_NO_ERROR)
		gflog_warning(GFARM_MSG_1000164,
		    "listxattr request: %s", gfarm_error_string(e));
	return (e);
}

static gfarm_error_t
gfm_listxattr_proccall_result(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_listxattr_proccall_closure *c = closure;
	gfarm_error_t e = gfm_client_listxattr_result(gfm_server,
	    c->listp, c->sizep);

#if 1 /* DEBUG */
	if (e != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1000165,
		    "listxattr result: %s", gfarm_error_string(e));
#endif
	return (e);
}

static gfarm_error_t
gfs_listxattr_proccall(int xmlMode, const char *path,
	char **listp, size_t *sizep)
{
	gfarm_timerval_t t1, t2;
	struct gfm_listxattr_proccall_closure closure;
	gfarm_error_t e;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	closure.xmlMode = xmlMode;
	closure.listp = listp;
	closure.sizep = sizep;
	e = gfm_inode_op(path, GFARM_FILE_LOOKUP,
	    gfm_listxattr_proccall_request,
	    gfm_listxattr_proccall_result,
	    gfm_inode_success_op_connection_free,
	    NULL,
	    &closure);

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return (e);
}

static gfarm_error_t
gfs_listxattr0(int xmlMode, const char *path, char *list, size_t *size)
{
	gfarm_error_t e;
	char *l;
	size_t s;

	e = gfs_listxattr_proccall(xmlMode, path, &l, &s);
	if (e != GFARM_ERR_NO_ERROR)
		return e;

	if (*size >= s)
		memcpy(list, l, s);
	else if (*size != 0)
		e = GFARM_ERR_RESULT_OUT_OF_RANGE;
	*size = s;
	free(l);
	return e;
}

gfarm_error_t
gfs_listxattr(const char *path, char *list, size_t *size)
{
	return gfs_listxattr0(0, path, list, size);
}

gfarm_error_t
gfs_listxmlattr(const char *path, char *list, size_t *size)
{
	return gfs_listxattr0(1, path, list, size);
}


struct gfm_removexattr0_closure {
	int xmlMode;
	const char *name;
};

static gfarm_error_t
gfm_removexattr0_request(struct gfm_connection *gfm_server, void *closure)
{
	struct gfm_removexattr0_closure *c = closure;
	gfarm_error_t e = gfm_client_removexattr_request(gfm_server,
	    c->xmlMode, c->name);

	if (e != GFARM_ERR_NO_ERROR)
		gflog_warning(GFARM_MSG_1000166, "removexattr request: %s",
		    gfarm_error_string(e));
	return (e);
}

static gfarm_error_t
gfm_removexattr0_result(struct gfm_connection *gfm_server, void *closure)
{
	gfarm_error_t e = gfm_client_removexattr_result(gfm_server);

#if 1 /* DEBUG */
	if (e != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1000167,
		    "removexattr result: %s", gfarm_error_string(e));
#endif
	return (e);
}

static gfarm_error_t
gfs_removexattr0(int xmlMode, const char *path, const char *name)
{
	gfarm_timerval_t t1, t2;
	struct gfm_removexattr0_closure closure;
	gfarm_error_t e;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	closure.xmlMode = xmlMode;
	closure.name = name;
	e = gfm_inode_op(path, GFARM_FILE_LOOKUP,
	    gfm_removexattr0_request,
	    gfm_removexattr0_result,
	    gfm_inode_success_op_connection_free,
	    NULL,
	    &closure);

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return (e);
}

gfarm_error_t gfs_removexattr(const char *path, const char *name)
{
	return gfs_removexattr0(0, path, name);
}

gfarm_error_t gfs_removexmlattr(const char *path, const char *name)
{
	return gfs_removexattr0(1, path, name);
}

gfarm_error_t
gfs_fremovexattr(GFS_File gf, const char *name)
{
	gfarm_timerval_t t1, t2;
	struct gfm_removexattr0_closure closure;
	gfarm_error_t e;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	closure.xmlMode = 0;
	closure.name = name;
	e = gfm_client_compound_fd_op(gfs_pio_metadb(gf), gfs_pio_fileno(gf),
	    gfm_removexattr0_request,
	    gfm_removexattr0_result,
	    NULL,
	    &closure);

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return (e);
}

#ifndef GFARM_DEFAULT_FINDXMLATTR_NENRTY
#define GFARM_DEFAULT_FINDXMLATTR_NENRTY 100
#endif

struct gfs_xmlattr_ctx *
gfs_xmlattr_ctx_alloc(int nentry)
{
	struct gfs_xmlattr_ctx *ctxp;
	size_t ctxsize;
	int overflow;
	char *p = NULL;

	overflow = 0;
	ctxsize = gfarm_size_add(&overflow, sizeof(*ctxp),
			gfarm_size_mul(&overflow, nentry, sizeof(*ctxp->entries)));
	if (!overflow)
		p = calloc(1, ctxsize);
	if (p != NULL) {
		ctxp = (struct gfs_xmlattr_ctx *)p;
		ctxp->nalloc = nentry;
		ctxp->entries = (struct gfs_foundxattr_entry *)(ctxp + 1);
		return ctxp;
	} else
		return NULL;
}

static void
gfs_xmlattr_ctx_free_entries(struct gfs_xmlattr_ctx *ctxp, int freepath)
{
	int i;

	for (i = 0; i < ctxp->nvalid; i++) {
		if (freepath) {
			free(ctxp->entries[i].path);
			free(ctxp->entries[i].attrname);
		}
		ctxp->entries[i].path = NULL;
		ctxp->entries[i].attrname = NULL;
	}
	ctxp->index = 0;
	ctxp->nvalid = 0;
}

void
gfs_xmlattr_ctx_free(struct gfs_xmlattr_ctx *ctxp, int freepath)
{
	if (ctxp != NULL) {
		gfs_xmlattr_ctx_free_entries(ctxp, freepath);
		free(ctxp->path);
		free(ctxp->expr);
		free(ctxp->cookie_path);
		free(ctxp->cookie_attrname);
		free(ctxp->workpath);
		free(ctxp);
	}
}

gfarm_error_t
gfs_findxmlattr(const char *path, const char *expr,
	int depth, struct gfs_xmlattr_ctx **ctxpp)
{
	gfarm_error_t e;
	gfarm_timerval_t t1, t2;
	struct gfs_xmlattr_ctx *ctxp;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	if ((ctxp = gfs_xmlattr_ctx_alloc(GFARM_DEFAULT_FINDXMLATTR_NENRTY))
			== NULL)
		e = GFARM_ERR_NO_MEMORY;
	else if ((e = gfm_open_fd(path, GFARM_FILE_RDONLY,
	    &ctxp->gfm_server, &ctxp->fd, &ctxp->type)) != GFARM_ERR_NO_ERROR)
		gfs_xmlattr_ctx_free(ctxp, 1);
	else {
		ctxp->path = strdup(path);
		ctxp->expr = strdup(expr);
		ctxp->depth = depth;
		*ctxpp = ctxp;
	}

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return (e);
}

static gfarm_error_t
gfm_findxmlattr_request(struct gfm_connection *gfm_server, void *closure)
{
	struct gfs_xmlattr_ctx *ctxp = closure;
	gfarm_error_t e = gfm_client_findxmlattr_request(gfm_server, ctxp);

	if (e != GFARM_ERR_NO_ERROR)
		gflog_warning(GFARM_MSG_1000168, "find_xml_attr request: %s",
		    gfarm_error_string(e));
	return (e);
}

static gfarm_error_t
gfm_findxmlattr_result(struct gfm_connection *gfm_server, void *closure)
{
	struct gfs_xmlattr_ctx *ctxp = closure;
	gfarm_error_t e = gfm_client_findxmlattr_result(gfm_server, ctxp);

#if 1 /* DEBUG */
	if (e != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1000169,
		    "find_xml_attr result: %s", gfarm_error_string(e));
#endif
	return (e);
}

static gfarm_error_t
gfs_findxmlattr_get(struct gfs_xmlattr_ctx *ctxp)
{
	return (gfm_client_compound_fd_op(ctxp->gfm_server, ctxp->fd,
	    gfm_findxmlattr_request,
	    gfm_findxmlattr_result,
	    NULL,
	    ctxp));
}

gfarm_error_t
gfs_getxmlent(struct gfs_xmlattr_ctx *ctxp, char **fpathp, char **namep)
{
	gfarm_error_t e = GFARM_ERR_NO_ERROR;
	gfarm_timerval_t t1, t2;
	char *fpath, *p;
	int pathlen, overflow;
	size_t allocsz;

	*fpathp = NULL;
	*namep = NULL;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	if ((ctxp->eof == 0) && (ctxp->index >= ctxp->nvalid)) {
		gfs_xmlattr_ctx_free_entries(ctxp, 1);
		e = gfs_findxmlattr_get(ctxp);
	}
	if (e == GFARM_ERR_NO_ERROR) {
		if (ctxp->index < ctxp->nvalid) {
			fpath = ctxp->entries[ctxp->index].path;
			pathlen = strlen(ctxp->path);
			overflow = 0;
			allocsz = gfarm_size_add(&overflow,
				gfarm_size_add(&overflow, pathlen, strlen(fpath)), 2);
			if (!overflow)
				p = realloc(ctxp->workpath, allocsz);
			if (!overflow && (p != NULL)) {
				ctxp->workpath = p;
				if (ctxp->path[pathlen-1] == '/')
					sprintf(ctxp->workpath, "%s%s", ctxp->path, fpath);
				else
					sprintf(ctxp->workpath, "%s/%s", ctxp->path, fpath);
				pathlen = strlen(ctxp->workpath);
				if ((pathlen > 1) && (ctxp->workpath[pathlen-1] == '/'))
					ctxp->workpath[pathlen-1] = '\0';
				*fpathp = ctxp->workpath;
				*namep = ctxp->entries[ctxp->index].attrname;
				ctxp->index++;
			} else
				e = GFARM_ERR_NO_MEMORY;
		}
	}

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return e;
}

gfarm_error_t
gfs_closexmlattr(struct gfs_xmlattr_ctx *ctxp)
{
	gfarm_error_t e;
	gfarm_timerval_t t1, t2;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	gfs_profile(gfarm_gettimerval(&t1));

	if (ctxp != NULL) {
		e = gfm_close_fd(ctxp->gfm_server, ctxp->fd);
		gfm_client_connection_free(ctxp->gfm_server);
		gfs_xmlattr_ctx_free(ctxp, 1);
	} else {
		e = GFARM_ERR_NO_ERROR;
	}

	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(gfs_xattr_time += gfarm_timerval_sub(&t2, &t1));

	return (e);
}

void
gfs_xattr_display_timers(void)
{
	gflog_info(GFARM_MSG_1000170,
	    "gfs_xattr      : %g sec", gfs_xattr_time);
}
