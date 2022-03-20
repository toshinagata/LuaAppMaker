#!/usr/bin/perl
#
#  Prepare AppDir for creating AppImage
#  (1) Find the executable (from -e option)
#  (2) Find the needed libraries (by calling ldd)
#  (3) Repeat (2) for the newly found libraries
#  (4) Remove the 'excluded' libraries
#  (5) Create the AppDir directory (from --appdir option)
#  (6) Copy the libraries to AppDir/usr/lib
#  (7) Patch the libraries for RUNPATH to be $ORIGIN
#  (8) Strip the libraries
#  (9) Copy the executable to AppDir/usr/bin
#  (10) Patch the executable for RUNPATH to be $ORIGIN/../lib
#  (11) Create AppDir/AppRun as a symbolic link to the executable
#  (12) Examine the icon PNG or SVG (from -i option)
#  (13) Copy the icon PNG to AppDir/usr/share/icons/hicolor/128x128/apps (or 256x256 or somewhere like that)
#  (14) Create desktop file in AppDir/usr/share/applications/$(APPNAME).desktop
#  (15) Make symbolic links to desktop file and icon filer in AppDir

use File::Copy;
use File::Basename;

sub usage() {
  print STDERR "Usage: perl prepare_appdir.pl -e executable -i icon --appdir appdir\n";
  print STDERR "Prepare an AppDir from the executable file and icon file\n";
  exit 1;
}

while ($arg = shift(@ARGV)) {
  if ($arg eq "-e") {
    $executable = shift(@ARGV);
  } elsif ($arg eq "-i") {
    $icon = shift(@ARGV);
  } elsif ($arg eq "--appdir") {
    $appdir = shift(@ARGV);
  } elsif ($arg eq "-f") {
    $force_overwrite = 1;
  } else {
    print STDERR "Unknown option $arg\n";
    usage();
  }
}

if (!$executable) {
  print STDERR "Executable is not specified\n";
  usage();
}
if (!-e $executable) {
  print STDERR "Cannot find executable $executable\n";
  exit 1;
}
$execname = basename($executable);
if (!$icon) {
  print STDERR "Icon file is not specified\n";
  usage();
}
if (!-e $icon) {
  print STDERR "Cannot find icon file $icon\n";
  exit 1;
}
$iconname = basename($icon);
if (!$appdir) {
  print STDERR "AppDir is not specified\n";
  usage();
}
if (-e $appdir && !$force_overwrite) {
  print STDERR "AppDir is already present. To overwrite, specify -f option.\n";
  exit 1;
}

#  Machine architecture
chomp($arch = `uname -m`);
if (!$arch) {
  die "Cannot find architecture by uname -m: $!";
}

#  Examine the icon file
open(IDENTIFY, "identify \"$icon\" |") || die "Cannot execute identify: $!";
$line = <IDENTIFY>;
close(IDENTIFY);
$line = substr($line, length($icon));
if ($line =~ /\s+(\w+)\s+([x0-9]+)/) {
  $icon_type = $1;
  $icon_size = $2;
} else {
  die "Cannot identify the icon type and size for $icon";
}

if ($icon_type ne "PNG" && $icon_type ne "SVG") {
  die "The icon file must be PNG or SVG";
}
if ($icon_type eq "PNG" && ($icon_size ne "16x16" && $icon_size ne "32x32" && $icon_size ne "64x64" && $icon_size ne "128x128" && $icon_size ne "256x256")) {
  die "The icon size must be one of the following: 16x16, 32x32, 64x64, 128x128, 256x256";
}

#  Download exclude list
$url = "https://raw.githubusercontent.com/probonopd/AppImages/master/excludelist";
@exclude_list = split("\n", `wget --quiet \"$url\" -O - | sed 's|#.*||g' | sort | uniq`);

#  Find library
my %libraries = ();

