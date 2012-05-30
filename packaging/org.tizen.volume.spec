Name:       org.tizen.volume
Summary:    volume
Version:	0.2.0
Release:    3
Group:      Applications
License:    Samsung Proprietary License
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/org.tizen.volume.manifest 
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(ecore-input)
BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(mm-sound)
BuildRequires: pkgconfig(svi)
BuildRequires: pkgconfig(syspopup)
BuildRequires: pkgconfig(utilX)
BuildRequires: pkgconfig(vconf)

BuildRequires:  cmake
BuildRequires:  edje-tools
BuildRequires:  embryo-bin
BuildRequires:  gettext-tools

%description
Volume App


%prep
%setup -q -n %{name}-%{version}


%build
cp %{SOURCE1001} .
RPM_OPT=`echo $CFLAGS|sed -n 's/-Wp,-D_FORTIFY_SOURCE=2//'`
export CFLAGS=$RPM_OPT
cmake  -DCMAKE_INSTALL_PREFIX="/opt/apps/org.tizen.volume"
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%find_lang volume

%files -f volume.lang
%manifest org.tizen.volume.manifest
/opt/apps/org.tizen.volume/bin/*
/opt/apps/org.tizen.volume/res/*
/opt/share/applications/*
