/*
 * $Id: host.c 4162 2009-05-22 05:27:03Z n-soda $
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#define _SOCKADDR_LEN /* for __osf__ */
#include <sys/socket.h>
#include <sys/uio.h>
#define BSD_COMP /* for __svr4__ */
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/un.h> /* for SUN_LEN */
#include <errno.h>

#include <unistd.h>
#include <netdb.h>

#include <gfarm/gfarm.h>

#include "gfnetdb.h"

#include "hostspec.h"
#include "gfm_client.h"
#include "host.h"

static gfarm_error_t
host_info_get_by_name_alias(struct gfm_connection *gfm_server,
	const char *hostname, struct gfarm_host_info *info)
{
	gfarm_error_t e, e2;

	e = gfm_client_host_info_get_by_namealiases(gfm_server,
	    1, &hostname, &e2, info);
	return (e != GFARM_ERR_NO_ERROR ? e : e2);
}

gfarm_error_t
gfm_host_info_get_by_name_alias(struct gfm_connection *gfm_server,
	const char *if_hostname, struct gfarm_host_info *info)
{
	gfarm_error_t e;
	struct hostent *hp;
	int i;
	char *n;

	e = host_info_get_by_name_alias(gfm_server, if_hostname, info);
	if (e == GFARM_ERR_NO_ERROR)
		return (GFARM_ERR_NO_ERROR);

	/* XXX should not use gethostbyname(3), but ...*/
	/*
	 * This interface is never called from gfmd,
	 * so MPSAFE-ness is not an issue for now.
	 * Unlike gethostbyname(), getaddrinfo() doesn't return multiple
	 * host aliases, so we cannot use getaddrino() here to see alias names.
	 *
	 * FIXME: This design must be revised, when we support IPv6.
	 */
	hp = gethostbyname(if_hostname);
	if (hp == NULL || hp->h_addrtype != AF_INET)
		return (GFARM_ERR_UNKNOWN_HOST);
	for (i = 0, n = hp->h_name; n != NULL; n = hp->h_aliases[i++]){
		if (host_info_get_by_name_alias(gfm_server, n, info) ==
		    GFARM_ERR_NO_ERROR)
			return (GFARM_ERR_NO_ERROR);
	}
	return (e);
}

/*
 * The value returned to `*canonical_hostnamep' should be freed.
 */
gfarm_error_t
gfm_host_get_canonical_name(struct gfm_connection *gfm_server,
	const char *hostname, char **canonical_hostnamep, int *portp)
{
	gfarm_error_t e;
	struct gfarm_host_info info;
	int port;
	char *n;

	e = gfm_host_info_get_by_name_alias(gfm_server, hostname, &info);
	if (e != GFARM_ERR_NO_ERROR)
		return (e);

	n = strdup(info.hostname);
	port = info.port;
	gfarm_host_info_free(&info);
	if (n == NULL)
		return (GFARM_ERR_NO_MEMORY);
	*canonical_hostnamep = n;
	*portp = port;
	return (GFARM_ERR_NO_ERROR);
}

char *
gfarm_host_get_self_name(void)
{
	static int initialized;
	static char hostname[MAXHOSTNAMELEN + 1];

	if (!initialized) {
		hostname[0] = hostname[MAXHOSTNAMELEN] = 0;
		/* gethostname(2) almost shouldn't fail */
		gethostname(hostname, MAXHOSTNAMELEN);
		if (hostname[0] == '\0')
			strcpy(hostname, "hostname-not-set");
		initialized = 1;
	}

	return (hostname);
}

/*
 * shouldn't free the return value of this function.
 *
 * NOTE: gfarm_error_initialize() and gfarm_metadb_initialize()
 *	should be called before this function.
 */
gfarm_error_t
gfm_host_get_canonical_self_name(struct gfm_connection *gfm_server,
	char **canonical_hostnamep, int *portp)
{
	gfarm_error_t e;
	static char *canonical_self_name = NULL;
	static int port;
	static gfarm_error_t error_save = GFARM_ERR_NO_ERROR;

	if (canonical_self_name == NULL) {
		if (error_save != GFARM_ERR_NO_ERROR)
			return (error_save);
		e = gfm_host_get_canonical_name(gfm_server,
		    gfarm_host_get_self_name(), &canonical_self_name, &port);
		if (e != GFARM_ERR_NO_ERROR) {
			error_save = e;
			return (e);
		}
	}
	*canonical_hostnamep = canonical_self_name;
	*portp = port;
	return (GFARM_ERR_NO_ERROR);
}


#if 0 /* not yet in gfarm v2 */

