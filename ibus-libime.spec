Name:           ibus-libime
Version:        0.2
Release:        1%{?dist}
Summary:        An ibus front-end for libime

License:        GLP-3.0
URL:            empty
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  g++
BuildRequires:  cmake
BuildRequires:  ibus-devel
Requires:       ibus

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
* Tue Dec 16 2025 Weifeng Liu <weifeng.liu@intel.com> 0.2-1
- new package built with tito

* Tue Dec 16 2025 Weifeng Liu <weifeng.liu@intel.com>
- 
