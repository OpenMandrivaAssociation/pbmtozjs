Summary:	Driver for the HP LaserJet 1000 GDI printers
Name: 		pbmtozjs
Version:	0
Release:	%mkrel 12
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


%changelog
* Wed May 04 2011 Oden Eriksson <oeriksson@mandriva.com> 0-12mdv2011.0
+ Revision: 666998
- mass rebuild

* Fri Dec 03 2010 Oden Eriksson <oeriksson@mandriva.com> 0-11mdv2011.0
+ Revision: 607081
- rebuild

* Sun Mar 14 2010 Oden Eriksson <oeriksson@mandriva.com> 0-10mdv2010.1
+ Revision: 519053
- rebuild

* Thu Sep 03 2009 Christophe Fergeau <cfergeau@mandriva.com> 0-9mdv2010.0
+ Revision: 426373
- rebuild

* Sat Jan 31 2009 Oden Eriksson <oeriksson@mandriva.com> 0-8mdv2009.1
+ Revision: 335845
- rebuilt against new jbigkit major

* Thu Dec 25 2008 Oden Eriksson <oeriksson@mandriva.com> 0-7mdv2009.1
+ Revision: 319035
- use %%ldflags

* Tue Jun 17 2008 Thierry Vignaud <tv@mandriva.org> 0-6mdv2009.0
+ Revision: 223445
- rebuild

* Tue Mar 04 2008 Oden Eriksson <oeriksson@mandriva.com> 0-5mdv2008.1
+ Revision: 179154
- rebuild

  + Olivier Blin <oblin@mandriva.com>
    - restore BuildRoot

  + Thierry Vignaud <tv@mandriva.org>
    - kill re-definition of %%buildroot on Pixel's request

* Thu Aug 30 2007 Oden Eriksson <oeriksson@mandriva.com> 0-4mdv2008.0
+ Revision: 75352
- fix deps (pixel)

* Fri Aug 17 2007 Oden Eriksson <oeriksson@mandriva.com> 0-3mdv2008.0
+ Revision: 65216
- bump release
- use the new System/Printing RPM GROUP

* Tue Aug 14 2007 Oden Eriksson <oeriksson@mandriva.com> 0-1mdv2008.0
+ Revision: 63102
- Import pbmtozjs



* Tue Aug 14 2007 Oden Eriksson <oeriksson@mandriva.com> 0-1mdv2008.0
- initial Mandriva package
