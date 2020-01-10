###############################################################################
#
# Copyright 2015 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License version 2.
#
###############################################################################

Name: ricci
Version: 0.16.2
Release: 81%{?dist}
License: GPLv2
URL: http://sources.redhat.com/cluster/conga/
Group: System Environment/Base
Summary: Remote Cluster and Storage Management System
Source0: http://people.redhat.com/rmccabe/conga/fedora/src/ricci-0.16.2.tar.gz
Patch0: bz580575.patch
Patch1: bz585126.patch
Patch2: bz587526.patch
Patch3: bz553384.patch
Patch4: bz610042.patch
Patch5: bz612318.patch
Patch6: bz617090.patch
Patch7: bz644047.patch
Patch8: bz602399.patch
Patch9: bz614647.patch
Patch10: bz614647-2.patch
Patch11: bz652837.patch
Patch12: bz614647-3.patch
Patch13: bz682317.patch
Patch14: bz681646.patch
Patch15: bz682868.patch
Patch16: bz682868-1.patch
Patch17: bz614647-4.patch
Patch18: bz614647-5.patch
Patch19: bz614647-6.patch
Patch20: bz697493.patch
Patch21: bz718230.patch
Patch22: bz725722.patch
Patch23: bz696901.patch
Patch24: bz696901-2.patch
Patch25: bz725722-2.patch
Patch26: bz736795-1.patch
Patch27: bz736795-2.patch
Patch28: bz736795-3.patch
Patch29: bz736795-4.patch
Patch30: bz797267.patch
Patch31: bz738008.patch
Patch32: bz773383.patch
Patch33: bz729011.patch
Patch34: bz738797.patch
Patch35: bz724014-plus-more.patch
Patch36: bz773383-1.patch
Patch37: bz738797-1.patch
Patch38: bz797292.patch
Patch39: bz758823.patch
Patch40: bz797292-1.patch
Patch41: bz797292-2.patch
Patch42: bz773383-2.patch
Patch43: bz726772.patch
Patch44: bz811702-841288-839039.patch
Patch45: bz818335.patch
Patch46: bz815752.patch
Patch47: bz866894.patch
Patch48: bz867019.patch
Patch49: bz878108.patch
Patch50: bz877381.patch
Patch51: bz842939.patch
Patch52: bz815752-2.patch 
Patch53: bz878108-2.patch
Patch54: bz878108-3.patch
Patch55: bz853890.patch
Patch56: bz877863.patch
Patch57: bz893574.patch
Patch58: bz918555.patch
Patch59: bz883585.patch
Patch60: bz893574-2.patch
Patch61: bz1009098.patch
Patch62: bz893574-3.patch
Patch63: bz1025053.patch
Patch64: bz1090642.patch
Patch65: bz917809.patch
Patch66: bz1062402.patch
Patch67: bz1024962.patch
Patch68: bz996196.patch
Patch69: bz1075716.patch
Patch70: bz1076713.patch
Patch71: bz1044122.patch
Patch72: bz1055424.patch
Patch73: bz1156157.patch
Patch74: bz1126876.patch
Patch75: bz1125954.patch
Patch76: bz1125957.patch
Patch77: bz1079032.patch
Patch78: bz1126872.patch
Patch79: bz1084991.patch
Patch80: bz1166589.patch
Patch81: bz1156157-2.patch
Patch82: bz1187745.patch
Patch83: bz1210679.patch
Patch84: bz1079032-2.patch
Patch85: bz1156157-3.patch

Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: libxml2-devel python-devel libcap-devel
BuildRequires: openssl-devel dbus-devel pkgconfig file-devel nss-devel
BuildRequires: cyrus-sasl-devel >= 2.1

ExclusiveArch: i686 x86_64

Requires: oddjob dbus openssl cyrus-sasl >= 2.1 file nss-tools modcluster
# shadow-utils:groupadd,useradd; util-linux-ng:/sbin/nologin
Requires: shadow-utils util-linux-ng
# modstorage
Requires: parted

Requires(post): chkconfig initscripts
Requires(preun): chkconfig initscripts
Requires(postun): initscripts

