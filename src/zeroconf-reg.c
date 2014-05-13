/* -*- c-file-style: "java"; indent-tabs-mode: nil; tab-width: 4; fill-column: 78 -*-
 *
 * Copyright (C) 2007 Lennart Poettering
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <config.h>

#include <assert.h>
#include <stdio.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <avahi-common/thread-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/alternative.h>
#include <avahi-common/malloc.h>
#include <avahi-client/publish.h>

#include "distcc.h"
#include "zeroconf.h"
#include "trace.h"
#include "exitcode.h"

struct context {
    int advertise_capabilities;
    char *name;
    char *service_type;
    AvahiThreadedPoll *threaded_poll;
    AvahiClient *client;
    AvahiEntryGroup *group;
    uint16_t port;
    int n_cpus;
};

static void publish_reply(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata);

static void register_stuff(struct context *ctx) {
#ifndef ENABLE_RFC2553
    static const AvahiProtocol dcc_proto = AVAHI_PROTO_INET;
#else
    static const AvahiProtocol dcc_proto = AVAHI_PROTO_UNSPEC;
#endif

    if (!ctx->group) {

        if (!(ctx->group = avahi_entry_group_new(ctx->client, publish_reply, ctx))) {
            rs_log_crit("Failed to create entry group: %s\n", avahi_strerror(avahi_client_errno(ctx->client)));
            goto fail;
        }

    }

    if (avahi_entry_group_is_empty(ctx->group)) {
        char cpus[32], machine[64] = "cc_machine=", version[64] = "cc_version=", *m, *v;

        if (ctx->advertise_capabilities) {
            snprintf(cpus, sizeof(cpus), "cpus=%i", ctx->n_cpus);
            v = dcc_get_gcc_version(version+11, sizeof(version)-11);
            m = dcc_get_gcc_machine(machine+11, sizeof(machine)-11);
        }

        /* Register our service */

        if (avahi_entry_group_add_service(
                    ctx->group,
                    AVAHI_IF_UNSPEC,
                    dcc_proto,
                    0,
                    ctx->name,
                    ctx->service_type,
                    NULL,
                    NULL,
                    ctx->port,
                    !ctx->advertise_capabilities ? NULL : "txtvers=1",
                    cpus,
                    "distcc="PACKAGE_VERSION,
                    "gnuhost="GNU_HOST,
                    v ? version : NULL,
                    m ? machine : NULL,
                    NULL) < 0) {
            rs_log_crit("Failed to add service: %s\n", avahi_strerror(avahi_client_errno(ctx->client)));
            goto fail;
        }

        if (ctx->advertise_capabilities) {
            if (v && m) {
                char stype[128];

                dcc_make_dnssd_subtype(stype, sizeof(stype), v, m);

                if (avahi_entry_group_add_service_subtype(
                            ctx->group,
                            AVAHI_IF_UNSPEC,
                            AVAHI_PROTO_UNSPEC,
                            0,
                            ctx->name,
                            ctx->service_type,
                            NULL,
                            stype) < 0) {
                    rs_log_crit("Failed to add service: %s", avahi_strerror(avahi_client_errno(ctx->client)));
                    goto fail;
                }
            } else
                rs_log_warning("Failed to determine CC version, not registering DNS-SD service subtype!");
        }

        if (avahi_entry_group_commit(ctx->group) < 0) {
            rs_log_crit("Failed to commit entry group: %s\n", avahi_strerror(avahi_client_errno(ctx->client)));
            goto fail;
        }

    }

    return;

fail:
    avahi_threaded_poll_quit(ctx->threaded_poll);
}