static int
host_address_is_match(struct gfarm_hostspec *hostspec,
	const char *name, struct hostent *hp)
{
	struct sockaddr_in peer_addr_in;
	struct sockaddr *peer_addr = (struct sockaddr *)&peer_addr_in;
	int i, j;
	const char *n;

	if (hp == NULL || hp->h_addrtype != AF_INET)
		return (gfarm_hostspec_match(hostspec, name, NULL));

	memset(&peer_addr_in, 0, sizeof(peer_addr_in));
	peer_addr_in.sin_port = 0;
	peer_addr_in.sin_family = hp->h_addrtype;
	for (i = 0; hp->h_addr_list[i] != NULL; i++) {
		memcpy(&peer_addr_in.sin_addr, hp->h_addr_list[i],
		    sizeof(peer_addr_in.sin_addr));
		if (gfarm_hostspec_match(hostspec, name, peer_addr))
			return (1);
		if (gfarm_hostspec_match(hostspec, hp->h_name, peer_addr))
			return (1);
		for (j = 0; (n = hp->h_aliases[j]) != NULL; j++) {
			if (gfarm_hostspec_match(hostspec, n, peer_addr))
				return (1);
		}
	}
	return (0);
}

struct gfarm_client_architecture_config {
	struct gfarm_client_architecture_config *next;

	char *architecture;
	struct gfarm_hostspec *hostspec;
};

struct gfarm_client_architecture_config
	*gfarm_client_architecture_config_list = NULL;
struct gfarm_client_architecture_config
	**gfarm_client_architecture_config_last =
	    &gfarm_client_architecture_config_list;

gfarm_error_t
gfarm_set_client_architecture(char *architecture, struct gfarm_hostspec *hsp)
{
	struct gfarm_client_architecture_config *cacp;

	GFARM_MALLOC(cacp);
	if (cacp == NULL)
		return (GFARM_ERR_NO_MEMORY);

	cacp->architecture = strdup(architecture);
	if (cacp->architecture == NULL) {
		free(cacp);
		return (GFARM_ERR_NO_MEMORY);
	}
	cacp->hostspec = hsp;
	cacp->next = NULL;

	*gfarm_client_architecture_config_last = cacp;
	gfarm_client_architecture_config_last = &cacp->next;
	return (GFARM_ERR_NO_ERROR);
}

static gfarm_error_t
gfarm_get_client_architecture(const char *client_name, char **architecturep)
{
	struct hostent *hp;
	struct gfarm_client_architecture_config *cacp =
	    gfarm_client_architecture_config_list;

	if (cacp == NULL)
		return (GFARM_ERR_NO_SUCH_OBJECT);
	hp = gethostbyname(client_name);
	for (; cacp != NULL; cacp = cacp->next) {
		if (host_address_is_match(cacp->hostspec, client_name, hp)) {
			*architecturep = cacp->architecture;
			return (GFARM_ERR_NO_ERROR);
		}
	}
	return (GFARM_ERR_NO_SUCH_OBJECT);
}


/*
 * shouldn't free the return value of this function.
 *
 * NOTE: gfarm_error_initialize() and gfarm_metadb_initialize()
 *	should be called before this function.
 */
gfarm_error_t
gfarm_host_get_self_architecture(char **architecture)
{
	gfarm_error_t e;
	char *canonical_self_name;
	static char *self_architecture = NULL;
	static gfarm_error_t error_save = GFARM_ERR_NO_ERROR;

	if (self_architecture == NULL) {
		if (error_save != GFARM_ERR_NO_ERROR)
			return (error_save);

		if ((self_architecture = getenv("GFARM_ARCHITECTURE"))!= NULL){
			/* do nothing */
		} else if ((e = gfarm_host_get_canonical_self_name(
		    &canonical_self_name)) == GFARM_ERR_NO_ERROR) {
			/* filesystem node case */
			self_architecture =
			    gfarm_host_info_get_architecture_by_host(
			    canonical_self_name);
			if (self_architecture == NULL) {
				error_save = GFARM_ERR_NO_SUCH_OBJECT;
				return (error_save);
			}
		} else if ((e = gfarm_get_client_architecture(
		    gfarm_host_get_self_name(), &self_architecture)) ==
		    GFARM_ERR_NO_ERROR) {
			/* client case */
			/* do nothing */
		} else {
			error_save = e;
			return (e);
		}
	}
	*architecture = self_architecture;
	return (GFARM_ERR_NO_ERROR);
}

#endif /* not yet in gfarm v2 */

