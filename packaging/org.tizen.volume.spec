#sbs-git:slp/pkgs/v/volume-app volume-app 0.1.2 226202351de9fefb43756c36d215ca74f52431d0
Name:	org.tizen.volume
Summary:	Volume application (EFL)
Version:	0.2.4
Release:	1
Group:	TO_BE/FILLED_IN
License:	Flora Software License
Source0:	%{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(syspopup)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(svi)
BuildRequires:  pkgconfig(ui-gadget-1)

BuildRequires:  cmake
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  gettext-devel

%description
volume.

%prep
%setup -q

%define PREFIX /opt/apps/org.tizen.volume

%build
RPM_OPT=`echo $CFLAGS|sed 's/-Wp,-D_FORTIFY_SOURCE=2//'`
export CFLAGS=$RPM_OPT
cmake  -DCMAKE_INSTALL_PREFIX="%{PREFIX}"
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%files
%defattr(-,root,root,-)
%{PREFIX}/bin/*
%{PREFIX}/res/*
/opt/share/packages/org.tizen.volume.xml
/opt/share/icons/default/small/org.tizen.volume.png