%prep
%setup -q
%patch0 -p2
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1 -b .bz610042
%patch5 -p1 -b .bz612318
%patch6 -p1 -b .bz617090
%patch7 -p1 -b .bz644047
%patch8 -p2 -b .bz602399
%patch9 -p1 -b .bz614647
%patch10 -p1 -b .bz614647-2
%patch11 -p2 -b .bz652837
%patch12 -p2 -b .bz614647-3
%patch13 -p2 -b .bz682317
%patch14 -p2 -b .bz681646
%patch15 -p2 -b .bz682868
%patch16 -p2 -b .bz682868-1
%patch17 -p2 -b .bz614647-4
%patch18 -p2 -b .bz614647-5
%patch19 -p2 -b .bz614647-6
%patch20 -p2 -b .bz697493
%patch21 -p2 -b .bz718230
%patch22 -p2 -b .bz725722
%patch23 -p2 -b .bz696901
%patch24 -p2 -b .bz696901-2
%patch25 -p2 -b .bz725722-2
%patch26 -p2 -b .bz736795-1
%patch27 -p2 -b .bz736795-2
%patch28 -p2 -b .bz736795-3
%patch29 -p2 -b .bz736795-4
%patch30 -p1 -b .bz797267
%patch31 -p2 -b .bz738008
%patch32 -p2 -b .bz773383
%patch33 -p2 -b .bz729011
%patch34 -p2 -b .bz738797
%patch35 -p2 -b .bz724014-plus-more
%patch36 -p2 -b .bz773383-1
%patch37 -p2 -b .bz738797-1
%patch38 -p2 -b .bz797292
%patch39 -p2 -b .bz758823
%patch40 -p2 -b .bz797292-1
%patch41 -p2 -b .bz797292-2
%patch42 -p2 -b .bz773383-2
%patch43 -p2 -b .bz726772
%patch44 -p2 -b .bz811702-841288-839039.patch
%patch45 -p2 -b .bz818335.patch
%patch46 -p2 -b .bz815752.patch
%patch47 -p2 -b .bz866894
%patch48 -p2 -b .bz867019
%patch49 -p2 -b .bz878108
%patch50 -p2 -b .bz877381
%patch51 -p2 -b .bz842939
%patch52 -p2 -b .bz815752-2
%patch53 -p2 -b .bz878108-2
%patch54 -p2 -b .bz878108-3
%patch55 -p2 -b .bz853890.patch
%patch56 -p2 -b .bz877863.patch
%patch57 -p2 -b .bz893574.patch
%patch58 -p2 -b	.bz918555.patch
%patch59 -p2 -b .bz883585.patch
%patch60 -p2 -b .bz893574-2.patch
%patch61 -p2 -b .bz1009098
%patch62 -p2 -b .bz893574-3
%patch63 -p2 -b .bz1025053
%patch64 -p2 -b .bz1090642
%patch65 -p2 -b .bz917809
%patch66 -p2 -b .bz1062402
%patch67 -p2 -b .bz1024962
%patch68 -p2 -b .bz996196
%patch69 -p2 -b .bz1075716
%patch70 -p2 -b .bz1076713
%patch71 -p1 -b .bz1044122
%patch72 -p1 -b .bz1055424
%patch73 -p2 -b .bz1156157
%patch74 -p2 -b .bz1126876
%patch75 -p2 -b .bz1125954
%patch76 -p2 -b .bz1125957
%patch77 -p2 -b .bz1079032
%patch78 -p2 -b .bz1126872
%patch79 -p2 -b .bz1084991
%patch80 -p2 -b .bz1166589
%patch81 -p2 -b .bz1156157-2
%patch82 -p1 -b .bz1187745
%patch83 -p1 -b .bz1210679
%patch84 -p2 -b .bz1079032-2
%patch85 -p2 -b .bz1156157-3

%build
%configure --arch=%{_arch} --docdir=%{_docdir}
make %{?_smp_mflags} ricci

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install_ricci
cd ccs; make DESTDIR=%{buildroot} install ; cd ..

%clean
rm -rf %{buildroot}

%description
ricci is a cluster and storage management and configuration
daemon. The ricci daemon, dispatches incoming messages to
underlying management modules.

