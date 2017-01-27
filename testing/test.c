#include <stdio.h>
#include <stdlib.h>
#include <mcrypt.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
  MCRYPT td;
  int i;
  char *key;
  char password[20];
  char block_buffer;
  char *IV;
  int keysize = 16;

  //create key
  key = calloc(1, keysize);
  strcpy(password, "A_large_key");///////////this should be from a file
  memmove( key, password, strlen(password));

  //open encrypt
  td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  if (td == MCRYPT_FAILED) exit(1);

  //set IV
  IV = malloc(mcrypt_enc_get_iv_size(td));

  for( i = 0; i < mcrypt_enc_get_iv_size(td); i++)
    {
      IV[i] = 'x';
    }

  //initialize the encrypt
  i = mcrypt_generic_init(td, key, keysize, IV);
  if (i<0)
    {
      mcrypt_perror(i);
      exit(1);
    }

  //encrypt and write
  while( read(STDIN_FILENO, &block_buffer, 1) == 1)
    {
      mcrypt_generic(td, &block_buffer, 1);
      //mdecrypt_generic(td, &block_buffer, 1);
      write(STDOUT_FILENO, &block_buffer, 1);
    }

  mcrypt_generic_end(td);

  return 0;
}
