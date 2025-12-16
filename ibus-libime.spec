Name:           ibus-libime
Version:        0.13
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
* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.13-1
- test (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.12-1
- test (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.11-1
- test (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.10-1
- test (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.9-1
- test (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.8-1
- test (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.7-1
- Don't use icon for language state (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.6-1
- Fix paths (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.5-1
- Fix cmake lists (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.4-1
- Use %%cmake_install (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.3-1
- Use standard cmake commands in spec (weifeng.liu@intel.com)

* Wed Dec 17 2025 Weifeng Liu <weifeng.liu@intel.com> 0.2-1
- Set install prefix (weifeng.liu@intel.com)

* Tue Dec 16 2025 Weifeng Liu <weifeng.liu@intel.com> 0.5-1
- Add installed files (weifeng.liu@intel.com)

* Tue Dec 16 2025 Weifeng Liu <weifeng.liu@intel.com> 0.4-1
- Add deps (weifeng.liu@intel.com)

* Tue Dec 16 2025 Weifeng Liu <weifeng.liu@intel.com> 0.3-1
- Add deps (weifeng.liu@intel.com)

* Tue Dec 16 2025 Weifeng Liu <weifeng.liu@intel.com>
- 
