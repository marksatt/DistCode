/* -*- c-file-style: "java"; indent-tabs-mode: nil; tab-width: 4; fill-column: 78 -*-
 *
 * distcc -- A simple distributed compiler system
 *
 * Copyright (C) 2002, 2003, 2004 by Martin Pool
 * Copyright 2007 Google Inc.
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

/*
 * Send a compilation request to a remote server.
 */


#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>

#include "distcc.h"
#include "trace.h"
#include "rpc.h"
#include "exitcode.h"
#include "util.h"
#include "clinet.h"
#include "hosts.h"
#include "exec.h"
#include "lock.h"
#include "compile.h"
#include "bulk.h"
#ifdef HAVE_GSSAPI
#include "auth.h"

/* Global security context in case confidentiality/integrity */
/* type services are needed in the future. */
extern gss_ctx_id_t distcc_ctx_handle;
#endif

/*
 * TODO: If cpp finishes early and fails then perhaps break out of
 * trying to connect.
 *
 * TODO: If we abort, perhaps kill the SSH child rather than closing
 * the socket.  Closing while a lot of stuff has been written through
 * might make us block until the other side reads all the data.
 */

/**
 * Open a connection using either a TCP socket or SSH.  Return input
 * and output file descriptors (which may or may not be different.)
 **/
static int dcc_remote_connect(struct dcc_hostdef *host,
                              int *to_net_fd,
                              int *from_net_fd,
                              pid_t *ssh_pid)
{
    int ret;

    if (host->mode == DCC_MODE_TCP) {
        *ssh_pid = 0;
        if ((ret = dcc_connect_by_name(host->hostname, host->port,
                                       to_net_fd)) != 0)
            return ret;
        *from_net_fd = *to_net_fd;
        return 0;
    } else if (host->mode == DCC_MODE_SSH) {
        if ((ret = dcc_ssh_connect(NULL, host->user, host->hostname,
                                   host->ssh_command,
                                   from_net_fd, to_net_fd,
                                   ssh_pid)))
            return ret;
        return 0;
    } else {
        rs_log_crit("impossible host mode");
        return EXIT_DISTCC_FAILED;
    }
}


static int dcc_wait_for_cpp(pid_t cpp_pid,
                            int *status,
                            const char *input_fname)
{
    int ret;

    if (cpp_pid) {
        dcc_note_state(DCC_PHASE_CPP, NULL, NULL, DCC_LOCAL);
        /* Wait for cpp to finish (if not already done), check the
         * result, then send the .i file */

        if ((ret = dcc_collect_child("cpp", cpp_pid, status, timeout_null_fd)))
            return ret;

        /* Although cpp failed, there is no need to try running the command
         * locally, because we'd presumably get the same result.  Therefore
         * critique the command and log a message and return an indication
         * that compilation is complete. */
        if (dcc_critique_status(*status, "cpp", input_fname, dcc_hostdef_local, 0))
            return 0;
    }
    return 0;
}

static const char *dcc_map_optx_language_for_cpp(const char *language, int use_new_names) {
    /* These are the only types we allowed in arg.c */

    static const char* keys[] = {
    	"c",
    	"c++",
    	"objective-c",
    	"objective-c++",
    	"cpp-output",
    	"c++-cpp-output",
    	"objc-cpp-output",
    	"objc++-cpp-output",
    	"objective-c-cpp-output",
    	"objective-c++-cpp-output",
    	NULL};
    
    static const char* old_transforms[] = {
    	"cpp-output",
    	"c++-cpp-output",
    	"objc-cpp-output",
    	"objc++-cpp-output",
    	"cpp-output",
    	"c++-cpp-output",
    	"objc-cpp-output",
    	"objc++-cpp-output",
    	"objc-cpp-output",
    	"objc++-cpp-output",
    	NULL};

    static const char* new_transforms[] = {
    	"cpp-output",
    	"c++-cpp-output",
    	"objective-c-cpp-output",
    	"objective-c++-cpp-output",
    	"cpp-output",
    	"c++-cpp-output",
    	"objective-c-cpp-output",
    	"objective-c++-cpp-output",
    	"objective-c-cpp-output",
    	"objective-c++-cpp-output",
    	NULL};

    char** transform = (char**)(use_new_names ? new_transforms : old_transforms);
    char* cur_key = (char*)keys[0];
    unsigned int i = 0;
    do
    {
    	if(!strcmp(language, cur_key)) {
			rs_trace("mapped cpp language (new style: %i) %s to %s", use_new_names, language, transform[i]);
    		return transform[i];
    	}
    }
    while((cur_key = (char*)keys[++i]) != NULL);

	return NULL;
}

