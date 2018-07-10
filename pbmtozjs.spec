Summary:	Driver for the HP LaserJet 1000 GDI printers
Name:		pbmtozjs
Version:	0
Release:	23
License:	GPLv2
Group:		System/Printing
Url:		http://www.linuxprinting.org/download/printing/pbmtozjs.c
Source0:	http://www.linuxprinting.org/download/printing/pbmtozjs.c
BuildRequires:	jbig-devel

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
install -d %{buildroot}%{_bindir}
install -m0755 pbmtozjs %{buildroot}%{_bindir}/

%files
%doc pbmtozjs.txt
%{_bindir}/pbmtozjs

