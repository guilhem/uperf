/*
 * Copyright (C) 2008 Sun Microsystems
 *
 * This file is part of uperf.
 *
 * uperf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 *
 * uperf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uperf.  If not, see http://www.gnu.org/licenses/.
 */

/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved. Use is
 * subject to license terms.
 */
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <strings.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "logging.h"
#include "uperf.h"
#include "flowops.h"
#include "workorder.h"
#include "protocol.h"
#include "generic.h"

#define	USE_POLL_ACCEPT	1
#define	LISTENQ		10240	/* 2nd argument to listen() */
#define	TCP_TIMEOUT	1200000	/* Argument to poll */
#define	SOCK_PORT(sin)	((sin).sin_port)

/* returns the port number */
static int
protocol_listen(protocol_t *p, void *options)
{
	char msg[128];

	/* SO_RCVBUF must be set before bind */

	if (generic_socket(p, IPPROTO_TCP) != UPERF_SUCCESS) {
		(void) snprintf(msg, 128, "%s: Cannot create socket", "tcp");
		uperf_log_msg(UPERF_LOG_ERROR, errno, msg);
		return (-1);
	}
	set_tcp_options(p->fd, (flowop_options_t *)options);

	if (generic_listen(p, IPPROTO_TCP) != UPERF_FAILURE) {
		return (p->port);
	}
	return (-1);
}

static int
protocol_connect(protocol_t *p, void *options)
{
	flowop_options_t *flowop_options = (flowop_options_t *)options;
	int status;

	status = generic_connect(p, IPPROTO_TCP);
	if (status == UPERF_SUCCESS)
		set_tcp_options(p->fd, flowop_options);

	return (status);
}

static protocol_t *tcp_accept(protocol_t *p, void *options);

static protocol_t *
protocol_tcp_new()
{
	protocol_t *newp;

	if ((newp = calloc(sizeof (protocol_t), 1)) == NULL) {
		perror("calloc");
		return (NULL);
	}
	newp->connect = protocol_connect;
	newp->disconnect = generic_disconnect;
	newp->listen = protocol_listen;
	newp->accept = tcp_accept;
	newp->read = generic_read;
	newp->write = generic_write;
	newp->send = generic_send;
	newp->recv = generic_recv;
	newp->wait = generic_undefined;
	newp->type = PROTOCOL_TCP;
	(void) strlcpy(newp->host, "Init", MAXHOSTNAME);
	newp->fd = -1;
	newp->port = -1;
	newp->next = NULL;

	return (newp);
}

static protocol_t *
tcp_accept(protocol_t *p, void *options)
{
	protocol_t *newp;

	if ((newp = protocol_tcp_new()) == NULL) {
		return (NULL);
	}
	if (generic_accept(p, newp, options) != 0)
		return (NULL);
	if (options) {
		set_tcp_options(newp->fd, options);
	}

	return (newp);
}

protocol_t *
protocol_tcp_create(char *host, int port)
{
	protocol_t *p = protocol_tcp_new();

	if (!p)
		return (NULL);
	(void) strlcpy(p->host, host, MAXHOSTNAME);
	if (strlen(host) == 0)
		(void) strlcpy(p->host, "localhost", MAXHOSTNAME);
	p->port = port;

	return (p);
}
