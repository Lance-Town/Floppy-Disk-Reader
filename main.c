/*
 * Lance Townsend
 *
 * Program to read in a print the directories that are on a
 * specified disk image. It will also print out information about
 * the files on the disk, print out ASCII files on the disk. The
 * directory information will also include any deleted files still
 * on the disk.
 *
 * USAGE: ./main <DISK_IMAGE>
 *
 */

/* ----------------------- System Includes ---------------------- */
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* ------------------------- Structures ------------------------- */

// Boot sector structure
typedef struct {
    unsigned short bpb; // bytes per block
    unsigned char bpc; // bytes per cluster
    unsigned short rb; // reserved blocks
    unsigned char nFAT; // number of FATs
    unsigned short nerd; // number of root directories
    unsigned short nlb; // number of logical blocks
    unsigned char md; // media descriptor
    unsigned short bpf; // number of blocks per FAT
    unsigned short sectors; // sectors per track
    unsigned short surfaces; // number of heads
    unsigned short hb; // number of hidden blocks
} BootBlock;

// Directory entry structure
typedef struct {
   unsigned char name[8];
   char ext[3];
   unsigned char attribute;
   unsigned char reserved[10];
   unsigned short time;
   unsigned short date;
   unsigned short blockNum;
   unsigned int size;
}__attribute__((__packed__)) DirEntry;

/* --------------------- Function Prototypes -------------------- */

// read in the boot block
BootBlock readBootBlock(int fd);

// print out the boot block
void printBootBlock(BootBlock bb);

// read in the FAT12 tables
int readFAT12Table(int fd, BootBlock bb, unsigned short **table);

// strip the whitespace at the end of a string
int stripWhitespace(char** s, int len);

// print entire ASCII file
void printAsciiFile(int fd, BootBlock bb, unsigned short startingCluster, unsigned short *FAT);

// get next cluster number from FAT12 table
unsigned short getNextCluster(unsigned short cluster, unsigned short *FAT);

// read a cluster into a buffer
void readCluster(int fd, BootBlock bb, unsigned short clusterNumber, unsigned char *buffer);

// read subdirectories
void readDirectory(int fd, BootBlock bb, unsigned short startCluster, unsigned short *table, int isRoot, int tabs);

// print out a specified number of tabs for nice formatting
void printTabs(int tab);

// prints out the info about a directory
void printInfo(unsigned char attr, unsigned short time, unsigned short date, unsigned int size,unsigned short CN, int tabs);

/* -------------------- Function Definitions -------------------- */

int main(int argc, char** argv) {
   char diskName[64]; // holds disk image file name
   int fd; // UNIX file descriptor
   unsigned short *FAT12Table; // array to hold the all the FAT12s

   if (argc < 2) {
      printf("Enter Disk Image Name: ");
      scanf("%s", diskName);
   } else {
      strcpy(diskName, argv[1]);
   }

   fd = open(diskName, O_RDONLY);

   if (fd <= 0) {
      printf("ERROR: Could not open file. Terminating program...\n");
      exit(1);
   }

   // initiliaze the boot block
   BootBlock bb = readBootBlock(fd);

   // print out the boot block
   printf("Boot Block:\n");
   printBootBlock(bb);

   readFAT12Table(fd, bb, &FAT12Table);

   printf("\n\nPrinting Directories: \n");
   readDirectory(fd, bb, 0, FAT12Table, 1, 0);

   free(FAT12Table);
   return 0;
}

// read in the boot block
BootBlock readBootBlock(int fd) {
   BootBlock bb;
   unsigned char bba[32]; // temporarily hold all bytes of the boot block

   lseek(fd, 0, SEEK_SET);
   read(fd, &bba, 32);

   bb.bpb = ((unsigned short) bba[0xc] << 8) | bba[0xb];
   bb.bpc = bba[0xd];
   bb.rb = ( (unsigned short) bba[0xf] << 8) | bba[0xe];
   bb.nFAT = bba[0x10];
   bb.nerd = ( (unsigned short) bba[0x12] << 8) | bba[0x11];
   bb.nlb = ( (unsigned short) bba[0x14] << 8) | bba[0x13];
   bb.md = bba[0x15];
   bb.bpf = ( (unsigned short) bba[0x17] << 8) | bba[0x16];
   bb.sectors = ( (unsigned short) bba[0x19] << 8) | bba[0x18];
   bb.surfaces = ( (unsigned short) bba[0x1b] << 8) | bba[0x1a];
   bb.hb = ( (unsigned short) bba[0x1d] << 8) | bba[0x1c];
   return bb;
}

