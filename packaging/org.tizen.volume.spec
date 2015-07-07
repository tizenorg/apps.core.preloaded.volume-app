%define _project_name volume
%define _package_name org.tizen.%{_project_name}
%if "%{?tizen_profile_name}" == "wearable"
ExcludeArch: %{arm} %ix86 x86_64
%endif

%if "%{?tizen_profile_name}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

Name:	org.tizen.volume
Summary:	Volume application (EFL)
Version:	0.1.148
Release:	1
Group:	TO_BE/FILLED_IN
License:	Apache
Source0:	%{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(capi-appfw-app-manager)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(elementary)
#BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(capi-media-sound-manager)
BuildRequires:  pkgconfig(syspopup)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(feedback)
BuildRequires:  pkgconfig(syspopup-caller)
#BuildRequires:  pkgconfig(x11)
#BuildRequires:  pkgconfig(xcomposite)
#BuildRequires:  pkgconfig(xext)
#BuildRequires:  pkgconfig(xi)
BuildRequires:  pkgconfig(capi-network-bluetooth)

BuildRequires:  cmake
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  gettext-devel
BuildRequires:	hash-signer

%description
volume.

%prep
%setup -q

%define PREFIX /usr/apps/%{_package_name}

%build
export CFLAGS+=" -fPIE"
export CXXFLAGS+=" -fPIE"
export LDFLAGS+=" -pie"

%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%if 0%{?sec_build_binary_sdk}
export CFLAGS+=" -DFEATURE_SDK"
export CXXFLAGS+=" -DFEATURE_SDK"
export FFLAGS+=" -DFEATURE_SDK"

echo EMULATOR BUILD
%endif

RPM_OPT=`echo $CFLAGS|sed 's/-Wp,-D_FORTIFY_SOURCE=2//'`
export CFLAGS=$RPM_OPT
cmake  -DCMAKE_INSTALL_PREFIX="%{PREFIX}"
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{_package_name}
%define tizen_sign 1
%define tizen_sign_base /usr/apps/%{_package_name}
%define tizen_sign_level public
%define tizen_author_sign 1
%define tizen_dist_sign 1

%files
%manifest %{_package_name}.manifest
%defattr(-,root,root,-)
%{PREFIX}/*
#%{PREFIX}/bin/*
#%{PREFIX}/res/*
/usr/share/packages/%{_package_name}.xml
/usr/share/icons/default/small/%{_package_name}.png
/usr/share/license/%{_package_name}
