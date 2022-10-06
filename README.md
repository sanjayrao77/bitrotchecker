# bitrotchecker

## Description

Would you know if one of your old files changed on disk? Would that corrupted
file then be included in backups and displace good copies? As we store more data
and our drives inevitably get older, the potential for silent data loss becomes very
real.

This program is meant to track hidden changes in archived files. By running this tool
periodically, you can know when files are corrupted and have a chance to
restore them from good backups.

Alternatives are btrfs, zfs and dm-integrity. These all have real-time data checking
features, at a cost.

Advantages:
1. runs at designated times, and doesn't slow the system otherwise
2. checksums themselves can be transfered and backed up
3. can be run during tar backups, piggybacking on the same disk reads
4. has throttling support to limit io impact
5. checksum files are compatible with standard md5sum tools
6. can be used on existing filesystems

Disadvantages:
1. can't track data corruption in real-time
2. can't track data corruption in often-changing files like logs

## License

This software is licensed under GPLv2. Further information is in every source
file.

## Building

Currently, only linux and OSX are supported. No extra libraries are required.

On linux systems, you should be able to just download the
files and run "make".

On modern OSX systems, you should be able to download the files and run "make -f Makefile.osx".
I've only tested this on an M1 and haven't done extensive testing.

For other systems, you can remove -DUSEMMAP to use read() instead. It should be
easy to port to other unix-like systems.

The standard Makefile compiles a native implementation of md5. It's probably not
the fastest. You can use Makefile.gnutls and Makefile.openssl to use GNU-TLS and
OpenSSL for their md5 routines. You'll need header and library files for that
to be successful. Debian calls these libgnutls28-dev and libssl-dev. E.g., you
can run "apt-get install libgnutls28-dev" to install the required dependencies 
for GNU-TLS on Debian.

## Quick start

To run this for the first time, try the following commands. This will scan /bin and
store checksums as an md5sum-compatible file "/tmp/bin.md5".
```bash
./bitrotchecker --help
./bitrotchecker --progress /tmp/bin.md5 /bin
(cd /bin ; md5sum -c /tmp/bin.md5)
```

## checksumfile

This program reads and saves md5 checksums with a user-specified file. This
file is a simple text file listing md5 values and filenames.

On the first run, it will create the checksumfile. On subsequent runs, it will read
this checksumfile and use that as a basis to track changes in md5 checksums.

The file format is compatible with the system utility, "md5sum". On my systems,
I can run "md5sum -c checksumfile" to have md5sum verify the checksums without
using bitrotchecker. This can be handy if you want to verify files on a backup
system.

Here's an example checksumfile:
```
88c96ccaddd11b931ad6238e04ee0d88  etc/dhcp/dhclient-enter-hooks.d/resolvconf
95e21c32fa7f603db75f1dc33db53cf5  etc/dhcp/dhclient-exit-hooks.d/rfc3442-classless-routes
a891f21f45b0648b7082d999bf424591  etc/dhcp/dhclient-exit-hooks.d/timesyncd
521717b5f9e08db15893d3d062c59aeb  etc/dhcp/debug
649563ef7a61912664a400a5263958a6  etc/dhcp/dhclient.conf
```

### A warning on pathnames

The checksumfile has pathnames relative to a starting point. Filenames with tar have
a similar issue. The pathnames are relative to the _directory name_ specified when
directory scanning. When tar streaming, they're relative to tar's choice. 

If you want to compare existing files to a saved checksumfile, you'll want to
make sure the starting points are the same so the files line up. You can use
"--dry-run --verbose" to check the alignment.

**Warning**: if you run bitrotchecker with an existing checksumfile but a
different starting point, it will **remove** your old md5 values since the old
files will appear to no longer exist. You can use "--dry-run --verbose" to
check if a new command matches an old one.

Example:

Suppose you run "bitrotchecker /tmp/md5s.txt /bin". This will save "bash"
as an entry in md5s.txt. If you then run "cd /bin ; bitrotchecker /tmp/md5s.txt ."
this will also save "bash" so everything is fine. If you run "cd /bin ; md5sum -c /tmp/md5s.txt",
that will also work fine.

