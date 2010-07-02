#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <nl_types.h>
#include <pthread.h>

#include <gfarm/error.h>
#include <gfarm/gfarm_misc.h>

#define GFLOG_USE_STDARG
#include <gfarm/gflog.h>

#define GFARM_CATALOG_SET_NO 1

static const char *log_identifier = "libgfarm";
static char *log_auxiliary_info = NULL;
static int log_use_syslog = 0;
static int log_level = GFARM_DEFAULT_PRIORITY_LEVEL_TO_LOG;
static nl_catd catd = (nl_catd)-1;
static const char *catalog_file = "gfarm.cat";
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int log_message_verbose;

int
gflog_set_message_verbose(int new)
{
	int old = log_message_verbose;

	log_message_verbose = new;
	return (old);
}

int
gflog_syslog_enabled(void)
{
	return (log_use_syslog);
}

static void
gflog_catopen(const char *file)
{
	if (file == NULL)
		catd = catopen(catalog_file, 0);
	else
		catd = catopen(file, 0);
}

static void
gflog_catclose(void)
{
	catclose(catd);
}

void
gflog_initialize(void)
{
	gflog_catopen(NULL);
}

void
gflog_terminate(void)
{
	gflog_catclose();
}

#define GFLOG_SNPRINTF(buf, bp, endp, ...) \
{ \
	int s = snprintf(bp, (endp) - (bp), __VA_ARGS__); \
	if (s < 0 || s >= (endp) - (bp)) \
		return (buf); \
	(bp) += s; \
}

static char *
gflog_vmessage_message(int msg_no, const char *file, int line_no,
	const char *func, const char *format, va_list ap)
{
	static char buf[2048];
	char *catmsg, *bp = buf, *endp = buf + sizeof buf - 1;

	/* the last one is used as a terminator */
	*endp = '\0';

	if (!log_use_syslog)
		GFLOG_SNPRINTF(buf, bp, endp, "%s: ", log_identifier);
	GFLOG_SNPRINTF(buf, bp, endp, "[%06d] ", msg_no);
	if (log_message_verbose > 0) {
		GFLOG_SNPRINTF(buf, bp, endp, "(%s:%d", file, line_no);
		if (log_message_verbose > 1)
			GFLOG_SNPRINTF(buf, bp, endp, " %s()", func);
		GFLOG_SNPRINTF(buf, bp, endp, ") ");
	}
	if (log_auxiliary_info != NULL)
		GFLOG_SNPRINTF(buf, bp, endp, "(%s) ", log_auxiliary_info);

	catmsg = catgets(catd, GFARM_CATALOG_SET_NO, msg_no, NULL);
	vsnprintf(bp, endp - bp, catmsg != NULL ? catmsg : format, ap);

	return (buf);
}

void
gflog_vmessage(int msg_no, int priority, const char *file, int line_no,
	const char *func, const char *format, va_list ap)
{
	char *msg;

	if (priority > log_level) /* not worth reporting */
		return;

	/* gflog_vmessage_message returns statically allocated space */
	pthread_mutex_lock(&mutex);
	msg = gflog_vmessage_message(msg_no, file, line_no, func, format, ap);

	if (log_use_syslog)
		syslog(priority, "%s", msg);
	else
		fprintf(stderr, "%s\n", msg);
	pthread_mutex_unlock(&mutex);
}

void
gflog_message(int msg_no, int priority, const char *file, int line_no,
	const char *func, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	gflog_vmessage(msg_no, priority, file, line_no, func, format, ap);
	va_end(ap);
}

void
gflog_fatal_message(int msg_no, int priority, const char *file, int line_no,
	const char *func, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	gflog_vmessage(msg_no, priority, file, line_no, func, format, ap);
	va_end(ap);

	exit(2);
}

void
gflog_vmessage_errno(int msg_no, int priority, const char *file, int line_no,
	const char *func, const char *format, va_list ap)
{
	int save_errno = errno;
	char buffer[2048];

	vsnprintf(buffer, sizeof buffer, format, ap);
	gflog_message(msg_no, priority, file, line_no, func,
			"%s, %s", buffer, strerror(save_errno));
}

void
gflog_message_errno(int msg_no, int priority, const char *file, int line_no,
	const char *func, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	gflog_vmessage_errno(msg_no, priority, file, line_no, func, format, ap);
	va_end(ap);
}

void
gflog_fatal_message_errno(int msg_no, int priority, const char *file,
	int line_no, const char *func, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	gflog_vmessage_errno(msg_no, priority, file, line_no, func, format, ap);
	va_end(ap);

	exit(2);
}

void
gflog_set_priority_level(int priority)
{
	log_level = priority;
}

void
gflog_set_identifier(const char *identifier)
{
	log_identifier = identifier;
}

void
gflog_set_auxiliary_info(char *aux_info)
{
	log_auxiliary_info = aux_info;
}

char *
gflog_get_auxiliary_info(void)
{
	return (log_auxiliary_info);
}

void
gflog_syslog_open(int syslog_option, int syslog_facility)
{
	openlog(log_identifier, syslog_option, syslog_facility);
	log_use_syslog = 1;
}

int
gflog_syslog_name_to_facility(const char *name)
{
	int i;
	struct {
		const char *name;
		int facility;
	} syslog_facilities[] = {
		{ "kern",	LOG_KERN },
		{ "user",	LOG_USER },
		{ "mail",	LOG_MAIL },
		{ "daemon",	LOG_DAEMON },
		{ "auth",	LOG_AUTH },
		{ "syslog",	LOG_SYSLOG },
		{ "lpr",	LOG_LPR },
		{ "news",	LOG_NEWS },
		{ "uucp",	LOG_UUCP },
		{ "cron",	LOG_CRON },
#ifdef LOG_AUTHPRIV
		{ "authpriv",	LOG_AUTHPRIV },
#endif
#ifdef LOG_FTP
		{ "ftp",	LOG_FTP },
#endif
		{ "local0",	LOG_LOCAL0 },
		{ "local1",	LOG_LOCAL1 },
		{ "local2",	LOG_LOCAL2 },
		{ "local3",	LOG_LOCAL3 },
		{ "local4",	LOG_LOCAL4 },
		{ "local5",	LOG_LOCAL5 },
		{ "local6",	LOG_LOCAL6 },
		{ "local7",	LOG_LOCAL7 },
	};

	for (i = 0; i < GFARM_ARRAY_LENGTH(syslog_facilities); i++) {
		if (strcmp(syslog_facilities[i].name, name) == 0)
			return (syslog_facilities[i].facility);
	}
	return (-1); /* not found */
}

int
gflog_syslog_name_to_priority(const char *name)
{
	int i;
	struct {
		char *name;
		int priority;
	} syslog_priorities[] = {
		{ "emerg",	LOG_EMERG },
		{ "alert",	LOG_ALERT },
		{ "crit",	LOG_CRIT },
		{ "err",	LOG_ERR },
		{ "warning",	LOG_WARNING },
		{ "notice",	LOG_NOTICE },
		{ "info",	LOG_INFO },
		{ "debug",	LOG_DEBUG },
	};

	for (i = 0; i < GFARM_ARRAY_LENGTH(syslog_priorities); i++) {
		if (strcmp(syslog_priorities[i].name, name) == 0)
			return (syslog_priorities[i].priority);
	}
	return (-1); /* not found */
}

/*
 * authentication log
 */

static int authentication_verbose;

int
gflog_auth_set_verbose(int verbose)
{
	int old = authentication_verbose;

	authentication_verbose = verbose;
	return (old);
}

int
gflog_auth_get_verbose(void)
{
	return (authentication_verbose);
}