// print out the contents of the boot block
void printBootBlock(BootBlock bb) {
   printf("bpb: %hu\nbpc: %u\nrb: %hu\nnFAT: %u\n", bb.bpb, bb.bpc, bb.rb, bb.nFAT);
   printf("nerd: %hu\nnlb: %hu\nmd: %u\nbpf: %hu\n", bb.nerd, bb.nlb, bb.md, bb.bpf);
   printf("sectors: %hu\nsurfaces: %hu\nhb: %hu\n", bb.sectors, bb.surfaces, bb.hb);
}

// read in the FAT12 tables
int readFAT12Table(int fd, BootBlock bb, unsigned short **table) {
   // get start and size of fat table
   int fatStart = (bb.hb + bb.rb) * bb.bpb;
   int fatSize = bb.bpb * bb.bpf;

   unsigned char *fatBuffer = (unsigned char*) malloc(fatSize);
   if (fatBuffer == NULL) {
      printf("ERROR: Memory allocation for FAT12 table failed.\n");
      exit(1);
   }

   // go to start of fat table
   lseek(fd, fatStart, SEEK_SET);

   // read the FAT12 table into the buffer
   read(fd, fatBuffer, fatSize);

   // alocate memory for table
   *table = (unsigned short*) malloc(((fatSize * 2) / 3) * sizeof(unsigned short));

   // get cluster numbers from fat12
   for (int i = 0, j = 0; i < fatSize; i += 3) {
      (*table)[j++] = ((unsigned short)(fatBuffer[i + 1] & 0x0F) << 8) | fatBuffer[i];

      if (i + 2 < fatSize) {
         (*table)[j++] = ((unsigned short)fatBuffer[i + 2] << 4) | ((fatBuffer[i + 1] & 0xF0) >> 4);
      }
   }

   free(fatBuffer);

   return fatSize;
}

// strip the whitespace at the end of a string
int stripWhitespace(char** s, int len) {
   while ( isspace((*s)[len]) ) {
      (*s)[len] = '\0';
      len--;
   }

   return len+1;
}

// print out a specified number of tabs for nice formatting
void printTabs(int tab) {
   if (tab != 0) {
      printf("%*c", (tab)*3, ' ');
   }
}

// prints out the info on a directory entry
void printInfo(unsigned char attr, unsigned short time, unsigned short date, unsigned int size,unsigned short CN, int tabs) {
   // print a new line and more spacing
   printf("\n");
   printTabs(tabs);

   // get attributes
   /*
   unsigned char ro = attr & 0x01;
   unsigned char hidden = (attr & 0x02) >> 1;
   unsigned char system = (attr & 0x04) >> 2;
   unsigned char volLabel = (attr & 0x08) >> 3;
   unsigned char subDir = (attr & 0x10) >> 4;
   unsigned char archive = (attr & 0x20) >> 5;
   */

   // get day month year
   unsigned short day = date & 0x1F;
   date >>= 5;
   unsigned short month = date & 0xF;
   date >>= 4;
   unsigned short year = (date & 0x7F) + 1980;

   // get hour minute second
   unsigned short second = (time & 0x1F) * 2;
   time >>= 5;
   unsigned short minute = time & 0x3F;
   time >>= 6;
   unsigned short hour = time & 0x1F;

   //printf("%u:%u:%u:%u:%u:%u\t", ro, hidden, system, volLabel, subDir, archive);

   printf("Date: %02hu\\%02hu\\%04hu\n", month, day, year);
   printTabs(tabs);
   printf("Time: %02hu:%02hu:%02hu\n", hour, minute, second);
   printTabs(tabs);
   printf("Size: %d\n", size);
   printTabs(tabs);
   printf("Cluster Number: %d\n", CN);
   printTabs(tabs);
   printf("Attribute: %hu\n", attr);
   printf("\n");


   return;
}

// read a cluster into a buffer
void readCluster(int fd, BootBlock bb, unsigned short clusterNumber, unsigned char *buffer) {
   // get cluster size, data area start, and cluster offset
   int clusterSize = bb.bpc * bb.bpb;
   int dataAreaStart = (bb.rb + (bb.nFAT * bb.bpf) + ((bb.nerd * sizeof(DirEntry)) / bb.bpb)) * bb.bpb;
   int clusterOffset = dataAreaStart + (clusterNumber - 2) * clusterSize;

   // go to start of cluster
   lseek(fd, clusterOffset, SEEK_SET);

   // read the cluster data into the buffer
   read(fd, buffer, clusterSize);

   return;
}