However, if you then run "tar -cf - /bin | bitrotchecker --tar /tmp/md5s.txt" then
you'll have problems. As is, "bin/bash" will be saved as an entry to /tmp/md5s.txt and the "bash"
entry will be removed! The old entries will be removed since it will look like the files are
gone.

For this example, "(cd /bin ; tar -cf - .) | bitrotchecker --tar /tmp/md5s.txt" would
have created matching entries in md5s.txt and it should have worked fine. Note the change of directory
for tar.

To have been safe, "tar -cf - /bin | bitrotchecker --dry-run --verbose --tar /tmp/md5s.txt"
could have been run to check for mismatching filenames. Note the "--dry-run --verbose" flags.


## Directory scanning or tar

This program can either scan a live filesystem for files or it can scan
uncompressed tar streams.

These tar streams can either come from backup files or from live tar backups.

Example of directory scanning:
```bash
./bitrotchecker /tmp/bin_md5s.txt /bin
```

Example of tar streaming:
```bash
(cd /bin ; tar -cf - .) | ./bitrotchecker --tar --tar-stdout /tmp/bin_md5s.txt | gzip > /mnt/backups/backup_bin.tgz
```

Example of tar files:
```bash
zcat /mnt/backups/backup_bin.tgz | ./bitrotchecker --tar /tmp/bin_md5s.txt
```

## When files go bad

If a file doesn't match its md5, bitrotchecker will check the modification time.

If the modification time of the file is newer than the checksumfile, then bitrotchecker
will assume the file has legitimately changed.

If the modification time of the file is older than the checksumfile, then bitrotchecker
will assume the file _may_ have been corrupted. When this happens, a message will be
printed to stderr or stdout (depending on --tar-stdout).

When bitrotchecker suspects corruption, it will *not* update the md5 entry for the file;
it will keep the older md5 value that doesn't currently match the file. This allows
the user to compare backup copies to the last md5 value.

Subsequent runs of bitrotchecker will *continue* to print the mismatched md5 message since
the old value is preserved. To clear this message, you can either delete the matching line in
the checksumfile or you can use the "--savechanges" flag to update all old md5 values with
new ones.

## Usage

Usage: bitrotchecker [OPTIONS] CHECKSUMFILE [DIRECTORY]

The order of parameters doesn't matter. The program will assume a directory is
the directory and a regular file is the checksumfile.

### --help

This will print the following:

```
bitrotchecker scans a directory for changes, using a file listing md5 digests, compatible with md5sum
Usage: bitrotchecker [options] checksumfile directory
  --dry-run: don't overwrite checksumfile
  --follow: follow symlinks
  --nothingnew: only process files in checksumfile
  --nottoday: skip files that have changed recently
  --one-file-system: don't cross filesystems when scanning directory
  --progress: print filenames along the way
  --savechanges: update md5 values for files that have changed
  --slow: limit reading to approx 13MB/sec
  --slower: limit reading to approx 1.3MB/sec
  --slowest: limit reading to approx 130KB/sec
  --tar: read a tar file from stdin instead of scanning
  --tar-stdout: relay tar file to stdout
  --verbose: print extra information
Examples:
To build digests: "$ bitrotchecker --progress  /tmp/md5s.txt /home/myhome"
To update digests: "$ bitrotchecker /tmp/md5s.txt /home/myhome"
To verify old files: "$ bitrotchecker --nothingnew /tmp/md5s.txt /home/myhome"
To verify old files, with md5sum: "$ cd /home/myhome ; md5sum -c /tmp/md5s.txt"
```

### --dry-run
This will not change any files, in particular it won't write checksums to the checksumfile.

This is not very useful, except for debugging.

This will still scan files and compute checksums.

If you want to only scan files with existing checksums, using "--nothingnew" instead
can be more efficient as "--dry-run" would still scan new files.

### --follow
This follows symlinks when scanning the directory. By default, symlinks are ignored (unless it is the directory on the command line).

If you're going to scan an entire filesystem anyway, symlinks would add redundant reading.

Note that this is _not_ supported when reading tar files. Symlinks in tar files will be ignored.

