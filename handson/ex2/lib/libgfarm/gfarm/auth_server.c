#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <pwd.h>

#include <gfarm/gfarm_config.h>
#include <gfarm/gflog.h>
#include <gfarm/error.h>
#include <gfarm/gfarm_misc.h>

#include "gfutil.h"

#include "liberror.h"
#include "hostspec.h"
#include "auth.h"
#include "gfp_xdr.h"

#include "gfs_proto.h" /* for GFSD_USERNAME, XXX layering violation */

static gfarm_error_t gfarm_authorize_panic(struct gfp_xdr *, int,
	char *, char *,
	gfarm_error_t (*)(void *, enum gfarm_auth_method, const char *,
	    char **), void *,
	enum gfarm_auth_id_type *, char **);

gfarm_error_t (*gfarm_authorization_table[])(struct gfp_xdr *, int,
	char *, char *,
	gfarm_error_t (*)(void *, enum gfarm_auth_method, const char *,
	    char **), void *,
	enum gfarm_auth_id_type *, char **) = {
	/*
	 * This table entry should be ordered by enum gfarm_auth_method.
	 */
	gfarm_authorize_panic,		/* GFARM_AUTH_METHOD_NONE */
	gfarm_authorize_sharedsecret,	/* GFARM_AUTH_METHOD_SHAREDSECRET */
	gfarm_authorize_panic,		/* GFARM_AUTH_METHOD_GSI_OLD */
#ifdef HAVE_GSI
	gfarm_authorize_gsi,		/* GFARM_AUTH_METHOD_GSI */
	gfarm_authorize_gsi_auth,	/* GFARM_AUTH_METHOD_GSI_AUTH */
#else
	gfarm_authorize_panic,		/* GFARM_AUTH_METHOD_GSI */
	gfarm_authorize_panic,		/* GFARM_AUTH_METHOD_GSI_AUTH */
#endif
};

static gfarm_error_t
gfarm_authorize_panic(struct gfp_xdr *conn, int switch_to,
	char *service_tag, char *hostname,
	gfarm_error_t (*auth_uid_to_global_user)(void *,
	    enum gfarm_auth_method, const char *, char **), void *closure,
	enum gfarm_auth_id_type *peer_typep, char **global_usernamep)
{
	gflog_fatal(GFARM_MSG_1000021,
	    "gfarm_authorize: authorization assertion failed");
	return (GFARM_ERR_PROTOCOL);
}

static gfarm_error_t
gfarm_auth_sharedsecret_giveup_response(
	struct gfp_xdr *conn, int try, gfarm_int32_t error)
{
	gfarm_error_t e;

	e = gfp_xdr_flush(conn);
	if (e != GFARM_ERR_NO_ERROR) {
		gflog_error(GFARM_MSG_1000022, "auth_sharedsecret: cut: %s",
		    gfarm_error_string(e));
	} else if (try <= 1) {
		e = GFARM_ERR_AUTHENTICATION;
		gflog_error(GFARM_MSG_1000023, "auth_sharedsecret: scaned: %s",
			    gfarm_error_string(e));
	} else {
		switch (error) {
		case GFARM_AUTH_ERROR_EXPIRED:
			e = GFARM_ERR_EXPIRED;
			break;
		case GFARM_AUTH_ERROR_NOT_SUPPORTED:
			e = GFARM_ERR_PROTOCOL_NOT_SUPPORTED;
			break;
		default:
			e = GFARM_ERR_AUTHENTICATION;
			break;
		}
		gflog_error(GFARM_MSG_1000024,
		    "auth_sharedsecret: gives up: %s",
		    gfarm_error_string(e));
	}
	return (e);
}

static gfarm_error_t
gfarm_auth_sharedsecret_md5_response(
	struct gfp_xdr *conn, struct passwd *pwd, gfarm_int32_t *errorp)
{
	int eof;
	size_t len;
	gfarm_uint32_t expire, expire_expected;
	char challenge[GFARM_AUTH_CHALLENGE_LEN];
	char response[GFARM_AUTH_RESPONSE_LEN];
	char shared_key_expected[GFARM_AUTH_SHARED_KEY_LEN];
	char response_expected[GFARM_AUTH_RESPONSE_LEN];
	gfarm_int32_t error; /* gfarm_auth_error */
	gfarm_error_t e;

