/* -*- c-file-style: "java"; indent-tabs-mode: nil; tab-width: 4; fill-column: 78 -*-
 *
 * distcc -- A simple distributed compiler system
 *
 * Copyright 2005-2009 Apple Inc.
 * Copyright 2009 Google Inc.
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

/* Author: Mark Mentovai */

#include <config.h>

#ifdef XCODE_INTEGRATION

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "distcc.h"
#include "dopt.h"
#include "trace.h"
#include "xci.h"

#if defined(HAVE_AVAHI) || defined(HAVE_DNSSD)

static const char *dcc_xci_zeroconf_service_name(void) {
    const char *service = getenv("DISTCCD_SERVICE_NAME");

    if (!service)
        service = "_xcodedistcc._tcp";

    return service;
}

#endif

#if defined(HAVE_AVAHI)

#include "zeroconf.h"

void *dcc_xci_zeroconf_register(void) {
    int ncpus;
    const char *service_name = dcc_xci_zeroconf_service_name();

    if (dcc_ncpus(&ncpus))
        ncpus = 1;

    return dcc_zeroconf_register_extended(0, service_name, arg_port, ncpus);
}

#elif defined(HAVE_DNSSD)

#include <dns_sd.h>
#include <pthread.h>

typedef struct {
    pthread_t thread;
    int thread_live;
    DNSServiceRef zc;
} context;

static void dcc_xci_zeroconf_reply(const DNSServiceRef zeroconf,
                                   const DNSServiceFlags flags,
                                   const DNSServiceErrorType error,
                                   const char *name,
                                   const char *reg_type,
                                   const char *domain,
                                   void *user_data) {
    context *ctx = user_data;

    /* Avoid unused parameter warnings. */
    (void)zeroconf;
    (void)flags;

    if (error != kDNSServiceErr_NoError) {
        rs_log_error("received error %d", error);
        ctx->thread_live = 0;
        pthread_exit(NULL);
    } else {
        rs_log_info("registered as \"%s.%s%s\"", name, reg_type, domain);
    }
}

static void *dcc_xci_zeroconf_main(void *param) {
    context *ctx = param;
    const char *service;
    DNSServiceErrorType rv;

    service = dcc_xci_zeroconf_service_name();

    if ((rv = DNSServiceRegister(&ctx->zc, 0, 0, NULL, service,
                                 NULL, NULL, htons(arg_port), 0, NULL,
                                 dcc_xci_zeroconf_reply, ctx)) !=
        kDNSServiceErr_NoError) {
        rs_log_error("DNSServiceRegister() failed: error %d", rv);
        return NULL;
    }

    while ((rv = DNSServiceProcessResult(ctx->zc) == kDNSServiceErr_NoError)) {
    }

    rs_log_error("DNSServiceProcessResult() failed: error %d", rv);

    ctx->thread_live = 0;
    return NULL;
}

void *dcc_xci_zeroconf_register(void) {
    int rv;
    context *ctx = NULL;

    if (!(ctx = calloc(1, sizeof(context)))) {
        rs_log_error("calloc() failed: %s", strerror(errno));
        goto out_error;
    }

    if ((rv = pthread_create(&ctx->thread, NULL,
                             dcc_xci_zeroconf_main, ctx))) {
        rs_log_error("pthread_create() failed: %s", strerror(rv));
    }

    ctx->thread_live = 1;

    return ctx;

  out_error:
    if (ctx)
        free(ctx);

    return NULL;
}

#endif /* HAVE_DNSSD */

#endif /* XCODE_INTEGRATION */