### --nothingnew
This instructs the scanner to ignore files that aren't already present in the checksumfile.

### --nottoday
This instructs the scanner to ignore files that have modification times within 24 hours.

This allows you to skip files that frequently change, like logs. If files are actively changing, then
this tool can't effectively track bitrot. See btrfs, zfs and dm-integrity as alternatives.

### --one-file-system
This instructs the directory scanner to not cross filesystems.

This can be useful if you scan / and want to avoid /proc.

For further control of what files to include and exclude, you can use tar's options and use bitrotchecker
with the "--tar" mode.

### --progress
This prints scanning progress to the console.

If the scanning is quick, you might not see anything.

You can use "--verbose" to print more information.

### --savechanges
This updates changed md5 checksums in the checksumfile for files that might be corrupted.

An alternative to running this command is to delete the corresponding line in the checksumfile.
With the line missing, there is no old md5 value to match, so the new md5 value will be saved.
This alternative is safer (see the warning below).

Normally when a file's data changes, its modification time is also updated. If bitrotchecker
sees data changing without a corresponding modification time change, it will normally print
a warning but it won't change the checksum value in the checksumfile. This is done to preserve
the old checksum so backups can be restored and verified.

There are exceptions to this general rule. For example, if you have files restored from backups,
the data changes and the modification time is also restored to an old value. This looks the
same as data corruption to bitrotchecker.

There is also a possibility of false positives when files are changed after
they are scanned, but before the scan is complete and the checksumfile is
written. From the scanner's point of view, it will look like the change
happened after the last scan. This is unfortunate but can be mitigated with the
"--nottoday" flag.

It's better to have the possibility of false positives than false negatives.

**Warning:** If any files have been newly corrupted right before running this command, their old
md5 values will be **lost**, since --savechanges will write the new value over it. To protect from
that, you can save a backup of the checksumfile before running with this option.

### --slow
This throttles read speed. This affects both directory scanning and tar reading.

This keeps the IO read speed from exceeding 13MB per second but it could
in practice go much slower.

See also --slower and --slowest.

### --slower
This throttles read speed. This affects both directory scanning and tar reading.

This keeps the IO read speed from exceeding 1.3MB per second but it could
in practice go much slower.

See also --slow and --slowest.

### --slowest
This throttles read speed. This affects both directory scanning and tar reading.

This keeps the IO read speed from exceeding 130KB per second but it could
in practice go much slower.

See also --slow and --slower.

### --tar
This switches from directory scanning to reading tar data.

See also "--tar-stdout" to relay the tar data.

Directory scanning is very efficient, using memory-mapped IO. However,
there are two advantages to using the tar mode: tar's inclusion options
and computing checksums during a backup.

There are many options to control inclusion with tar. You can generate
a list of files or you can use the --exclude options. See the tar man
page for more about that.

Computing checksums during a backup can eliminate the disk IO needed
by the directory scanning mode. If you
already backup your files with tar, you can piggy-back on tar's reading by
using this --tar option with --tar-stdout.

Example:
```bash
tar -cf - . | ./bitrotchecker --tar --tar-stdout --progress /tmp/md5s.txt | gzip > /mnt/mybackup.tgz
```

Some versions of tar are not supported. It should work with modern GNU Tar
(with ././@LongLink) and POSIX 1003.1-1988 (aka ustar) formats. It should be
fairly easy to add support for other formats.

Note that bitrotchecker reads the uncompressed tar data but it is compressed
afterward.

bitrotchecker will not touch the tar data itself; it acts as a pass-through, looking
at filenames and computing checksums on the data.

### --tar-stdout
This will relay tar data to stdout when --tar is active.

Without this option, tar data will be consumed from stdin and not saved. With this option,
tar data will be relayed from stdin to stdout.

If you use this option **be** **sure** to redirect stdout! You'll spam your console with
tar data if you don't redirect stdout. E.g., the command
"tar -cf - . | bitrotchecker --tar --tar-stdout /tmp/md5s.txt" will flood your console with tar data.

### --verbose
This will print a lot more information about its operation.

If you are watching the output, you might also want "--progress".
