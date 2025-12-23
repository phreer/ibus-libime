%global version %(cat %{_sourcedir}/VERSION 2>/dev/null || echo 0.1.0)

Name:           ibus-libime
Version:        %{version}
Release:        1%{?dist}
Summary:        An ibus front-end for libime

License:        GLP-3.0
URL:            empty
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  g++
BuildRequires:  cmake
BuildRequires:  ibus-devel
BuildRequires:  libime-devel
BuildRequires:  glibmm2.4-devel
BuildRequires:  fcitx5-devel
Requires:       ibus
Requires:       libime
Requires:       libime-data
Requires:       glibmm2.4
Requires:       fcitx5-libs

%description
ibus-libime is an ibus front-end for libime.


%prep
%autosetup


%build
%cmake
%cmake_build


%install
%cmake_install


%files
%{_datadir}/ibus/component/libime.xml
%{_libexecdir}/ibus-engine-libime
%license LICENSE
%doc README.md



%changelog
