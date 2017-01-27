/*****************************************************

WARNING

DO NOT COPY.

PROJECT 3 IS SUBJECT TO PLAGARISM CHECKING

VIEW AT OWN DISCRETION

*****************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<math.h>

//USE INT
int i;

//GLOBALS
int NINODES = 0;
int NBLOCKS = 0;
int FRAGSIZE = 0;
int BLOCKSIZE = 0;
int INODESIZE = 0;
int BLOCKS_GROUP = 0;
int INODES_GROUP = 0;
int FRAGMENTS_GROUP = 0;
int FIRST_BLOCK = 0;
int NGROUPS = 0;

//type conversions
int shorttohex(short* arg, char* buf)
{
  return sprintf(buf,"%x", *arg & 0xFFFF);
}
int shorttooct(short* arg, char*buf)
{
  return sprintf(buf, "%o", *arg & 0xFFFF);
}
int inttohex(int* arg, char* buf)
{
  return sprintf(buf, "%x", *arg);
}
int inttochar(int* arg, char*buf)
{
  return sprintf(buf,"%d", *arg);
}
int shorttochar(short* arg, char*buf)
{
  return sprintf(buf,"%u", *arg & 0xFFFF);
}
int longlongtochar(long long int* arg, char* buf)
{
  return sprintf(buf,"%llu", *arg);
}
int short8tochar(short* arg, char* buf)
{
  return sprintf(buf, "%u", *arg & 0x00FF);
}


//write plus comma
void write_comma(int writefd, char* buf, int len)
{
  write(writefd, buf, len);
  write(writefd, ",", 1);
}
//write plus newline
void write_newline(int writefd, char* buf, int len)
{
  write(writefd, buf, len);
  write(writefd, "\n", 1);
}


//reads from readfd, prints to writefd change amount to value
void printint(int readfd, int writefd, int offset, char* buf, int* amount)
{
  pread(readfd, amount, 4, offset);
  int len = inttochar(amount, buf);
  write_comma(writefd, buf, len); 
}
void printshort(int readfd, int writefd, int offset, char* buf, short* amount)
{
  pread(readfd, amount, 2, offset);
  int len = shorttochar(amount, buf);
  write_comma(writefd, buf, len); 
}
void print2hex(int readfd, int writefd, int offset, char*buf, short* amount)
{
  pread(readfd, amount, 2, offset);
  int len = shorttohex(amount, buf);
  write_comma(writefd, buf, len);
}
void print4hex(int readfd, int writefd, int offset, char*buf, int* amount)
{
  pread(readfd, amount, 4, offset);
  int len = inttohex(amount, buf);
  write_comma(writefd, buf, len);
}
void print2oct(int readfd, int writefd, int offset, char* buf, short* amount)
{
  pread(readfd, amount, 2, offset);
  int len = shorttooct(amount, buf);
  write_comma(writefd, buf, len);
}
void print8bit(int readfd, int writefd, int offset, char* buf, short* amount)
{
  pread(readfd, amount, 1, offset);
  int len = short8tochar(amount, buf);
  write_comma(writefd, buf, len);
}
		       


void super_csv(int fd, char* buffer);
void group_csv(int fd, char* buffer);
void inode_csv(int fd, int inodefd, char* buffer, int offset, int id, int directoryfd, int indirectfd);
void choosetype( char* buffer, short* sh);
void traverse_block(int blockptr,int fd, int id, char* buffer, int directoryfd, int* entry);
void traverse_indirect(int blockptr, int fd, int id, char* buffer, int directoryfd, int* entry, int indirectfd);
void traverse_indirect2(int blockptr, int fd, int id, char* buffer, int directoryfd, int* entry, int indirectfd);
void traverse_indirect3(int blockptr, int fd, int id, char* buffer, int directoryfd, int* entry, int indirectfd);
int main(int argc, char* argv[])
{
  //check argument
  if( argc != 2){
    printf("Error: program only accepts one argument!");
    exit(1);
  }

  //open file system
  int fd =  open(argv[1], O_RDONLY);
  if (fd == -1){
    printf("Error: failed to open file");
    exit(1);
  }

  //create buffer
  char* buffer = malloc(30);
  
  //create super.csv
  super_csv(fd, buffer);

  //create group.csv
  group_csv(fd, buffer);


  
  return 0;
}


void super_csv(int fd, char* buffer)
{
  int superfd;
  superfd = open("super.csv", O_CREAT | O_TRUNC | O_WRONLY, 0600);
  
  //read magic number
  short short_val;
  print2hex(fd, superfd, 1024+56, buffer, &short_val); 
  
  //read number of inodes
  printint(fd, superfd, 1024, buffer, &NINODES);
  
  //read number of blocks
  printint(fd, superfd, 1024+4, buffer, &NBLOCKS);

  //block size
  pread(fd, &BLOCKSIZE, 4, 1024+24);
  BLOCKSIZE = 1024 << BLOCKSIZE;
  i = inttochar(&BLOCKSIZE, buffer);
  write_comma(superfd, buffer, i);
  
  //read frag size
  pread(fd, &FRAGSIZE, 4, 1024+28);
  if(FRAGSIZE > 0)
    FRAGSIZE = 1024 << FRAGSIZE;
  else
    FRAGSIZE = 1024 >> -FRAGSIZE;
  i = inttochar(&FRAGSIZE, buffer);
  write_comma(superfd, buffer, i);

  //blocks per group
  printint(fd, superfd, 1024+32, buffer, &BLOCKS_GROUP);

  //inodes per group
  printint(fd, superfd, 1024+40, buffer, &INODES_GROUP);

  //fragments per group
  printint(fd, superfd, 1024+36, buffer, &FRAGMENTS_GROUP);

  //first data block
  pread(fd, &FIRST_BLOCK, 4, 1024+20);
  i = inttochar(&FIRST_BLOCK, buffer);
  write_newline(superfd, buffer, i);

  //size of inode
  pread(fd, &INODESIZE, 2, 1024+88);

  double blocks = NBLOCKS;
  double blocks_group = BLOCKS_GROUP;
  
  NGROUPS = ceil(blocks/blocks_group);
  
  close(superfd);
}

void group_csv(int fd, char* buffer)
{
  int groupfd = open("group.csv", O_CREAT | O_TRUNC | O_WRONLY, 0600);
  int bitmapfd = open("bitmap.csv", O_CREAT | O_TRUNC | O_WRONLY, 0600);
  int inodefd = open("inode.csv", O_CREAT | O_TRUNC | O_WRONLY, 0600);
  int directoryfd = open("directory.csv", O_CREAT | O_TRUNC | O_WRONLY, 0600);
  int indirectfd = open("indirect.csv", O_CREAT | O_TRUNC | O_WRONLY, 0600);
  for(i = 0; i < NGROUPS; i++)
    {
      //reusable variables
      int offset;
      short short_val;
      int int_val;
      
      //number of blocks
      int nblocks = BLOCKS_GROUP;
      if (i == NGROUPS-1 && NBLOCKS % BLOCKS_GROUP != 0)
	nblocks = NBLOCKS % BLOCKS_GROUP;
      int len = inttochar(&nblocks, buffer);
      write_comma(groupfd, buffer, len);

      //number of inodes
      int ninodes = INODES_GROUP;
      if (i == NGROUPS-1 && NINODES % INODES_GROUP != 0)
	ninodes = NINODES % INODES_GROUP;
      
      //number of free blocks
      offset = (BLOCKSIZE*(FIRST_BLOCK+1)+(32*i))+12;
      printshort(fd, groupfd, offset, buffer, &short_val);

      //number of free inodes
      offset = (BLOCKSIZE*(FIRST_BLOCK+1)+(32*i))+14;
      printshort(fd, groupfd, offset, buffer, &short_val);

      //number of directories
      offset = (BLOCKSIZE*(FIRST_BLOCK+1)+(32*i))+16;
      printshort(fd, groupfd, offset, buffer, &short_val);

      //inode bitmap block
      int i_bitmap;
      offset = (BLOCKSIZE*(FIRST_BLOCK+1)+(32*i))+4;
      print4hex(fd, groupfd, offset, buffer, &i_bitmap);
      

      //block bitmap block
      int b_bitmap;
      offset = (BLOCKSIZE*(FIRST_BLOCK+1)+(32*i));
      print4hex(fd, groupfd, offset, buffer, &b_bitmap);

      //fill block bitmap
      int j;
      int bytecount = 0;
      char byte;
      for(j = 0; j < nblocks; j++)
	{
	  if(j % 8 == 0)
	    {
	      offset = (b_bitmap*BLOCKSIZE)+bytecount;
	      bytecount++;
	      pread(fd, &byte, 1, offset);
	    }
	  if((byte & 0x01) == 0)
	    {
	      len = inttohex(&b_bitmap, buffer);
	      write_comma(bitmapfd, buffer, len);
	      int_val = (i*BLOCKS_GROUP)+j+1;
	      len = inttochar(&int_val, buffer);
	      write_newline(bitmapfd, buffer, len);
	    }
	  byte = byte >> 1;
	}

      //inode table start block
      int inode_table;
      offset = (BLOCKSIZE*(FIRST_BLOCK+1)+(32*i))+8;
      pread(fd, &inode_table, 4, offset);
      len = inttohex(&inode_table, buffer);
      write_newline(groupfd, buffer, len);
      
      //fill free inode bitmap
      //fill allocated inode info
      bytecount = 0;
      for(j = 0; j < ninodes; j++)
	{
	  if(j % 8 == 0)
	    {
	      offset = (i_bitmap*BLOCKSIZE)+bytecount;
	      bytecount++;
	      pread(fd, &byte, 1, offset);
	    }
	  if((byte & 0x01) == 0)
	    {
	      len = inttohex(&i_bitmap, buffer);
	      write_comma(bitmapfd, buffer, len);
	      int_val = (i*INODES_GROUP)+j+1;
	      len = inttochar(&int_val, buffer);
	      write_newline(bitmapfd, buffer, len);
	    }
	  else
	    {
	      int_val = ((i*INODES_GROUP)+j+1);
	      offset = (inode_table*BLOCKSIZE)+(INODESIZE*(j));
	      inode_csv(fd, inodefd, buffer, offset,int_val, directoryfd, indirectfd);
	    }
	  byte = byte >> 1;
	}
      
      
      
    }
  close(indirectfd);
  close(directoryfd);
  close(inodefd);
  close(bitmapfd);
  close(groupfd);
}


void inode_csv(int fd, int inodefd, char* buffer, int offset, int id, int directoryfd, int indirectfd)
{
  int len;
  int int_val;
  short short_val;
  //inode number
  len = inttochar(&id, buffer);
  write_comma(inodefd, buffer, len);

  //file type
  pread(fd, &short_val, 2, offset);
  choosetype(buffer, &short_val);
  write_comma(inodefd, buffer, 1);
  char type = *buffer;

  //mode
  print2oct(fd, inodefd, offset, buffer, &short_val);

  //owner ID
  printshort(fd, inodefd, offset+2, buffer, &short_val);

  //group ID
  printshort(fd, inodefd, offset+24, buffer, &short_val);

  //link count
  printshort(fd, inodefd, offset+26, buffer, &short_val);

  //creation time
  print4hex(fd, inodefd, offset+12, buffer, &int_val);

  //mod time
  print4hex(fd, inodefd, offset+16, buffer, &int_val);

  //access time
  print4hex(fd, inodefd, offset+8, buffer, &int_val);

  //file size
  pread(fd, &int_val, 4, offset+4);
  long long int upper;
  pread(fd, &upper, 4, offset+108);
  long long int filesize = upper << 32 | int_val;
  len = longlongtochar(&filesize, buffer);
  write_comma(inodefd, buffer, len);

  //number of blocks
  int nblocks;
  pread(fd, &nblocks, 4, offset+28);
  nblocks = nblocks/(BLOCKSIZE/512);
  len = inttochar(&nblocks, buffer);
  write_comma(inodefd, buffer, len);
  
  //block pointers
  //directories
  int blockptr[15];
  int i;
  int entry = 0;
  for(i = 0;i < 12; i++)
    {
      print4hex(fd, inodefd, offset+40+(i*4), buffer, &blockptr[i]);
      if (type == 'd' && blockptr[i] != 0)
	{
	  traverse_block(blockptr[i],fd, id, buffer, directoryfd, &entry);
	}
    }

  //indirect
  print4hex(fd, inodefd, offset+40+(12*4), buffer, &blockptr[12]);
  if(nblocks > 12)
    {
      traverse_indirect(blockptr[12],fd, id, buffer, directoryfd, &entry, indirectfd);
    }
    
  
  //double indirect
  print4hex(fd, inodefd, offset+40+(13*4), buffer, &blockptr[13]);
  if(nblocks > 13)
    {
      traverse_indirect2(blockptr[13],fd,id,buffer,directoryfd,&entry,indirectfd);
    }
  
  //triple indirect
  pread(fd, &blockptr[14], 4, offset+40+(14*4));
  len = inttohex(&blockptr[14], buffer);
  write_newline(inodefd, buffer, len);
  if(nblocks > 14)
    {
      traverse_indirect3(blockptr[14],fd,id,buffer,directoryfd,&entry,indirectfd);
    }
  


 
}

void traverse_indirect3(int blockptr, int fd, int id, char* buffer, int directoryfd, int* entry, int indirectfd)
{
  int blockentry = 0;
  int len;
  int i;
  int iterations = BLOCKSIZE/4;
  int offset;
  int indir_block;
  for (i = 0; i < iterations; i++)
    {
      offset = (blockptr*BLOCKSIZE)+(i*4);
      pread(fd, &indir_block, 4, offset);

      if(indir_block == 0)
	{
	  blockentry++;
	  continue;
	}

      //containing block number
      len = inttohex(&blockptr, buffer);
      write_comma(indirectfd, buffer, len);

      //entry number
      len = inttochar(&blockentry, buffer);
      write_comma(indirectfd, buffer, len);
      blockentry++;

      //block pointer value
      len = inttohex(&indir_block, buffer);
      write_newline(indirectfd, buffer, len);
    }
  
  for (i = 0; i < iterations; i++)
    {
      offset = (blockptr*BLOCKSIZE)+(i*4);
      pread(fd, &indir_block, 4, offset);

      if(indir_block == 0) continue;

      traverse_indirect2(indir_block, fd, id, buffer, directoryfd, entry, indirectfd);
    }
}

void traverse_indirect2(int blockptr, int fd, int id, char* buffer, int directoryfd, int* entry, int indirectfd)
{
  int blockentry = 0;
  int len;
  int i;
  int iterations = BLOCKSIZE/4;
  int offset;
  int indir_block;
  for (i = 0; i < iterations; i++)
    {
      offset = (blockptr*BLOCKSIZE)+(i*4);
      pread(fd, &indir_block, 4, offset);

      if(indir_block == 0)
	{
	  blockentry++;
	  continue;
	}

      //containing block number
      len = inttohex(&blockptr, buffer);
      write_comma(indirectfd, buffer, len);

      //entry number
      len = inttochar(&blockentry, buffer);
      write_comma(indirectfd, buffer, len);
      blockentry++;

      //block pointer value
      len = inttohex(&indir_block, buffer);
      write_newline(indirectfd, buffer, len);
    }
  
  for (i = 0; i < iterations; i++)
    {
      offset = (blockptr*BLOCKSIZE)+(i*4);
      pread(fd, &indir_block, 4, offset);

      if(indir_block == 0) continue;

      traverse_indirect(indir_block, fd, id, buffer, directoryfd, entry, indirectfd);
    }
}

void traverse_indirect(int blockptr, int fd, int id, char* buffer, int directoryfd, int* entry, int indirectfd)
{
  int blockentry = 0;
  int len;
  int offset;
  int i;
  int indir_block;
  int iterations = BLOCKSIZE/4;
  for(i = 0; i < iterations; i++)
    {
      offset = (blockptr*BLOCKSIZE)+(i*4);
      pread(fd,&indir_block, 4, offset);

      if(indir_block == 0)
	{
	  blockentry++;
	  continue;
	}
      
      traverse_block(blockptr, fd, id, buffer, directoryfd, entry);

      //containing block number
      len = inttohex(&blockptr, buffer);
      write_comma(indirectfd, buffer, len);

      //entry number
      len = inttochar(&blockentry, buffer);
      write_comma(indirectfd, buffer, len);
      blockentry++;

      //block pointer value
      len = inttohex(&indir_block, buffer);
      write_newline(indirectfd, buffer, len);
      
    }
}



void traverse_block(int blockptr, int fd, int id, char* buffer, int directoryfd, int* entry)
{
  int len;
  int read = 0;
  short record_length = 0;
  short name_length = 0;
  int tempoffset = blockptr*BLOCKSIZE;
  int valid;
  
  while(read < BLOCKSIZE)
    {
      //check valid
      pread(fd, &valid, 4, tempoffset);
      pread(fd, &name_length, 1, tempoffset+6);
      
      
      if (valid != 0 && name_length != 0)
	{
	  //parent inode
	  len = inttochar(&id, buffer);
	  write_comma(directoryfd, buffer, len);
	  
	  //entry number
	  len = inttochar(entry, buffer);
	  write_comma(directoryfd, buffer, len);
	  
	  //entry length
	  printshort(fd, directoryfd, tempoffset+4, buffer, &record_length);
	  
	  //name length
	  print8bit(fd, directoryfd, tempoffset+6, buffer, &name_length);
	  
	  //inode number
	  len = inttochar(&valid, buffer);
	  write_comma(directoryfd, buffer, len);
	  
	  //string name
	  write(directoryfd, "\"", 1);
	  pread(fd, buffer, name_length, tempoffset+8);
	  write(directoryfd, buffer, name_length);
	  write(directoryfd, "\"\n", 2);
	}
      else
	{
	  pread(fd, &record_length, 2, tempoffset+4);
	} 
      //go to next entry
      tempoffset += record_length;
      read += record_length;
      (*entry)++;
    }
}  
  
  
  
  
  
void choosetype( char* buffer, short* sh)
{
  unsigned short value = *sh & 0xF000;
  switch(value)
    {
    case 40960:
      *buffer = 's';
      break;
    case 32768:
      *buffer = 'f';
      break;
    case 16384:
      *buffer = 'd';
      break;
    default:
      *buffer = '?';
      break;
    }
}