static int
gfm_canonical_hostname_is_local(struct gfm_connection *gfm_server,
	const char *canonical_hostname)
{
	gfarm_error_t e;
	char *self_name;
	int port;

	e = gfm_host_get_canonical_self_name(gfm_server, &self_name, &port);
	if (e != GFARM_ERR_NO_ERROR)
		self_name = gfarm_host_get_self_name();
	return (strcasecmp(canonical_hostname, self_name) == 0);
}

int
gfm_host_is_local(struct gfm_connection *gfm_server, const char *hostname)
{
	gfarm_error_t e;
	char *canonical_hostname;
	int is_local, port;

	e = gfm_host_get_canonical_name(gfm_server, hostname,
	    &canonical_hostname, &port);
	is_local = gfm_canonical_hostname_is_local(gfm_server,
	    canonical_hostname);
	if (e == GFARM_ERR_NO_ERROR)
		free(canonical_hostname);
	return (is_local);
}

#ifdef SUN_LEN
# ifndef linux
#  define NEW_SOCKADDR /* 4.3BSD-Reno or later */
# endif
#endif

#define ADDRESSES_DELTA 16
#define IFCBUFFER_SIZE	8192

gfarm_error_t
gfarm_get_ip_addresses(int *countp, struct in_addr **ip_addressesp)
{
	gfarm_error_t e = GFARM_ERR_NO_MEMORY;
	int fd;
	int size, count;
#ifdef NEW_SOCKADDR
	int i;
#endif
	struct in_addr *addresses, *p;
	struct ifreq *ifr; /* pointer to interface address */
	struct ifconf ifc; /* buffer for interface addresses */
	char ifcbuffer[IFCBUFFER_SIZE];
	struct ifreq ifreq; /* buffer for interface flag */

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return (gfarm_errno_to_error(errno));
	ifc.ifc_len = sizeof(ifcbuffer);
	ifc.ifc_buf = ifcbuffer;
	if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
		e = gfarm_errno_to_error(errno);
		close(fd);
		return (e);
	}

	count = 0;
	size = 2; /* ethernet address + loopback interface address */
	GFARM_MALLOC_ARRAY(addresses,  size);
	if (addresses == NULL)
		goto err;

#ifdef NEW_SOCKADDR
	ifreq.ifr_name[0] = '\0';

	for (i = 0; i < ifc.ifc_len; )
#else
	for (ifr = ifc.ifc_req; (char *)ifr < ifc.ifc_buf+ifc.ifc_len; ifr++)
