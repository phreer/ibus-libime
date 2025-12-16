Name:           ibus-libime
Version:        0.3
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
cmake -B builddir -S . -DCMAKE_BUILD_TYPE=Release
cmake --build builddir -- -j %{?_smp_mflags}


%install
cmake --install builddir --prefix %{buildroot}%{_prefix}


%files
%license LICENSE
%doc README.md



%changelog
* Tue Dec 16 2025 Weifeng Liu <weifeng.liu@intel.com> 0.3-1
- Add deps (weifeng.liu@intel.com)

* Tue Dec 16 2025 Weifeng Liu <weifeng.liu@intel.com>
- 
