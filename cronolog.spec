%define name cronolog
%define version 1.6.2
%define release 1
%define group System Networking/Daemons

Summary:	a flexible log file rotation program for Apache
Name:		%{name}
Version:	%{version}
Release:	%{release}
Copyright: 	Apache license
Group:		%{group}
Packager:	Andrew Ford <A.Ford@ford-mason.co.uk>
URL: 		http://www.ford-mason.co.uk/resources/cronolog/

Source:		http://www.ford-mason.co.uk/resources/cronolog/cronolog-%version.tar.gz

BuildRoot:	/tmp/%{name}-root

%description
"cronolog" is a simple program that reads log messages from its input
and writes them to a set of output files, the names of which are
constructed using template and the current date and time.  The
template uses the same format specifiers as the Unix date command
(which are the same as the standard C strftime library function).

%changelog

%prep
%setup -n %{name}-%{version}

%build

./configure
make 


%install
rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/usr/share/doc/%{name}-${RPM_PACKAGE_VERSION} -m 755
make prefix=$RPM_BUILD_ROOT/usr mandir=$RPM_BUILD_ROOT/usr/share/man install
install -m 644 README $RPM_BUILD_ROOT/usr/share/doc/%name-${RPM_PACKAGE_VERSION}
#install -m 644 $RPM_SOURCE_DIR/doc/cronolog.1m $RPM_BUILD_ROOT/usr/man/man1/cronolog.1
#install -m 755 $RPM_SOURCE_DIR/src/cronolog $RPM_BUILD_ROOT/usr/sbin/cronolog
#strip  $RPM_BUILD_ROOT/usr/sbin/* || echo Ignored strip on a non-binary file


%post

%preun

%postun


%clean
rm -rf $RPM_BUILD_ROOT


%files
#%attr(-,root,root) /usr/share/doc/%{name}-%{version}/README
%attr(-,root,root) /usr/sbin/cronolog
%attr(-,root,root) /usr/sbin/cronosplit

#%files man
%attr(644,root,root) /usr/share/man/man1/*.1*

%doc README
