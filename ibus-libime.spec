%global version %(cat %{_sourcedir}/VERSION 2>/dev/null || echo 0.1.0)

Name:           ibus-libime
Version:        0.1.1
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
* Wed Feb 04 2026 Weifeng Liu <weifeng.liu@intel.com> 0.1.1-1
- Bump to version 0.1.1 (weifeng.liu@intel.com)
- Add .gitignore (weifeng.liu@intel.com)
- README: Add link for libime (weifeng.liu@intel.com)
- Pass-through Esc when there is no pending input (weifeng.liu@intel.com)
- Fix single and double quote in Chinese mode (weifeng.liu@intel.com)
- scripts: Show tgz file generated (weifeng.liu@intel.com)
- Fix space handling (weifeng.liu@intel.com)
- Support comma-separated fuzzy flag items in config (weifeng.liu@intel.com)
- Reset current page when user commits a partial result (weifeng.liu@intel.com)
- Allow setting fuzzy_flags, nbest and page_size from config file
  (weifeng.liu@intel.com)
- Add a script to generate tgz source file (weifeng.liu@intel.com)
- Use VERSION file for consistent versioning (weifeng.liu@intel.com)
- Fix PageUp/PageDown in Chinese mode (weifeng.liu@intel.com)
- Fix Win+Character sequence in Chinese mode (weifeng.liu@intel.com)
- Add .clang-format to follow llvm style (weifeng.liu@intel.com)
- Format code (weifeng.liu@intel.com)
- Read configs from file (weifeng.liu@intel.com)
- Fix inputing -/= in Chinese mode (weifeng.liu@intel.com)
- Pass-through Super key (weifeng.liu@intel.com)
- Update README (weifeng.liu@intel.com)
- Initial commit (weifeng.liu@intel.com)