/* Send a request across to the already-open server.
 *
 * CPP_PID is the PID of the preprocessor running in the background.
 * We wait for it to complete before reading its output.
 */
static int
dcc_send_header(int net_fd,
                char **argv,
                struct dcc_hostdef *host)
{
    int ret, i;
    char **new_argv = NULL;
    const char *new_lang;

    /* Make a copy so we can mask the Xcode developer dir (no op without
     * that support enabled */
    if ((ret = dcc_copy_argv(argv, &new_argv, 0)))
        return ret;
    if ((ret = dcc_xci_mask_developer_dir_in_argv(new_argv)))
        return ret;
  
    tcp_cork_sock(net_fd, 1);

    if ((ret = dcc_x_req_header(net_fd, host->protover)))
        return ret;
    if (host->cpp_where == DCC_CPP_ON_SERVER) {
        if ((ret = dcc_x_cwd(net_fd)))
            goto out_error;
    } else if (host->cpp_where == DCC_CPP_ON_CLIENT) {
        /* Fix up the -x argument in new_argv, if any, to account for
         * preprocessing having been done. */
        int using_clang = 0;
        for (i = 0; new_argv[i]; i++) {
        	if (strstr(new_argv[i], "/usr/bin/clang")) // this is a hack to enable using new-style objective-c transformations for clang
        		using_clang = 1;
            if (!strcmp(new_argv[i], "-x") && new_argv[i+1]) {
                new_lang = dcc_map_optx_language_for_cpp(new_argv[i+1], using_clang);
                if (!new_lang) {
                    rs_log_error("got unsupported -x language: %s", new_argv[i+1]);
                    ret = EXIT_DISTCC_FAILED;
                    goto out_error;
                }
                new_argv[i+1] = strdup(new_lang);
                if (!new_argv[i+1]) {
                    rs_log_error("failed to duplicate string");
                    ret = EXIT_OUT_OF_MEMORY;
                    goto out_error;
                }
                break;
            }
        }
    }
    if ((ret = dcc_x_argv(net_fd, "ARGC", "ARGV", new_argv))) {
        goto out_error;
    }

    dcc_free_argv(new_argv);
    return 0;
  
  out_error:
    if (new_argv)
        dcc_free_argv(new_argv);
    return ret;
}


/**
 * Pass a compilation across the network.
 *
 * When this function is called, the preprocessor has already been
 * started in the background.  It may have already completed, or it
 * may still be running.  The goal is that preprocessing will overlap
 * with setting up the network connection, which may take some time
 * but little CPU.
 *
 * If this function fails, compilation will be retried on the local
 * machine.
 *
 * @param argv Compiler command to run.
 *
 * @param cpp_fname Filename of preprocessed source.  May not be complete yet,
 * depending on @p cpp_pid.
 *
 * @param files If we are doing preprocessing on the server, the names of
 * all the files needed; otherwise, NULL.
 *
 * @param output_fname File that the object code should be delivered to.
 *
 * @param cpp_pid If nonzero, the pid of the preprocessor.  Must be
 * allowed to complete before we send the input file.
 *
 * @param local_cpu_lock_fd If != -1, file descriptor for the lock file.
 * Should be != -1 iff (host->cpp_where != DCC_CPP_ON_SERVER).
 * If != -1, the lock must be held on entry to this function,
 * and THIS FUNCTION WILL RELEASE THE LOCK.
 *
 * @param host Definition of host to send this job to.
 *
 * @param status on return contains the wait-status of the remote
 * compiler.
 *
 * Returns 0 on success, otherwise error.  Returning nonzero does not
 * necessarily imply the remote compiler itself succeeded, only that
 * there were no communications problems.
 *
 * TODO: consider refactoring this (perhaps as two separate subroutines?)
 * to avoid the need for releasing the lock as a side effect of this call.
 */
