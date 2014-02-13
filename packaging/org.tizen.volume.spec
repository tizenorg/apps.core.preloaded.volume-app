#sbs-git:slp/pkgs/v/volume-app volume-app 0.1.2 226202351de9fefb43756c36d215ca74f52431d0
Name:	org.tizen.volume
Summary:	Volume application (EFL)
Version:	0.2.8
Release:	1
Group:	TO_BE/FILLED_IN
License:	Flora Software License
Source0:	%{name}-%{version}.tar.gz
Source1001: 	org.tizen.volume.manifest
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(syspopup)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(svi)
BuildRequires:  pkgconfig(ui-gadget-1)
BuildRequires:	pkgconfig(notification)
BuildRequires:	pkgconfig(capi-system-info)
BuildRequires:	pkgconfig(feedback)
BuildRequires:  pkgconfig(libtzplatform-config)

BuildRequires:  cmake
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  gettext-devel

%description
volume.

%prep
%setup -q
cp %{SOURCE1001} .

%define PREFIX /usr/apps/org.tizen.volume

%build
RPM_OPT=`echo $CFLAGS|sed 's/-Wp,-D_FORTIFY_SOURCE=2//'`
export CFLAGS=$RPM_OPT
cmake  -DCMAKE_INSTALL_PREFIX="%{PREFIX}" -DTZ_SYS_SHARE=%TZ_SYS_SHARE -DTZ_SYS_ETC=%TZ_SYS_ETC -DTZ_SYS_BIN=%TZ_SYS_BIN
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{PREFIX}/bin/*
%{PREFIX}/res/*
%{TZ_SYS_SHARE}/icons/default/small/org.tizen.volume.png
/usr/share/packages/org.tizen.volume.xml
%{TZ_SYS_ETC}/smack/accesses.d/org.tizen.volume.rule