/* Called when publishing of service data completes */
static void publish_reply(AvahiEntryGroup *UNUSED(g), AvahiEntryGroupState state, void *userdata) {
    struct context *ctx = userdata;

    switch (state) {

        case AVAHI_ENTRY_GROUP_COLLISION: {
            char *n;

            /* Pick a new name for our service */

            n = avahi_alternative_service_name(ctx->name);
            assert(n);

            avahi_free(ctx->name);
            ctx->name = n;

            register_stuff(ctx);
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE:
            rs_log_crit("Failed to register service: %s\n", avahi_strerror(avahi_client_errno(ctx->client)));
            avahi_threaded_poll_quit(ctx->threaded_poll);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
        case AVAHI_ENTRY_GROUP_ESTABLISHED:
            ;
    }
}

static void client_callback(AvahiClient *client, AvahiClientState state, void *userdata) {
    struct context *ctx = userdata;

    ctx->client = client;

    switch (state) {

        case AVAHI_CLIENT_S_RUNNING:

            register_stuff(ctx);
            break;

        case AVAHI_CLIENT_S_COLLISION:
        case AVAHI_CLIENT_S_REGISTERING:

            if (ctx->group)
                avahi_entry_group_reset(ctx->group);

            break;

        case AVAHI_CLIENT_FAILURE:

            if (avahi_client_errno(client) == AVAHI_ERR_DISCONNECTED) {
                int error;

                avahi_client_free(ctx->client);
                ctx->client = NULL;
                ctx->group = NULL;

                /* Reconnect to the server */

                if (!(ctx->client = avahi_client_new(
                              avahi_threaded_poll_get(ctx->threaded_poll),
                              AVAHI_CLIENT_NO_FAIL,
                              client_callback,
                              ctx,
                              &error))) {

                    rs_log_crit("Failed to contact server: %s\n", avahi_strerror(error));
                    avahi_threaded_poll_quit(ctx->threaded_poll);
                }

            } else {
                rs_log_crit("Client failure: %s\n", avahi_strerror(avahi_client_errno(client)));
                avahi_threaded_poll_quit(ctx->threaded_poll);
            }

            break;

        case AVAHI_CLIENT_CONNECTING:
            ;
    }
}

/* Register a distcc service in DNS-SD/mDNS.  If advertise_capabilities is
 * true, the registration will contain information about the distcc server's
 * capabilities.  If service_type is NULL, the default service type will be
 * used.  advertise_capabilities should be true when using the default
 * service type. */
void* dcc_zeroconf_register_extended(int advertise_capabilities,
                                     const char *service_type,
                                     uint16_t port,
                                     int n_cpus) {
    struct context *ctx = NULL;
    char hostname[_POSIX_HOST_NAME_MAX + 1];
    const AvahiPoll *threaded_poll;
    int len, error;

    ctx = calloc(1, sizeof(struct context));
    if (!ctx) {
        rs_log_crit("calloc() failed for ctx: %s", strerror(errno));
        goto fail;
    }

    ctx->advertise_capabilities = advertise_capabilities;
    ctx->port = port;
    ctx->n_cpus = n_cpus;

    /* Prepare service type.  Use the supplied value, or the default if
     * NULL was supplied. */
    if (service_type)
        ctx->service_type = strdup(service_type);
    else
        ctx->service_type = strdup(DCC_DNS_SERVICE_TYPE);
    if (!ctx->service_type) {
        rs_log_crit("strdup() failed for ctx->service_type: %s",
                    strerror(errno));
        goto fail;
    }

    /* Prepare service name.  This is just the chosen service type with
     * '@' and the hostname appended.  If this collides with anything else,
     * avahi_alternative_service_name will choose a replacement name. */
    if (gethostname(hostname, sizeof(hostname))) {
        rs_log_crit("gethostname() failed: %s", strerror(errno));
        goto fail;
    }

    /* Leave room for the '@' and trailing NUL. */
    len = strlen(ctx->service_type) + strlen(hostname) + 2;
    if (!(ctx->name = avahi_malloc(len))) {
        rs_log_crit("avahi_malloc() failed for ctx->name");
        goto fail;
    }

    snprintf(ctx->name, len, "%s@%s", ctx->service_type, hostname);

    /* Create the Avahi client. */
    if (!(ctx->threaded_poll = avahi_threaded_poll_new())) {
        rs_log_crit("Failed to create event loop object.");
        goto fail;
    }

    threaded_poll = avahi_threaded_poll_get(ctx->threaded_poll);

    if (!(ctx->client = avahi_client_new(threaded_poll, AVAHI_CLIENT_NO_FAIL,
                                         client_callback, ctx, &error))) {
        rs_log_crit("Failed to create client object: %s",
                    avahi_strerror(error));
        goto fail;
    }

    /* Create the mDNS event handler */
    if (avahi_threaded_poll_start(ctx->threaded_poll) < 0) {
        rs_log_crit("Failed to create thread.");
        goto fail;
    }

    return ctx;

fail:

    if (ctx)
        dcc_zeroconf_unregister(ctx);

    return NULL;
}

/* Register a distcc service in DNS-SD/mDNS with the given port and number of
 * CPUs.  Advertise capabilities and use the default service name. */
void* dcc_zeroconf_register(uint16_t port, int n_cpus) {
    return dcc_zeroconf_register_extended(1, NULL, port, n_cpus);
}

/* Unregister this server from DNS-SD/mDNS */
int dcc_zeroconf_unregister(void *u) {
    struct context *ctx = u;

    if (ctx->threaded_poll)
        avahi_threaded_poll_stop(ctx->threaded_poll);

    if (ctx->client)
        avahi_client_free(ctx->client);

    if (ctx->threaded_poll)
        avahi_threaded_poll_free(ctx->threaded_poll);

    if (ctx->name)
        avahi_free(ctx->name);

    if (ctx->service_type)
        free(ctx->service_type);

    free(ctx);

    return 0;
}