	gfarm_auth_random(challenge, sizeof(challenge));
	e = gfp_xdr_send(conn, "b", sizeof(challenge), challenge);
	if (e == GFARM_ERR_NO_ERROR)
		e = gfp_xdr_flush(conn);
	if (e != GFARM_ERR_NO_ERROR) {
		gflog_error(GFARM_MSG_1000025,
		    "auth_sharedsecret: challenge: %s",
		    gfarm_error_string(e));
		return (e);
	}
	e = gfp_xdr_recv(conn, 0, &eof, "ib",
	    &expire, sizeof(response), &len, response);
	if (e != GFARM_ERR_NO_ERROR) {
		gflog_error(GFARM_MSG_1000026,
		    "auth_sharedsecret: response: %s",
		    gfarm_error_string(e));
		return (e);
	}
	if (eof) {
		gflog_error(GFARM_MSG_1000027, "auth_sharedsecret: "
		    "unexpected EOF in response");
		return (GFARM_ERR_PROTOCOL);
	}
	/*
	 * Note that gfarm_auth_shared_key_get() should be called
	 * after the above gfp_xdr_recv(), otherwise
	 * client (re)generated shared key may not be accessible.
	 */
	if (pwd == NULL) {
		error = GFARM_AUTH_ERROR_INVALID_CREDENTIAL;
		/* already logged at gfarm_authorize_sharedsecret() */
	} else if ((e = gfarm_auth_shared_key_get(&expire_expected,
	    shared_key_expected, pwd->pw_dir, pwd,
	    GFARM_AUTH_SHARED_KEY_GET, 0))
	    != GFARM_ERR_NO_ERROR && e != GFARM_ERR_EXPIRED) {
		error = GFARM_AUTH_ERROR_INVALID_CREDENTIAL;
		gflog_error(GFARM_MSG_1000028,
		    "auth_sharedsecret: .gfarm_shared_key: %s",
		    gfarm_error_string(e));
	} else if (time(0) >= expire) {
		/* may reach here if (e == GFARM_ERR_EXPIRED) */
		error = GFARM_AUTH_ERROR_EXPIRED;
		gflog_warning(GFARM_MSG_1000029,
		    "auth_sharedsecret: key expired");
	} else {
		/* may also reach here if (e == GFARM_ERR_EXPIRED) */
		gfarm_auth_sharedsecret_response_data(
		    shared_key_expected, challenge, response_expected);
		if (expire != expire_expected) {
			error = GFARM_AUTH_ERROR_INVALID_CREDENTIAL;
			gflog_error(GFARM_MSG_1000030, "auth_sharedsecret: "
			    "expire time mismatch");
		} else if (memcmp(response, response_expected,
		    sizeof(response)) != 0) {
			error = GFARM_AUTH_ERROR_INVALID_CREDENTIAL;
			gflog_error(GFARM_MSG_1000031, "auth_sharedsecret: "
			    "key mismatch");
		} else { /* success */
			error = GFARM_AUTH_ERROR_NO_ERROR;
		}
	}
	*errorp = error;
	e = gfp_xdr_send(conn, "i", error);
	if (e != GFARM_ERR_NO_ERROR) {
		gflog_error(GFARM_MSG_1000032,
		    "auth_sharedsecret: send result: %s",
		    gfarm_error_string(e));
		return (e);
	}
	if (error == GFARM_AUTH_ERROR_NO_ERROR) {
		e = gfp_xdr_flush(conn);
		if (e != GFARM_ERR_NO_ERROR) {
			gflog_error(GFARM_MSG_1000033,
			    "auth_sharedsecret: completion: %s",
			    gfarm_error_string(e));
			return (e);
		}
		return (GFARM_ERR_NO_ERROR); /* success */
	}
	return (GFARM_ERRMSG_AUTH_SHAREDSECRET_MD5_CONTINUE);
}