This package contains the listener daemon (dispatcher), as well as
reboot, rpm, storage, service, virtual machine, and log management modules.

%files
%defattr(-,root,root)
%{_bindir}/ccs_sync
%{_mandir}/*/ccs_sync.*
%{_mandir}/*/ricci.*

# ricci
%config(noreplace)	%{_sysconfdir}/pam.d/ricci
%config(noreplace)	%{_sysconfdir}/oddjobd.conf.d/ricci.oddjob.conf
%config(noreplace)	%{_sysconfdir}/dbus-1/system.d/ricci.systembus.conf
			%{_sysconfdir}/rc.d/init.d/ricci
%attr(-,ricci,ricci)	%{_localstatedir}/lib/ricci
			%{_sbindir}/ricci
%attr(-,root,ricci)	%{_libexecdir}/ricci/
			%{_docdir}/ricci-%{version}/
# modrpm
%config(noreplace)	%{_sysconfdir}/oddjobd.conf.d/ricci-modrpm.oddjob.conf
%config(noreplace)	%{_sysconfdir}/dbus-1/system.d/ricci-modrpm.systembus.conf
			%{_libexecdir}/ricci-modrpm

# modstorage
%config(noreplace)	%{_sysconfdir}/oddjobd.conf.d/ricci-modstorage.oddjob.conf
%config(noreplace)	%{_sysconfdir}/dbus-1/system.d/ricci-modstorage.systembus.conf
			%{_libexecdir}/ricci-modstorage

# modservice
%config(noreplace)	%{_sysconfdir}/oddjobd.conf.d/ricci-modservice.oddjob.conf
%config(noreplace)	%{_sysconfdir}/dbus-1/system.d/ricci-modservice.systembus.conf
			%{_libexecdir}/ricci-modservice

# modlog
%config(noreplace)	%{_sysconfdir}/oddjobd.conf.d/ricci-modlog.oddjob.conf
%config(noreplace)	%{_sysconfdir}/dbus-1/system.d/ricci-modlog.systembus.conf
			%{_libexecdir}/ricci-modlog

# modvirt
%config(noreplace)	%{_sysconfdir}/oddjobd.conf.d/ricci-modvirt.oddjob.conf
%config(noreplace)	%{_sysconfdir}/dbus-1/system.d/ricci-modvirt.systembus.conf
			%{_libexecdir}/ricci-modvirt

%pre
/usr/sbin/groupadd -g 140 ricci 2> /dev/null
/usr/sbin/useradd -u 140 -g 140 -d /var/lib/ricci -s /sbin/nologin -r \
	-c "ricci daemon user" ricci 2> /dev/null
exit 0

%post
DBUS_PID=`cat /var/run/messagebus.pid 2>/dev/null`
/bin/kill -s SIGHUP $DBUS_PID >&/dev/null
/sbin/service oddjobd reload >&/dev/null
/sbin/chkconfig --add ricci
exit 0

%preun
if [ "$1" == "0" ]; then
	/sbin/service ricci stop >&/dev/null
	/sbin/chkconfig --del ricci
fi
exit 0

%postun
if [ "$1" == "0" ]; then
	DBUS_PID=`cat /var/run/messagebus.pid 2>/dev/null`
	/bin/kill -s SIGHUP $DBUS_PID >&/dev/null
	/sbin/service oddjobd reload >&/dev/null
fi
if [ "$1" == "1" ]; then
	/sbin/service ricci condrestart >&/dev/null
fi
exit 0

%package -n ccs
Group: System Environment/Base
Summary: Cluster Configuration System
Requires: libxml2

%description -n ccs
The Red Hat Cluster Configuration System

%files -n ccs
%{_sbindir}/ccs
%{_mandir}/man8/ccs.*
%{_datadir}/ccs/cluster.rng
%{_datadir}/ccs/empty_cluster.conf

%changelog
* Wed Apr 21 2015 Chris Feist <cfeist@redhat.com> - 0.16.2-81
- ricci: fixed issue with '-x' option not being recognized
- Resolves: rhbz#1156157

