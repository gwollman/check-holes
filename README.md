# check-holes
Scan a set of files for holes

Many filesystem implementations allow for files to contain "holes"
-- that is, regions of zeros which are not stored on disk but filled
in on-demand when read or written to.  An extension to the POSIX
lseek() interface allows applications like archivers which want to
know about the holes (so they too can avoid storing them) to iterate
through the file and enumerate each hole.

This utility exercises the API and prints out a list of all the holes
in one or more files, by beginning and ending offset, along with a
summary of the total number of holes and how many bytes of the file
size they account for.  It accepts a single flag, -q, which suppresses
the output of individual holes and also suppresses the summary if no
holes are found.

This code has only been compiled or tested on FreeBSD.