static gfarm_error_t
gfarm_auth_sharedsecret_response(struct gfp_xdr *conn, struct passwd *pwd)
{
	gfarm_error_t e;
	gfarm_uint32_t request;
	gfarm_int32_t error = GFARM_AUTH_ERROR_EXPIRED; /* gfarm_auth_error */
	int eof, try = 0;

	/* NOTE: `pwd' may be NULL, if invalid username is requested. */

	for (;;) {
		++try;
		e = gfp_xdr_recv(conn, 0, &eof, "i", &request);
		if (e != GFARM_ERR_NO_ERROR) {
			gflog_error(GFARM_MSG_1000034,
			    "auth_sharedsecret_response: %s",
			    gfarm_error_string(e));
			return (e);
		}
		if (eof) {
			gflog_error(GFARM_MSG_1000035,
			    "auth_sharedsecret_response: "
			    "unexpected EOF");
			return (GFARM_ERR_PROTOCOL);
		}
		switch (request) {
		case GFARM_AUTH_SHAREDSECRET_MD5:
		case GFARM_AUTH_SHAREDSECRET_GIVEUP:
			e = gfp_xdr_send(conn, "i", GFARM_AUTH_ERROR_NO_ERROR);
			break;
		default:
			error = GFARM_AUTH_ERROR_NOT_SUPPORTED;
			e = gfp_xdr_send(conn, "i", error);
			break;
		}			
		if (e != GFARM_ERR_NO_ERROR) {
			gflog_error(GFARM_MSG_1000036,
			    "auth_sharedsecret: key query: %s",
			    gfarm_error_string(e));
			return (e);
		}
		switch (request) {
		case GFARM_AUTH_SHAREDSECRET_GIVEUP:
			return (gfarm_auth_sharedsecret_giveup_response(
					conn, try, error));
		case GFARM_AUTH_SHAREDSECRET_MD5:
			e = gfarm_auth_sharedsecret_md5_response(
				conn, pwd, &error);
			if (e != GFARM_ERRMSG_AUTH_SHAREDSECRET_MD5_CONTINUE)
				return (e);
		default:
			e = gfp_xdr_flush(conn);
			if (e != GFARM_ERR_NO_ERROR) {
				gflog_error(GFARM_MSG_1000037,
				    "auth_sharedsecret: request "
				    "response: %s", gfarm_error_string(e));
				return (e);
			}
			break;
		}
	}
}

static pthread_once_t getpwnam_r_bufsz_initialized = PTHREAD_ONCE_INIT;
static int getpwnam_r_bufsz = 0;
#define BUFSIZE_MAX 2048

static void
getpwnam_r_bufsz_initialize(void)
{
	/* Solaris calls this function more than once with non-pthread apps */
	if (getpwnam_r_bufsz != 0)
		return;

	getpwnam_r_bufsz = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (getpwnam_r_bufsz == -1)
		getpwnam_r_bufsz = BUFSIZE_MAX;
}

gfarm_error_t
gfarm_authorize_sharedsecret(struct gfp_xdr *conn, int switch_to,
	char *service_tag, char *hostname,
	gfarm_error_t (*auth_uid_to_global_user)(void *,
	    enum gfarm_auth_method, const char *, char **), void *closure,
	enum gfarm_auth_id_type *peer_typep, char **global_usernamep)
{
	gfarm_error_t e;
	char *global_username, *local_username, *aux, *buf = NULL;
	int eof;
	enum gfarm_auth_id_type peer_type;
	struct passwd pwbuf, *pwd;

	e = gfp_xdr_recv(conn, 0, &eof, "s", &global_username);
	if (e != GFARM_ERR_NO_ERROR) {
		gflog_error(GFARM_MSG_1000038,
		    "authorize_sharedsecret: reading username");
		return (e);
	}
	if (eof) {
		gflog_error(GFARM_MSG_1000039,
		    "authorize_sharedsecret: unexpected EOF");
		return (GFARM_ERR_PROTOCOL);
	}

	if (strcmp(global_username, GFSD_USERNAME) == 0) {
		peer_type = GFARM_AUTH_ID_TYPE_SPOOL_HOST;
	} else {
		/*
		 * actually, a protocol-level uid is a gfarm global username
		 * in sharedsecret case.
		 * so, the purpose of (*auth_uid_to_global_user)() is
		 * to verify whether the user does exist or not in this case.
		 */
		peer_type = GFARM_AUTH_ID_TYPE_USER;
		e = (*auth_uid_to_global_user)(closure,
		    GFARM_AUTH_METHOD_SHAREDSECRET, global_username, NULL);
		if (e != GFARM_ERR_NO_ERROR)
			gflog_error(GFARM_MSG_1000040,
			    "(%s@%s) authorize_sharedsecret: "
			    "the global username isn't registered in gfmd: %s",
			    global_username, hostname, gfarm_error_string(e));
	}
	if (e == GFARM_ERR_NO_ERROR) {
		e = gfarm_global_to_local_username(global_username,
		    &local_username);
		if (e != GFARM_ERR_NO_ERROR)
			gflog_error(GFARM_MSG_1000041,
			    "(%s@%s) authorize_sharedsecret: "
			    "cannot map global username into local username: "
			    "%s",
			    global_username, hostname, gfarm_error_string(e));
	}
	if (e != GFARM_ERR_NO_ERROR) {
		local_username = NULL;
		pwd = NULL;
	} else {
		pthread_once(&getpwnam_r_bufsz_initialized,
		    getpwnam_r_bufsz_initialize);
		buf = malloc(getpwnam_r_bufsz);
		if (buf == NULL) {
			e = GFARM_ERR_NO_MEMORY;
			gflog_error(GFARM_MSG_1000042,
			    "(%s@%s) %s: authorize_sharedsecret: %s",
			    global_username, hostname, local_username,
			    gfarm_error_string(e));
			free(local_username);
			free(global_username);
			return (e);
		}
		if (getpwnam_r(local_username, &pwbuf, buf, getpwnam_r_bufsz,
		    &pwd) != 0)
			gflog_error(GFARM_MSG_1000043,
			    "(%s@%s) %s: authorize_sharedsecret: "
			    "local account doesn't exist",
			    global_username, hostname, local_username);
	}

