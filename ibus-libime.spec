Name:           ibus-libime
Version:        0.1.2
Release:        1%{?dist}
Summary:        An ibus front-end for libime

License:        GPL-3.0 AND CC-BY-SA-4.0
URL:            empty
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  g++
BuildRequires:  cmake
BuildRequires:  ibus-devel
BuildRequires:  libime-devel
BuildRequires:  glibmm2.4-devel
BuildRequires:  gtk3-devel
BuildRequires:  fcitx5-devel
Requires:       ibus
Requires:       libime
Requires:       libime-data
Requires:       glibmm2.4
Requires:       gtk3
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
%{_libexecdir}/ibus-setup-libime
%{_datadir}/applications/ibus-setup-libime.desktop
%{_datadir}/ibus-libime/
%license LICENSE
%doc README.md



%changelog
* Fri Oct 30 2026 Weifeng Liu <weifeng.liu.z@gmail.com> 0.1.2-1
- build: synchronize version metadata with Tito (weifeng.liu.z@gmail.com)
- Bump to version 0.1.2 (weifeng.liu.z@gmail.com)
- Organize preferences into tabs and add friendly fuzzy selections
  (weifeng.liu.z@gmail.com)
- Move preferences descriptions into option tooltips (weifeng.liu.z@gmail.com)
- Add GTK preferences with live settings and tray launcher
  (weifeng.liu.z@gmail.com)
- Fix user data persistence and orderly shutdown (weifeng.liu.z@gmail.com)
- Add input history and dictionary learning settings (weifeng.liu.z@gmail.com)
- Record committed Chinese input history for personal dictionaries
  (weifeng.liu.z@gmail.com)
- Clarify tray input mode labels (weifeng.liu.z@gmail.com)
- Fix tray input mode switching across panel focus changes
  (weifeng.liu.z@gmail.com)
- Fix partial pinyin candidates and preserve selectable raw remainders
  (weifeng.liu.z@gmail.com)
- Add English prefix candidates in Chinese mode (weifeng.liu.z@gmail.com)
- Fix delayed input-mode callback lifetime (weifeng.liu.z@gmail.com)
- Save user history (weifeng.liu.z@gmail.com)

* Wed Feb 04 2026 Weifeng Liu <weifeng.liu.z@gmail.com> 0.1.1-1
- Bump to version 0.1.1 (weifeng.liu.z@gmail.com)
- Add .gitignore (weifeng.liu.z@gmail.com)
- README: Add link for libime (weifeng.liu.z@gmail.com)
- Pass-through Esc when there is no pending input (weifeng.liu.z@gmail.com)
- Fix single and double quote in Chinese mode (weifeng.liu.z@gmail.com)
- scripts: Show tgz file generated (weifeng.liu.z@gmail.com)
- Fix space handling (weifeng.liu.z@gmail.com)
- Support comma-separated fuzzy flag items in config (weifeng.liu.z@gmail.com)
- Reset current page when user commits a partial result (weifeng.liu.z@gmail.com)
- Allow setting fuzzy_flags, nbest and page_size from config file
  (weifeng.liu.z@gmail.com)
- Add a script to generate tgz source file (weifeng.liu.z@gmail.com)
- Use VERSION file for consistent versioning (weifeng.liu.z@gmail.com)
- Fix PageUp/PageDown in Chinese mode (weifeng.liu.z@gmail.com)
- Fix Win+Character sequence in Chinese mode (weifeng.liu.z@gmail.com)
- Add .clang-format to follow llvm style (weifeng.liu.z@gmail.com)
- Format code (weifeng.liu.z@gmail.com)
- Read configs from file (weifeng.liu.z@gmail.com)
- Fix inputing -/= in Chinese mode (weifeng.liu.z@gmail.com)
- Pass-through Super key (weifeng.liu.z@gmail.com)
- Update README (weifeng.liu.z@gmail.com)
- Initial commit (weifeng.liu.z@gmail.com)
