SUBDIRS = COSMIC

bin_PROGRAMS = dmucs gethost loadavg monitor remhost

dmucs_SOURCES = dmucs_resolve.cc dmucs_db.cc dmucs_host.cc \
	dmucs_hosts_file.cc dmucs_msg.cc dmucs_host_state.cc main.cc

LDADD = COSMIC/libsimpleskts.la

gethost_SOURCES = dmucs_resolve.cc gethost.cc

loadavg_SOURCES = loadavg.cc

monitor_SOURCES = monitor.cc

remhost_SOURCES = remhost.cc

#
# Make -DPKGDATADIR=<pkgdatadir> be passed on each compile.
#
AM_CPPFLAGS = -DPKGDATADIR=\"${pkgdatadir}\"

