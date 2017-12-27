# Project Description

Implementation of a file system that will navigate the provided Turkish postal codes data. The file system will analyze a CSV  file that contains this data and display a hierarchy of the neighborhoods based on the location names and codes.

The top-level directory will contain two-directories: NAMES and CODES. The NAMES directory will display a subdirectory for every city. A city directory will display a subdirectory for every district. A district directory will display a text  le for every neighborhood.

# Usage

To compile: gcc rofs.c -o rofs -Wall -ansi -W -std=c99 -g -ggdb -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -lfuse
For mounting: ./rofs myfs/
For unmounting: fusermount -u myfs

Mounting file system to "myfs" folder makes it is possible to create all the files and folders directly under myfs directory with using a hierarchy. In other words myfs become root folder of our file system.