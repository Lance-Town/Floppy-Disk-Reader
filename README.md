# DOS Floppy Disk Reader

## Description

Program to read in and print the directories that are on a specified DOS floppy disk image. 

It will also print out information about the files on the disk, print out ASCII files on the disk. The directory information will also include any deleted files still on the disk.

## Usage

1. Compile the server and client using the provided Makefile:
    ```bash
    make
    ```

2. Run the server in one terminal:
    ```bash
    ./main <floppy_disk_img>
    ```

## Example

If you have a floppy image with one directory inside, and in that one directory is a txt file that contains "Hello, World!" the program will output:

```bash
Boot Block:
bpb: 512
bpc: 1
rb: 1
nFAT: 2
nerd: 224
nlb: 2880
md: 240
bpf: 9
sectors: 18
surfaces: 2
hb: 0

Printing Directories:
Directory: MYDIR
Date: 06\22\2024
Time: 14:01:36
Size: 0
Cluster Number: 759
Attribute: 16

   File: README.TXT
   Date: 06\22\2024
   Time: 14:02:42
   Size: 14
   Cluster Number: 761
   Attribute: 32

   PRINTING: README.txt
   Hello, World!

```

## Makefile Commands

- `all`: Builds both the server and client.
- `clean`: Removes object files.