* Tue Apr 21 2015 Chris Feist <cfeist@redhat.com> - 0.16.2-80
- ccs: Don't allow adding duplicate actions for the same resource
- Resolves: rhbz#1079032

* Wed Apr 15 2015 Jan Pokorný <jpokorny@redhat.com> - 0.16.2-79
- ccs: sync cluster.rng schema with latest updates from cluster packages
  (last-minute revamp)
- Resolves: rhbz#1210679

* Thu Mar 05 2015 Jan Pokorný <jpokorny@redhat.com> - 0.16.2-78
- ricci: fix modules cannot return values due to bug in Module::empty_response
         (introduced with rhbz#1044122 fix)
- Resolves: rhbz#1187745

* Tue Mar 03 2015 Chris Feist <cfeist@redhat.com> - 0.16.2-77
- ricci: disable SSLv2/SSLv3 by default, added '-x' option to enable SSLv2/3
- ccs: Added ability to add actions to vm services
       (and other resources/services)
- ccs: Show error when a user is already authenticated and uses a bad
       password
- ccs: Fixed error when piping output of ccs (specifically --getschema)
- ccs: Using --setconf without hostname no longer produces a traceback
- ccs: comments in cluster.conf should no longer produce a traceback when
       listing services
- ccs: ccs should no longer produce anything if cluster.conf doesn't
       exist on remote node
- ccs: ccs no longer triggers activation more than once in one command
- Resolves: rhbz#1156157 rhbz#1126876 rhbz#1125954 rhbz#1125957 rhbz#1079032
  rhbz#1126872 rhbz#1084991 rhbz#1166589

* Tue Jul 15 2014 Jan Pokorny <jpokorny@redhat.com> - 0.16.2-75
- ccs: sync cluster.rng schema with latest updates from cluster packages
  (last-minute revamp)
- Resolves: rhbz#1055424

* Fri Jul 04 2014 Jan Pokorny <jpokorny@redhat.com> - 0.16.2-74
- ccs: sync cluster.rng schema with latest updates from cluster packages
- ricci: enhance possible reboot loop prevention (avoid stale batches)
- Resolves: rhbz#1055424 rhbz#1044122

* Fri Jun 20 2014 Jan Pokorny <jpokorny@redhat.com> - 0.16.2-73
- ricci: fix end-use modules do not handle stdin polling correctly
- ricci: fix missing deployment-time dependencies
- ricci: prevent possible reboot loop
- Resolves: rhbz#1076713 rhbz#1106842 rhbz#1044122

* Mon Jun 16 2014 Chris Feist <cfeist@redhat.com> - 0.16.2-72
- Added '-s' option to disable SSLv2 connections to ricci
- Added ability to configure RICCIOPTS variable in /etc/sysconfig/cluster
  or /etc/sysconfig/ricci
- Resolves: rhbz#1075176

* Fri Jun 06 2014 Chris Feist <cfeist@redhat.com> - 0.16.2-71
- Added --noenable/--nodisable options when starting/stoping cluster
- Only propagate/activate cluster on last node when using --activate
- ccs no longer returns 0 when --startall/--stopall fails on a node
- If not hostname or filename is specified, use 'localhost'
- Don't leave tempfiles around from schema tests
- Resolves: rhbz#1090642 rhbz#917809 rhbz#1062402 rhbz#1024962 rhbz#996196

* Wed Nov 27 2013 Chris Feist <cfeist@redhat.com> - 0.16.2-70
- Added ability to create/remove uidgid entries for corosync in cluster.conf
- Resolves: rhbz#1025053

* Wed Sep 25 2013 Chris Feist <cfeist@redhat.com> - 0.16.2-69
- Added --nounfence to usage/man page and auto nounfence for fence_sanlock
- Resolves: rhbz#893574

* Thu Sep 19 2013 Chris Feist <cfeist@redhat.com> - 0.16.2-67
- Resync cluster.rng
- Resolves: rhbz#1009098

* Wed Aug 14 2013 Chris Feist <cfeist@redhat.com> - 0.16.2-66
- Fix issue with unfencing always being added
- Resolves: rhbz#bz893574

* Fri Aug 09 2013 Fabio M. Di Nitto <fdinitto@redhat.com> - 0.16.2-65
- Don't install ricci/ccs man pages with executable bit set
- Resolves: rhbz#883585

* Thu Aug 08 2013 Chris Feist <cfeist@redhat.com> - 0.16.2-64
- Automatically add unfencing section when adding scsi fence agents
- Prevent ccs_sync from segfaulting when multiple hostnames were provided
- ccs --lsmisc now shows fence_daemon options
- ccs --lsresourceopts has been added as an alias to --lsserviceopts
- Resolves: rhbz#853890 rhbz#877863 rhbz#893574 rhbz#918555


* Tue Dec 11 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-63
- Synced cluster schema with latest cman, resource-agents, fence-agents
  fence-sanlock & fence_virt
- Resolves: rhbz#878108

* Mon Dec 10 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-62
- Synced cluster schema with latest cman/resource-agents/fence-agents
- Resolves: rhbz#878108

* Thu Nov 29 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-61
- Reset thread stack size to value before bz#815752 fix 
- Resolves: rhbz#815752

* Thu Nov 29 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-60
- Fix issue causing conga to be unable to find/install packages due to line
  breaks in yum output
- Resolves: rhbz#842939

* Wed Nov 21 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-59
- Better fix for large cluster.conf and fix for long values in cluster.conf
- Resolves: rhbz#877381

* Mon Nov 19 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-58
- Synced cluster schema with latest cman/resource-agents
- Resolves: rhbz#878108

* Thu Nov 08 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-57
- Updated --addresource to properly add resources when rm section is missing
- Fixed redirect which wouldn't allow creation of keys in a read-only folder
- Resolves: rhbz#867019 rhbz#866894

* Fri Oct 12 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-56
- Fixed traceback when using ccs --stopall
- Fixed return codes from ccs_sync
- Fixed listing altmulticast with ccs --lsmisc
- Ricci & ccs_sync should no longer crash when trying to update a cluster.conf 
  file that is too large 
- Resolves: rhbz#811702 rhbz#841288 rhbz#839039 rhbz#815752 rhbz#818335

* Mon Apr 30 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-55
- Resync cluster schema due to fence-agents & resource-agents changes
- Resolves: rhbz#726772

* Thu Mar 22 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-54
- Fix issue with non english languages with ricci detecting if a passwd
  is set
- Resolves: rhbz#773383

* Wed Feb 29 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-53
- --createcluster now verifies that a cluster.conf doesn't already exist.
- Added support for altname & altmulticast to support RRP
- Resolves: rhbz#797292 rhbz#758823

* Mon Feb 27 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-49
- Reworded ricci log message and usage/man page
- Resolves: rhbz#773383 rhbz#738797

* Mon Feb 27 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-48
- Added ipv6 support (#724014)
- Providing a useful error when passed a bad cluster.conf (#731113)
- --checkconf on a local file now works (#738567)
- Fixed issue with ccs backtrace with bad response (#721113)
- Error gracefully when given a directory with '-f' (#734910)
- Updated usage and man page for --start/--stop (#729168)
- Show virtual machines when using --lsservices (#770637)
- Added --getschema command and use remote schema for validation (#726722)
- Log exec path and arguments when fork()/execv()ing in ricci and modcluster
- Fixed ccs_sync to build with '-g' option.
- If ricci user password isn't set, log a warning message
- ccs_sync now returns 1 when there is an error syncing the file
- Prevent deadlock when executing external programs such as yum
- Resolves: rhbz#724014 rhbz#731113 rhbz#738567 rhbz#721113 rhbz#734910 rhbz#729168 rhbz#770637 rhbz#726722 rhbz#738797 rhbz#729011 rhbz#773383 rhbz#738008 rhbz#rhbz#736795

* Mon Feb 27 2012 Ryan McCabe <rmccabe@redhat.com> - 0.16.2-47
- Fix bz797267 (modcluster doesn't properly return the cluster schema)

* Mon Jan 02 2012 Chris Feist <cfeist@redhat.com> - 0.16.2-46
- Reverted changes from rhbz#736795 for review
- Resolves: rhbz#736795

* Mon Jan 02 2012 Jan Pokorny <jpokorny@redhat.com> - 0.16.2-45
- Prevent deadlock when executing external programs such as yum.
- Resolves: rhbz#736795

* Tue Oct 18 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-43
- Require modcluster to prevent ricci from malfunctioning
- Resolves: rhbz#721109

* Thu Sep 15 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-42
- Resynced cluster.rng from fence-agents, resource-agents & cman.
- Resolves: rhbz#725722

* Fri Sep 02 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-41
- Don't print out 'action' resource agent
- Resolves: rhbz#696901

* Thu Aug 11 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-39
- Added support for listing available servcies/fence devices.
- Resolves: rhbz#696901

* Wed Aug 10 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-38
- Synced up with latest cluster.rng from cman (including fence_ucs agent)
- Resolves: rhbz#725722

* Wed Jul 27 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-37
- Added support for --addvm/--rmvm
- Resolves: rhbz#718230

* Thu Jul 21 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-36
- Fixed issue with ccs_sync not sending to ipv6 hosts
- Resolves: rhbz#697493

* Tue Apr 05 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-35
- Fixed issues with logging in RHEL6
- Related: rhbz#682868

* Tue Apr 05 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-34
- Fixed several minor documentation issues
- Fixed --status errors
- Fixed missing --debug option
- Fixed traceback when using --help as an argument
- Related: rhbz#682868

* Tue Mar 29 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-33
- Fixed issue with --checkconf
- Resolves: rhbz#614647

* Fri Mar 11 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-32
- Add validation logic when first looking at the config file and after
  changing it
- Resolves: rhbz#682868

* Thu Mar 10 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-30
- Added ability to configure unfence section in ccs
- Fixed --setmulticast to properly remove multicast settings
- Resolves: rhbz#683192 rhbz#681646

* Tue Mar 08 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-29
- Fixed issues with piping and entering passwords into ccs_sync
- Resolves: rhbz#682317 rhbz#682323

* Thu Feb 03 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-28
- More fixes for ccs (added expert mode)
- Resolves: rhbz#614647

* Thu Jan 20 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-27
- More fixes for ccs
- Ricci no longer sends non-SSL over an SSL connection
- Resolves: rhbz#652837 rhbz#614647

* Tue Jan 04 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-22
- Load latest version of ccs
- Resolves: rhbz#614647

* Mon Jan 03 2011 Chris Feist <cfeist@redhat.com> - 0.16.2-21
- Added man page for ricci
- Resolves: rhbz#602399

* Mon Dec 06 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-17
- Load latest version of ccs
- Resolves: rhbz#614647

* Tue Nov 30 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-16
- Check password against ricci user instead of root user
- Resolves: rhbz#644047

* Mon Nov 29 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-14
- Added ccs package (CLI to do configuration)
- Resolves: rhbz#614647

* Mon Jul 26 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-13
- Fix issue with incorrect package/service lists for RHEL6
- Resolves: rhbz#617090

* Thu Jul 08 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-9
- Ricci will now start even if IPv6 is disabled
- Resolves: rhbz#612318

* Wed Jul 07 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-8
- Added a missing man page for ccs_sync
- Resolves: rhbz#610042

* Thu Jun 03 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-7
- Init script should now fully comply with Fedora Guidelines
- Resolves: rhbz#553384

* Tue May 18 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-6
- Init script now complies more closely with Fedora Guidelines
- Passing a bad node to ccs_sync no longer results in a segfault
- Resolves: rhbz#585126 rhbz#587526

* Mon May 17 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-5
- Added static UID/GID for ricci user
- Resolves: rhbz#585987

* Wed May 12 2010 Fabio M. Di Nitto <fdinitto@redhat.com> - 0.16.2-4
- Do not build on ppc and ppc64.
  Resolves: rhbz#591001

* Tue Apr 27 2010 Chris Feist <cfeist@redhat.com> - 0.16.2-3
- An issue with calling 'virsh nodelist' would cause ricci to hang for
  30 seconds during most requests resulting in timeouts to the web interface.
- Resolves: rhbz#580575

* Thu Feb 25 2010 Fabio M. Di Nitto <fdinitto@redhat.com> - 0.16.2-2
- Resolves: rhbz#568013
- Do not build ricci on s390 and s390x.

* Thu Jan 14 2010 Ryan McCabe <rmccabe@redhat.com> - 0.16.2-1
- make the ricci init script exit with status 2 when invalid arguments
  are provided

* Thu Dec 10 2009 Ryan McCabe <rmccabe@redhat.com> - 0.16.1-6
- Add a ricci function to set the cluster version, as it's no longer done
  while syncing the configuration files via ccs_sync.

* Wed Dec 09 2009 Ryan McCabe <rmccabe@redhat.com> - 0.16.1-5
- Don't update the cluster version via cman_set_version

* Wed Sep 16 2009 Tomas Mraz <tmraz@redhat.com> - 0.16.1-4
- Use password-auth common PAM configuration instead of system-auth

* Fri Aug 21 2009 Tomas Mraz <tmraz@redhat.com> - 0.16.1-3
- rebuilt with new openssl

* Sun Jul 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.16.1-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Thu Apr 16 2009 Ryan McCabe <rmccabe@redhat.com> 0.16.1-1
- More updates for cluster3.

* Tue Apr 07 2009 Ryan McCabe <rmccabe@redhat.com> 0.16.0-2
- Fix memory corruption bug.
- Update package and service list.
- Add missing dependency for nss-tools

* Mon Mar 30 2009 Ryan McCabe <rmccabe@redhat.com> 0.16.0-1
- Update for F11
- Fix build issues uncovered by g++ 4.4
- Remove legacy RHEL4 and RHEL5-specific code.

* Tue Mar 03 2009 Caolán McNamara <caolanm@redhat.com> - 0.15.0-11
- include stdio.h for perror, stdint.h for uint32_t

* Wed Feb 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.15.0-10
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Sat Jan 17 2009 Tomas Mraz <tmraz@redhat.com> 0.15.0-9
- rebuild with new openssl

* Wed Oct 22 2008 Ryan McCabe <rmccabe@redhat.com> 0.15.0-8
- Fix build.

* Wed Oct 22 2008 Ryan McCabe <rmccabe@redhat.com> 0.15.0-7
- Fix a bug that caused some connections to be dropped prematurely.
- Add better error reporting in the "ccs_sync" tool.

* Wed Oct 15 2008 Ryan McCabe <rmccabe@redhat.com> 0.15.0-6
- When setting a cluster.conf file with ccs_sync, only try to update the cman cluster version if the node is a member of a cluster.

* Mon Oct 06 2008 Ryan McCabe <rmccabe@redhat.com> 0.15.0-5
- Generate the ricci NSS certificate database at startup if it doesn't exist
- By default, set the "propagate" attribute to true when setting a new cluster
  configuration file with "ccs_sync"

* Fri Sep 26 2008 Fabio M. Di Nitto <fdinitto@redhat.com> 0.15.0-4
- Drop BuildRequires on cman-devel as it's not required.

* Fri Sep 26 2008 Fabio M. Di Nitto <fdinitto@redhat.com> 0.15.0-3
- Add versioned BR on cman-devel

* Tue Sep 09 2008 Ryan McCabe <rmccabe@redhat.com> 0.15.0-2
 - Add nss-devel to BuildDepends

* Tue Sep 09 2008 Ryan McCabe <rmccabe@redhat.com> 0.15.0-1
 - Break circular dependency with cman

* Mon Jun 02 2008 Ryan McCabe <rmccabe@redhat.com> 0.13.0-4
 - No longer need -lgroup with the new cman packages.
 - Recognize F9 by name (Sulphur).

* Tue May 20 2008 Ryan McCabe <rmccabe@redhat.com> 0.13.0-3
 - Initial build

* Wed Mar 26 2008 Chris Feist <cfeist@redhat.com> 0.13.0-2
 - Don't require cap and xml libraries (RPM will find them)
 - Fix buildroot to meet Fedora standard

* Wed Feb 20 2008 Ryan McCabe <rmccabe@redhat.com> 0.13.0-1
 - Initial build.