int dcc_compile_remote(char **argv,
                       char *input_fname,
                       char *cpp_fname,
                       char **files,
                       char *output_fname,
                       char *deps_fname,
                       char *server_stderr_fname,
                       pid_t cpp_pid,
                       int local_cpu_lock_fd,
                       struct dcc_hostdef *host,
                       int *status)
{
    int to_net_fd = -1, from_net_fd = -1;
    int ret;
    pid_t ssh_pid = 0;
    int ssh_status;
    off_t doti_size;
    struct timeval before, after;
    unsigned int n_files;

    if (gettimeofday(&before, NULL))
        rs_log_warning("gettimeofday failed");

    dcc_note_execution(host, argv);
    dcc_note_state(DCC_PHASE_CONNECT, input_fname, host->hostname, DCC_REMOTE);

    /* For ssh support, we need to allow for separate fds writing to and
     * reading from the network, because our connection to the ssh client may
     * be over pipes, which are one-way connections. */

    *status = 0;
    if ((ret = dcc_remote_connect(host, &to_net_fd, &from_net_fd, &ssh_pid)))
        goto out;

#ifdef HAVE_GSSAPI
    /* Perform requested security. */
    if(host->authenticate) {
        rs_log_info("Performing authentication.");

        if ((ret = dcc_gssapi_perform_requested_security(to_net_fd, from_net_fd)) != 0) {
            rs_log_crit("Failed to perform authentication.");
            goto out;
        }

        /* Context deleted here as we no longer need it.  However, we have it available */
        /* in case we want to use confidentiality/integrity type services in the future. */
        dcc_gssapi_delete_ctx(&distcc_ctx_handle);
    } else {
        rs_log_info("No authentication requested.");
    }
#endif

    dcc_note_state(DCC_PHASE_SEND, NULL, NULL, DCC_REMOTE);

    if (host->cpp_where == DCC_CPP_ON_SERVER) {
        if ((ret = dcc_send_header(to_net_fd, argv, host))) {
          goto out;
        }

        n_files = dcc_argv_len(files);
        if ((ret = dcc_x_many_files(to_net_fd, n_files, files))) {
            goto out;
        }
    } else {
        /* This waits for cpp and puts its status in *status.  If cpp failed,
         * then the connection will have been dropped and we need not bother
         * trying to get any response from the server. */

        if ((ret = dcc_send_header(to_net_fd, argv, host)))
            goto out;

        if ((ret = dcc_wait_for_cpp(cpp_pid, status, input_fname)))
            goto out;

        /* We are done with local preprocessing.  Unlock to allow someone
         * else to start preprocessing. */
        if (local_cpu_lock_fd != -1) {
            dcc_unlock(local_cpu_lock_fd);
            local_cpu_lock_fd = -1;
        }

        if (*status != 0)
            goto out;

        if ((ret = dcc_x_file(to_net_fd, cpp_fname, "DOTI", host->compr,
                              &doti_size)))
            goto out;
    }

    rs_trace("client finished sending request to server");
    tcp_cork_sock(to_net_fd, 0);
    /* but it might not have been read in by the server yet; there's
     * 100kB or more of buffers in the two kernels. */

    /* OK, now all of the source has at least made it into the
     * client's TCP transmission queue, sometime soon the server will
     * start compiling it.  */
    dcc_note_state(DCC_PHASE_COMPILE, NULL, host->hostname, DCC_REMOTE);

    /* If cpp failed, just abandon the connection, without trying to
     * receive results. */
    if (ret == 0 && *status == 0) {
        ret = dcc_retrieve_results(from_net_fd, status, output_fname,
                                   deps_fname, server_stderr_fname, host);
    }

    if (gettimeofday(&after, NULL)) {
        rs_log_warning("gettimeofday failed");
    } else if (host->cpp_where == DCC_CPP_ON_CLIENT) {
        double secs, rate;

        dcc_calc_rate(doti_size, &before, &after, &secs, &rate);
        rs_log(RS_LOG_INFO|RS_LOG_NONAME,
               "%lu bytes from %s compiled on %s in %.4fs, rate %.0fkB/s",
               (unsigned long) doti_size, input_fname, host->hostname,
               secs, rate);
    }

  out:
    if (local_cpu_lock_fd != -1) {
        dcc_unlock(local_cpu_lock_fd);
        local_cpu_lock_fd = -1; /* Not really needed; just for consistency. */
    }

    /* Close socket so that the server can terminate, rather than
     * making it wait until we've finished our work. */
    if (to_net_fd != from_net_fd) {
        if (to_net_fd != -1)
            dcc_close(to_net_fd);
    }
    if (from_net_fd != -1)
        dcc_close(from_net_fd);

    /* Collect the SSH child.  Strictly this is unnecessary; it might slow the
     * client down a little when things could otherwise be proceeding in the
     * background.  But it helps make sure that we don't assume we succeeded
     * when something possibly went wrong, and it allows us to account for the
     * cost of the ssh child. */
    if (ssh_pid) {
        dcc_collect_child("ssh", ssh_pid, &ssh_status, timeout_null_fd); /* ignore failure */
    }

    return ret;
}