// read subdirectories
void readDirectory(int fd, BootBlock bb, unsigned short startCluster, unsigned short *table, int isRoot, int tabs) {
   int dirEntriesPerBlock = bb.bpb / 32 ;
   int blockNum, rootOffset, nRoot;

   if (isRoot) {
      // get number of blocks in root dir and the root offset
      nRoot = (bb.nerd * 32 + bb.bpb - 1) / bb.bpb;
      rootOffset = (bb.hb + bb.rb + (bb.nFAT * bb.bpf)) * bb.bpb;
      blockNum = nRoot;
   } else {
      // in subdirectory - get start of data area and its offset
      int dataAreaStart = (bb.rb + bb.nFAT * bb.bpf + bb.nerd / dirEntriesPerBlock) * bb.bpb;
      rootOffset = dataAreaStart + (startCluster - 2) * bb.bpc * bb.bpb;
      blockNum = 1;
   }

   // create a directory entry and allocate space for entry names and extentions
   DirEntry entry;
   int longNameIndex = 0;
   char *name = (char*) malloc(sizeof(char) * 256);
   memset(name, 0, 256);
   char *ext = (char*) malloc(sizeof(char) * 4);
   int extLen = 0;

   for (int i = 0; i < blockNum * dirEntriesPerBlock; i++) {
      lseek(fd, rootOffset + (i * 32), SEEK_SET);
      int bytesRead = read(fd, &entry, 32);

      if (bytesRead < sizeof(DirEntry)) {
         printf("Error Reading directory entry\n");
         break;
      }

      if (entry.name[0] == 0x00) break; // end of directory entries
      if (entry.size == -1 || entry.attribute == 0x0f) continue; // skip weird files

      if (longNameIndex <= 0) {
         strncpy(name, entry.name, 8);
         name[8] = '\0';
         stripWhitespace(&name, 7);  // strip trailing whitespace
      }

      if (entry.attribute & 0x10) { // check if it's a directory
         if (!isRoot && (name[0] == '.')) continue; // skip '.' and '..' entries in subdirectories

         // print out 3 spaces to indicate files within subdirectories
         printTabs(tabs);

         printf("Directory: %8s\t", name);
         printInfo(entry.attribute, entry.time, entry.date, entry.size, entry.blockNum, tabs);
         // recursively read the subdirectory
         readDirectory(fd, bb, entry.blockNum, table, 0, tabs+1);
      } else {
         // copy and format extention
         strncpy(ext, entry.ext, 3);
         ext[3] = '\0';
         extLen = stripWhitespace(&ext, 2); // strip trailing whitespace

         // print out 3 spaces to indicate files within subdirectories
         printTabs(tabs);


         // print file name and optional extention
         printf("File: %8s", name);
         if (extLen > 0) {
            printf(".%-3s\t", ext);
         } else {
            printf("\t");
         }

         // print rest of entry info
         printInfo(entry.attribute, entry.time, entry.date, entry.size, entry.blockNum, tabs);

         // print out readme file (only wanting to output readme files)
         if (strncmp(name, "README", 6) == 0) {
            printf("\nPRINTING: %s.%s\n", name, ext);
            printAsciiFile(fd, bb, entry.blockNum, table);
            printf("\n");
         }
      }

      if (longNameIndex > 0) {
         longNameIndex = 0;
         name[0] = '\0';
      }
   }

   free(name);
   free(ext);
}


// get next cluster number from FAT12 table
unsigned short getNextCluster(unsigned short cluster, unsigned short *FAT) {
   unsigned short fatEntryValue = FAT[cluster];
   if (fatEntryValue >= 0xFF8) { // check for end-of-chain marker in FAT12
      return 0;
   }
   return fatEntryValue;
}

// print entire ASCII file
void printAsciiFile(int fd, BootBlock bb, unsigned short startingCluster, unsigned short *FAT) {
   int clusterSize = bb.bpc * bb.bpb;
   unsigned char *buffer = malloc(clusterSize);

   unsigned short currentCluster = startingCluster;
   bool isFileAscii = true;

   while (currentCluster) {
      // get the offset for the current cluster
      int clusterOffset = ((bb.rb + bb.nFAT * bb.bpf + bb.nerd / (bb.bpb / 32)) * bb.bpb) + (currentCluster - 2) * clusterSize;
      lseek(fd, clusterOffset, SEEK_SET);
      read(fd, buffer, clusterSize);

      if (isFileAscii) {
         // print the clusters content

         for (int i = 0; i < clusterSize; i++) {
            if (buffer[i] == 0) break; // stop printing at the first null byte
            putchar(buffer[i]);
         }
      }

      // get the next cluster number from the FAT
      currentCluster = getNextCluster(currentCluster, FAT);
   }

   free(buffer);

   return;
}
