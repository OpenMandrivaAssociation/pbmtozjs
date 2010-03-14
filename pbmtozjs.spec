Summary:	Driver for the HP LaserJet 1000 GDI printers
Name: 		pbmtozjs
Version:	0
Release:	%mkrel 10
License:	GPL
Group:		System/Printing
URL:		http://www.linuxprinting.org/download/printing/pbmtozjs.c
Source0:	http://www.linuxprinting.org/download/printing/pbmtozjs.c
BuildRequires:	jbig-devel
Conflicts:	printer-utils = 2007
Conflicts:	printer-filters = 2007
BuildRoot:	%{_tmppath}/%{name}-%{version}-root

%description
Driver for the HP LaserJet 1000 GDI printers. Perhaps it also works with some
other GDI printers made by QMS and Minolta (these manufacturer names appear in
the driver's source code).

%prep

%setup -q -c -T -n %{name}
cp %{SOURCE0} .

head -34 pbmtozjs.c | tail -33 > pbmtozjs.txt

%build
ln -s %{_includedir}/jbig.h .
%{__cc} %{optflags} %{ldflags} -o pbmtozjs pbmtozjs.c -ljbig


%install
rm -rf %{buildroot}

install -d %{buildroot}%{_bindir}

install -m0755 pbmtozjs %{buildroot}%{_bindir}/

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc pbmtozjs.txt
%attr(0755,root,root) %{_bindir}/pbmtozjs