#ifdef XCODE_INTEGRATION
/**
 * Print information about a host to stdout.
 *
 * This function connects to host, sends an HINF query, and prints the result.
 * Xcode uses the output to determine various characteristics about the host,
 * such as the hardware and operating system in use, the compilers available,
 * and the number of CPUs available.  Only distccd servers built with Xcode
 * integration support HINF.
 *
 * @param host The host to query.
 *
 * Returns 0 on success, otherwise error.  Returning nonzero does not
 */
int dcc_show_host_info(char *host)
{
    int to_net_fd = -1, from_net_fd = -1;
    int ret;
    pid_t ssh_pid = 0;
    int ssh_status;
    char *info;
    struct dcc_hostdef *hostdef;
    int n_hosts = 0;
    const char *argv[] = { "--host-info", NULL };

    if ((ret = dcc_parse_hosts(host, "command line",
                               &hostdef, &n_hosts, NULL))) {
        rs_log_error("bad host argument: %s", host);
        return ret;
    }

    if (n_hosts != 1) {
        rs_log_error("too many hosts for --host-info %s", host);
        return EXIT_BAD_ARGUMENTS;
    }

    if ((ret = dcc_remote_connect(hostdef, &to_net_fd,
                                  &from_net_fd, &ssh_pid))) {
        rs_log_error("couldn't connect to %s", host);
        printf("ERROR=%d\n", errno);
        return ret;
    }

    if ((ret = dcc_send_header(to_net_fd, (char**)argv, hostdef)) != 0) {
        rs_log_error("failed to send request");
        printf("ERROR=%d\n", errno);
        goto out;
    }

    /* Pop the cork placed by dcc_send_header.  Normally the cork gets
     * popped in dcc_compile_remote, but that function won't be called
     * when running --host-info. */
    tcp_cork_sock(to_net_fd, 0);

    if ((ret = dcc_r_result_header(from_net_fd, hostdef->protover)))
        goto out; /* dcc_r_result_header logs its own error on failure */

    if ((ret = dcc_r_token_string(from_net_fd, "HINF", &info))) {
        rs_log_error("failed to read result");
        printf("ERROR=%d\n", errno);
    }

  out:
    /* Close socket so that the server can terminate, rather than
     * making it wait until we've finished our work. */
    if (to_net_fd != from_net_fd) {
        if (to_net_fd != -1)
            dcc_close(to_net_fd);
    }
    if (from_net_fd != -1)
        dcc_close(from_net_fd);

    /* Collect the SSH child.  Strictly this is unnecessary; it might slow the
     * client down a little when things could otherwise be proceeding in the
     * background.  But it helps make sure that we don't assume we succeeded
     * when something possibly went wrong, and it allows us to account for the
     * cost of the ssh child. */
    if (ssh_pid) {
        dcc_collect_child("ssh", ssh_pid, &ssh_status, timeout_null_fd); /* ignore failure */
    }

    if (!ret)
        printf("%s\n", info);

    return ret;
}
#endif /* XCODE_INTEGRATION */