	e = gfarm_auth_sharedsecret_response(conn, pwd); /* pwd may be NULL */

	/* if (pwd == NULL), must be (e != GFARM_ERR_NO_ERROR) here */
	if (e != GFARM_ERR_NO_ERROR) {
		if (local_username != NULL)
			free(local_username);
		free(global_username);
		if (buf != NULL)
			free(buf);
		return (e);
	}
	assert(local_username != NULL);

	/* succeed, do logging */
	gflog_notice(GFARM_MSG_1000044,
	    "(%s@%s) authenticated: auth=sharedsecret local_user=%s",
	    global_username, hostname, local_username);

	if (switch_to) {
		GFARM_MALLOC_ARRAY(aux,
		    strlen(global_username) + 1 + strlen(hostname) + 1);
		if (aux == NULL) {
			gflog_error(GFARM_MSG_1000045,
			    "(%s@%s) authorize_sharedsecret: %s",
			    global_username, hostname,
			    gfarm_error_string(GFARM_ERR_NO_MEMORY));
			free(local_username);
			free(global_username);
			if (buf != NULL)
				free(buf);
			return (GFARM_ERR_NO_MEMORY);
		}
		sprintf(aux, "%s@%s", global_username, hostname);
		gflog_set_auxiliary_info(aux);

		/*
		 * because the name returned by getlogin() is
		 * an attribute of a session on 4.4BSD derived OSs,
		 * we should create new session before calling
		 * setlogin().
		 */
		seteuid(0); /* make sure to have root privilege */
		setsid();
#ifdef HAVE_SETLOGIN
		setlogin(pwd->pw_name);
#endif
		initgroups(pwd->pw_name, pwd->pw_gid);
		setgid(pwd->pw_gid);
		setuid(pwd->pw_uid);

		gfarm_set_global_username(global_username);
		gfarm_set_local_username(local_username);
		gfarm_set_local_homedir(pwd->pw_dir);
	}
	free(local_username);
	if (peer_typep != NULL)
		*peer_typep = peer_type;
	if (global_usernamep != NULL)
		*global_usernamep = global_username;
	else
		free(global_username);
	if (buf != NULL)
		free(buf);
	return (GFARM_ERR_NO_ERROR);
}

/*
 * the `switch_to' flag has the following side effects:
 *	- gfarm_authorize() isn't thread safe.
 *      - the privilege of this program will switch to the authenticated user.
 *      - gflog_set_auxiliary_info("user@hostname") will be called.
 *        thus, the caller of gfarm_authorize() must call the following later:
 *              char *aux = gflog_get_auxiliary_info();
 *              gflog_get_auxiliary_info(NULL);
 *              free(aux);
 *      - gfarm_get_local_username(), gfarm_get_local_homedir() and
 *        gfarm_get_global_username() become available.
 *
 * note that the user's account is not always necessary on this host,
 * if the `switch_to' flag isn't set. but also note that some
 * authentication methods (e.g. "sharedsecret") require the user's
 * local account anyway even if the `switch_to' isn't set.
 */
gfarm_error_t
gfarm_authorize(struct gfp_xdr *conn,
	int switch_to, char *service_tag,
	char *hostname, struct sockaddr *addr,
	gfarm_error_t (*auth_uid_to_global_user)(void *,
	    enum gfarm_auth_method, const char *, char **), void *closure,
	enum gfarm_auth_id_type *peer_typep, char **global_usernamep,
	enum gfarm_auth_method *auth_methodp)
{
	gfarm_error_t e;
	gfarm_int32_t methods; /* bitset of enum gfarm_auth_method */
	gfarm_int32_t method; /* enum gfarm_auth_method */
	gfarm_int32_t error; /* enum gfarm_auth_error */
	int i, eof, try = 0;
	size_t nmethods;
	unsigned char methods_buffer[CHAR_BIT * sizeof(gfarm_int32_t)];