sub find_library {
  my ($elf) = @_;
  my @new;
  # print "find_library(${elf})\n";
  open(LDD, "ldd \"${elf}\" |") || die "Cannot execute ldd: $!";
  while (<LDD>) {
    if (/^\s*(.*) => (.*) \(0x/) {
      my $soname = $1;
      my $sopath = $2;
      if (!$libraries{$soname}) {
        push(@new, $sopath);
        $libraries{$soname} = $sopath;
        #print("  ${soname} : ${sopath}\n");
        #print("${soname}\n");
      }
    }
  }
  close(LDD);
  return @new;
}

#  Find needed shared libraries
@libs = ($executable);
while ($#libs >= 0) {
  my $i;
  my @newlibs;
  for ($i = 0; $i <= $#libs; $i++) {
    my @foundlibs = find_library($libs[$i]);
    push(@newlibs, @foundlibs);
  }
  @libs = @newlibs;
}

#  Check the exclude list
@keys = sort keys(%libraries);
foreach $key (@keys) {
  if (grep { $key eq $_ } @exclude_list) {
    print "$key is excluded\n";
    delete $libraries{$key};
    next;
  }
  print("$key : $libraries{$key}\n");
}

#  Create target directory
print("Creating ${appdir}...\n");
system("rm -rf \"${appdir}\"");
system("mkdir -p \"${appdir}/usr/lib\"") == 0 || die "Cannot create directory ${appdir}/usr/lib: $!";

#  Copy the needed libraries, patch RPATH, and strip
foreach $key (sort keys(%libraries)) {
  print "Copying $libraries{$key} as usr/lib/${key}\n";
  copy($libraries{$key}, "${appdir}/usr/lib/${key}") || die "Failed to copy $libraries{$key} to ${appdir}/usr/lib/${key}: $!";
  print "Setting RPATH and stripping usr/lib/${key}\n";
  system("patchelf --set-rpath '\$ORIGIN' \"${appdir}/usr/lib/${key}\"") == 0 || die "Failed to set RPATH for ${key}: $!";
  system("strip \"${appdir}/usr/lib/${key}\"") == 0 || die "Failed to strip ${key}: $!";
}

#  Copy the executable and patch RPATH
system("mkdir -p \"${appdir}/usr/bin\"") == 0 || die "Cannot create directory ${appdir}/usr/bin: $!";
print("Copying executable to usr/bin/${execname}\n");
copy($executable, "${appdir}/usr/bin/${execname}") || die "Failed to copy $executable to ${appdir}/usr/bin/${execname}: $!";
system("chmod +x \"${appdir}/usr/bin/${execname}\"") == 0 || die "Cannot add +x permission to usr/bin/${execname}: $!";
system("patchelf --set-rpath '\$ORIGIN/../lib' \"${appdir}/usr/bin/${execname}\"") == 0 || die "Failed to set RPATH for ${execname}: $!";

#  Create AppRun as a symbolic link to the executable
system("ln -s \"usr/bin/${execname}\" \"${appdir}/AppRun\"") == 0 || die "Failed to create AppRun as a symbolic link to usr/bin/${execname}: $!";

#  Copy the icon file
foreach $subpath (("16x16", "32x32", "64x64", "128x128", "256x256", "scalable")) {
  system("mkdir -p \"${appdir}/usr/share/icons/hicolor/${subpath}/apps\"") == 0 || die "Failed to create directory usr/share/icons/hicolor/${subpath}/apps: $!";
}
if ($icon_type eq "PNG") {
  $icon_path = "usr/share/icons/hicolor/${icon_size}/apps";
} else {
  $icon_path = "usr/share/icons/hicolor/scalable/apps";
}
print("Copying the icon file to ${icon_path}\n");
copy($icon, "${appdir}/${icon_path}/${iconname}") || die "Failed to copy $icon to ${appdir}/${icon_path}: $!";
system("ln -s \"${icon_path}/${iconname}\" \"${appdir}/${iconname}\"") == 0 || die "Failed to create symbolic link to ${icon_path}: $!";

#  Create .DirIcon as a symbolic link to the icon file
system("ln -s \"${iconname}\" \"${appdir}/.DirIcon\"") == 0 || die "Failed to create .DirIcon as a symbolic link to ${iconname}: $!";

#  Create the desktop file
$desktop_path = "usr/share/applications";
($iconbasename = $iconname) =~ s/\.\w+$//;  # Icon base name without extension
system("mkdir -p \"${appdir}/${desktop_path}\"") == 0 || die "Failed to create directory ${desktop_path}: $!";
print("Creating the desktop file ${desktop_path}/${execname}.desktop\n");
open(DESKTOP, ">", "${appdir}/${desktop_path}/${execname}.desktop") || die "Cannot create desktop file at ${appdir}/${desktop_path}: $!";
print DESKTOP "[Desktop Entry]\n";
print DESKTOP "Name=${execname}\n";
print DESKTOP "Exec=${execname}\n";
print DESKTOP "Icon=${iconbasename}\n";
print DESKTOP "Type=Application\n";
print DESKTOP "Categories=Utility;\n";
close(DESKTOP);
system("ln -s \"${desktop_path}/${execname}.desktop\" \"${appdir}/${execname}.desktop\"") == 0 || die "Failed to create symbolic link to ${desktop_path}: $!";

print "=== ${appdir}/usr/bin and ${appdir}/usr/lib were successfully produced. ===\n";

exit 0;
