# UFS2 File System Explorer 
This C program operates on raw images of disk partitions, created using the `dd` tool after unmounting the partition:
```sh
dd if=/dev/[NAME_OF_PARTITION] of=disk.img bs=4k
```
This command creates a raw disk image called `disk.img`. Using `bs=4k` ensures faster reading and writing by processing data in 4k chunks. Instructions for adding virtual disk images, creating partitions, and mounting partitions can be found in the <a href="https://docs.freebsd.org/en/books/handbook/disks/">FreeBSD Handbook - Disks</a>.

## Program Description
The program uses UFS2 (Unix File System) structures, including the <b><i>superblock</i></b>, <b><i>inode</i></b> structures, and <b><i>direct</i></b> structures, to read a raw disk image and hierarchically list all the files and folders within it.

##### Usage:
```sh
./fs-find [PATH TO RAW IMAGE OF DISK PARTITION]
```