	assert(GFARM_ARRAY_LENGTH(gfarm_authorization_table) ==
	    GFARM_AUTH_METHOD_NUMBER);

	methods = gfarm_auth_method_get_enabled_by_name_addr(hostname, addr);
	if (methods == 0) {
		gflog_error(GFARM_MSG_1000046,
		    "%s: refusing access", hostname);
	} else {
		methods &= gfarm_auth_method_get_available();
		if (methods == 0)
			gflog_error(GFARM_MSG_1000047,
			    "%s: auth-method not configured",
			    hostname);
	}

	nmethods = 0;
	for (i = GFARM_AUTH_METHOD_NONE + 1; i < GFARM_AUTH_METHOD_NUMBER &&
	    i < CHAR_BIT * sizeof(gfarm_int32_t); i++) {
		if ((methods & (1 << i)) != 0)
			methods_buffer[nmethods++] = i;
	}
	e = gfp_xdr_send(conn, "b", nmethods, methods_buffer);
	if (e == GFARM_ERR_NO_ERROR)
		e = gfp_xdr_flush(conn);
	if (e != GFARM_ERR_NO_ERROR) {
		gflog_error(GFARM_MSG_1000048,
		    "%s: %s", hostname, gfarm_error_string(e));
		return (e);
	}
	for (;;) {
		++try;
		e = gfp_xdr_recv(conn, 0, &eof, "i", &method);
		if (e != GFARM_ERR_NO_ERROR) {
			gflog_error(GFARM_MSG_1000049,
			    "%s: %s", hostname, gfarm_error_string(e));
			return (e);
		}
		if (eof) {
			if (try <= 1)
				gflog_warning(GFARM_MSG_1000050,
				    "%s: port scan", hostname);
			else
				gflog_warning(GFARM_MSG_1000051,
				    "%s: client disappeared",
				    hostname);
			return (GFARM_ERR_PROTOCOL);
		}
		if (method == GFARM_AUTH_METHOD_NONE)
			error = GFARM_AUTH_ERROR_NO_ERROR;
		else if (method >= GFARM_AUTH_METHOD_NUMBER)
			error = GFARM_AUTH_ERROR_NOT_SUPPORTED;
		else if (method <= GFARM_AUTH_METHOD_NONE ||
		    ((1 << method) & methods) == 0)
			error = GFARM_AUTH_ERROR_DENIED;
		else if (gfarm_authorization_table[method] ==
		    gfarm_authorize_panic)
			error = GFARM_AUTH_ERROR_NOT_SUPPORTED;
		else
			error = GFARM_AUTH_ERROR_NO_ERROR;
		e = gfp_xdr_send(conn, "i", error);
		if (e == GFARM_ERR_NO_ERROR)
			e = gfp_xdr_flush(conn);
		if (e != GFARM_ERR_NO_ERROR) {
			gflog_error(GFARM_MSG_1000052,
			    "%s: %s", hostname, gfarm_error_string(e));
			return (e);
		}
		if (error != GFARM_AUTH_ERROR_NO_ERROR) {
			gflog_error(GFARM_MSG_1000053,
			    "%s: incorrect auth-method request",
			    hostname);
			return (GFARM_ERR_PROTOCOL);
		}
		if (method == GFARM_AUTH_METHOD_NONE) {
			/* client gave up */
			if (methods == 0) {
				e = GFARM_ERR_PERMISSION_DENIED;
			} else if (try <= 1) {
				/*
				 * there is no usable auth-method
				 * between client and server.
				 */
				gflog_notice(GFARM_MSG_1000054,
				    "%s: authentication method "
				    "doesn't match", hostname);
				e = GFARM_ERR_PROTOCOL_NOT_SUPPORTED;
			} else {
				e = GFARM_ERR_AUTHENTICATION;
			}
			return (e);
		}

		e = (*gfarm_authorization_table[method])(conn, switch_to,
		    service_tag, hostname, auth_uid_to_global_user, closure,
		    peer_typep, global_usernamep);
		if (e != GFARM_ERR_PROTOCOL_NOT_SUPPORTED &&
		    e != GFARM_ERR_EXPIRED &&
		    e != GFARM_ERR_AUTHENTICATION) {
			/* protocol error, or success */
			if (e == GFARM_ERR_NO_ERROR) {
				if (auth_methodp != NULL)
					*auth_methodp = method;
			}
			return (e);
		}
	}
}