#endif
	{
#ifdef NEW_SOCKADDR
		ifr = (struct ifreq *)((char *)ifc.ifc_req + i);
		i += sizeof(ifr->ifr_name) +
			((ifr->ifr_addr.sa_len > sizeof(struct sockaddr) ?
			  ifr->ifr_addr.sa_len : sizeof(struct sockaddr)));
#endif
		if (ifr->ifr_addr.sa_family != AF_INET)
			continue;
#ifdef NEW_SOCKADDR
		if (strncmp(ifreq.ifr_name, ifr->ifr_name,
			    sizeof(ifr->ifr_name)) != 0)
#endif
		{
			/* if this is first entry of the interface, get flag */
			ifreq = *ifr;
			if (ioctl(fd, SIOCGIFFLAGS, &ifreq) < 0) {
				e = gfarm_errno_to_error(errno);
				goto err;
			}
		}
		if ((ifreq.ifr_flags & IFF_UP) == 0)
			continue;

		if (count + 1 > size) {
			size += ADDRESSES_DELTA;
			GFARM_REALLOC_ARRAY(p, addresses, size);
			if (p == NULL)
				goto err;
			addresses = p;
		}
		addresses[count++] =
			((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr;

	}
	if (count == 0) {
		free(addresses);
		addresses = NULL;
	} else if (size != count) {
		GFARM_REALLOC_ARRAY(p, addresses, count);
		if (p == NULL)
			goto err;
		addresses = p;
	}
	*ip_addressesp = addresses;
	*countp = count;
	close(fd);
	return (GFARM_ERR_NO_ERROR);

err:
	if (addresses != NULL)
		free(addresses);
	close(fd);
	return (e);
}

#if 0 /* "address_use" directive is disabled for now */

struct gfarm_host_address_use_config {
	struct gfarm_host_address_use_config *next;

	struct gfarm_hostspec *hostspec;
};

struct gfarm_host_address_use_config *gfarm_host_address_use_config_list =
    NULL;
struct gfarm_host_address_use_config **gfarm_host_address_use_config_last =
    &gfarm_host_address_use_config_list;

gfarm_error_t
gfarm_host_address_use(struct gfarm_hostspec *hsp)
{
	struct gfarm_host_address_use_config *haucp;

	GFARM_MALLOC(haucp);
	if (haucp == NULL)
		return (GFARM_ERR_NO_MEMORY);

	haucp->hostspec = hsp;
	haucp->next = NULL;

	*gfarm_host_address_use_config_last = haucp;
	gfarm_host_address_use_config_last = &haucp->next;
	return (GFARM_ERR_NO_ERROR);
}

#endif /* "address_use" directive is disabled for now */

static int
always_match(struct gfarm_hostspec *hostspecp,
	const char *name, struct sockaddr *addr)
{
	return (1);
}

/* XXX should try to connect all IP addresses. i.e. this interface is wrong. */
static gfarm_error_t
host_address_get(const char *name, int port,
	int (*match)(struct gfarm_hostspec *, const char *, struct sockaddr *),
	struct gfarm_hostspec *hostspec,
	struct sockaddr *peer_addr, char **if_hostnamep)
{
	struct addrinfo hints, *res, *res0;
	int error;
	char *n, sbuf[NI_MAXSERV];

	snprintf(sbuf, sizeof(sbuf), "%u", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; /* XXX maybe used for SOCK_DGRAM */
	error = gfarm_getaddrinfo(name, sbuf, &hints, &res0);
	if (error != 0)
		return (GFARM_ERR_UNKNOWN_HOST);

	for (res = res0; res != NULL; res = res->ai_next) {
		if ((*match)(hostspec, name, res->ai_addr)) {
			/* to be sure */
			if (res0->ai_addr->sa_family != AF_INET ||
			    res0->ai_addrlen > sizeof(*peer_addr)) {
				gfarm_freeaddrinfo(res0);
				return (GFARM_ERR_ADDRESS_FAMILY_NOT_SUPPORTED_BY_PROTOCOL_FAMILY);
			}
			if (if_hostnamep != NULL) {
				/* XXX - or strdup(res0->ai_canonname)? */
				n = strdup(name); 
				if (n == NULL) {
					gfarm_freeaddrinfo(res0);
					return (GFARM_ERR_NO_MEMORY);
				}
				*if_hostnamep = n;
			}
			memset(peer_addr, 0, sizeof(*peer_addr));
			memcpy(peer_addr, res0->ai_addr, sizeof(*peer_addr));
			gfarm_freeaddrinfo(res0);
			return (GFARM_ERR_NO_ERROR);
		}
	}
	gfarm_freeaddrinfo(res0);
	return (GFARM_ERR_NO_SUCH_OBJECT);
}

static gfarm_error_t
host_address_get_matched(const char *name, int port,
	struct gfarm_hostspec *hostspec,
	struct sockaddr *peer_addr, char **if_hostnamep)
{
	return (host_address_get(name, port,
	    hostspec == NULL ? always_match: gfarm_hostspec_match, hostspec,
	    peer_addr, if_hostnamep));
}

static gfarm_error_t
host_info_address_get_matched(struct gfarm_host_info *info, int port,
	struct gfarm_hostspec *hostspec,
	struct sockaddr *peer_addr, char **if_hostnamep)
{
	gfarm_error_t e;
	int i;

	e = host_address_get_matched(info->hostname, port, hostspec,
	    peer_addr, if_hostnamep);
	if (e == GFARM_ERR_NO_ERROR)
		return (GFARM_ERR_NO_ERROR);
	for (i = 0; i < info->nhostaliases; i++) {
		e = host_address_get_matched(info->hostaliases[i], port,
		    hostspec, peer_addr, if_hostnamep);
		if (e == GFARM_ERR_NO_ERROR)
			return (GFARM_ERR_NO_ERROR);
	}
	return (e);
}

struct host_info_rec {
	struct gfarm_host_info *info;
	int tried, got;
};

static gfarm_error_t
address_get_matched(struct gfm_connection *gfm_server,
	const char *name, struct host_info_rec *hir, int port,
	struct gfarm_hostspec *hostspec,
	struct sockaddr *peer_addr, char **if_hostnamep)
{
	gfarm_error_t e;

	e = host_address_get_matched(name, port, hostspec,
	    peer_addr, if_hostnamep);
	if (e == GFARM_ERR_NO_ERROR)
		return (GFARM_ERR_NO_ERROR);
	if (!hir->tried) {
		hir->tried = 1;
		if (gfm_host_info_get_by_name_alias(gfm_server,
		    name, hir->info) == GFARM_ERR_NO_ERROR)
			hir->got = 1;
	}
	if (hir->got) {
		e = host_info_address_get_matched(hir->info, port, hostspec,
		    peer_addr, if_hostnamep);
	}
	return (e);
}

static gfarm_error_t
address_get(struct gfm_connection *gfm_server, const char *name,
	struct host_info_rec *hir, int port,
	struct sockaddr *peer_addr, char **if_hostnamep)
{
#if 0 /* "address_use" directive is disabled for now */
	if (gfarm_host_address_use_config_list != NULL) {
		struct gfarm_host_address_use_config *config;

		for (config = gfarm_host_address_use_config_list;
		    config != NULL; config = config->next) {
			if (address_get_matched(gfm_server,
			    name, hir, port, config->hostspec,
			    peer_addr, if_hostnamep) == GFARM_ERR_NO_ERROR)
				return (GFARM_ERR_NO_ERROR);
		}
	}
#endif /* "address_use" directive is disabled for now */
	return (address_get_matched(gfm_server, name, hir, port, NULL,
	    peer_addr, if_hostnamep));
}

gfarm_error_t
gfm_host_info_address_get(struct gfm_connection *gfm_server,
	const char *host, int port,
	struct gfarm_host_info *info,
	struct sockaddr *peer_addr, char **if_hostnamep)
{
	struct host_info_rec hir;

	hir.info = info;
	hir.tried = hir.got = 1;
	return (address_get(gfm_server, host, &hir, port, peer_addr, 
	    if_hostnamep));
}

gfarm_error_t
gfm_host_address_get(struct gfm_connection *gfm_server,
	const char *host, int port,
	struct sockaddr *peer_addr, char **if_hostnamep)
{
	gfarm_error_t e;
	struct gfarm_host_info info;
	struct host_info_rec hir;

	hir.info = &info;
	hir.tried = hir.got = 0;
	e = address_get(gfm_server, host, &hir, port, peer_addr, if_hostnamep);
	if (hir.got)
		gfarm_host_info_free(&info);
	return (e);
}

/*
 * `*widendep' is only set, when this function returns True.
 * `*widendep' means:
 * -1: the addr is adjacent to the lower bound of the min address.
 *  0: the addr is between min and max.
 *  1: the addr is adjacent to the upper bound of the max address.
 *
 * XXX mostly works, but by somewhat haphazard way, if wild_guess is set.
 */
int
gfarm_addr_is_same_net(struct sockaddr *addr,
	struct sockaddr *min, struct sockaddr *max, int wild_guess,
	int *widenedp)
{
	gfarm_uint32_t addr_in, min_in, max_in;
	gfarm_uint32_t addr_net, min_net, max_net;

	assert(addr->sa_family == AF_INET &&
	    min->sa_family == AF_INET &&
	    max->sa_family == AF_INET);
	addr_in = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	min_in = ntohl(((struct sockaddr_in *)min)->sin_addr.s_addr);
	max_in = ntohl(((struct sockaddr_in *)max)->sin_addr.s_addr);
	if (min_in <= addr_in && addr_in <= max_in) {
		*widenedp = 0;
		return (1);
	}
	if (!wild_guess) /* `*widenedp' is always false, if !wild_guess */
		return (0);
	/* do wild guess */

	/* XXX - get IPv4 C class part */
	addr_net = (addr_in >> 8) & 0xffffff;
	min_net = (min_in >> 8) & 0xffffff;
	max_net = (max_in >> 8) & 0xffffff;
	/* adjacent or same IPv4 C class? */
	if (addr_net == min_net - 1 ||
	    (addr_net == min_net && addr_in < min_in)) {
		*widenedp = -1;
		return (1);
	}
	if (addr_net == max_net + 1 ||
	    (addr_net == max_net && addr_in > max_in)) {
		*widenedp = 1;
		return (1);
	}
	return (0);
}

gfarm_error_t
gfarm_addr_range_get(struct sockaddr *addr,
	struct sockaddr *min, struct sockaddr *max)
{
	gfarm_uint32_t addr_in, addr_net;

	assert(addr->sa_family == AF_INET);
	addr_in = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	/* XXX - get IPv4 C class part */
	addr_net = addr_in & 0xffffff00;

	/* XXX - minimum & maximum address in the IPv4 C class net  */
	memset(min, 0, sizeof(*min));
	min->sa_family = AF_INET;
	((struct sockaddr_in *)min)->sin_addr.s_addr = htonl(addr_net);

	memset(max, 0, sizeof(*max));
	max->sa_family = AF_INET;
	((struct sockaddr_in *)max)->sin_addr.s_addr = htonl(addr_net | 0xff);
	return (GFARM_ERR_NO_ERROR);
}
