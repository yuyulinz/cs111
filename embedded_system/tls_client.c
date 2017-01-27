/*	
 *	Demonstration TLS client
 *
 *       Compile with
 *
 *       gcc -Wall -o tls_client tls_client.c -L/usr/lib -lssl -lcrypto
 *
 *       Execute with
 *
 *       ./tls_client <server_INET_address> <port> <client message string>
 *
 *       Generate certificate with 
 *
 *       openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout tls_demonstration_cert.pem -out tls_demonstration_cert.pem
 *
 *	 Developed for Intel Edison IoT Curriculum by UCLA WHI
 */
#include "tls_header.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void *readfile();

int port, range, rate;
int server;
int receive_length, line_length;
char ip_addr[BUFSIZE];
char *my_ip_addr;
char *line = NULL;
double heart_rate;
FILE *fp = NULL;
SSL *ssl;
SSL_CTX *ctx;

int fd;
char *buff;

int main(int args, char *argv[])
{

  buff = malloc(1000*sizeof(char));
  fd = open("log.txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
  
  char buf[BUFSIZE];
  
  my_ip_addr = get_ip_addr();
  printf("My ip addr is: %s\n", my_ip_addr);
  sprintf(buff, "My ip addr is: %s\n", my_ip_addr);
  write(fd, buff, strlen(buff));
  
  /* READ INPUT FILE */
  fp = fopen("config_file", "r");

  
  if(fp == NULL){
    fprintf(stderr, "Error opening config file with name 'config_file'. Exiting.\n");
    sprintf(buff, "Error opening config file with name 'config_file'. Exiting.\n");
    write(fd, buff, strlen(buff));
    exit(1);
  }
  printf("Reading input file...\n");
  sprintf(buff, "Reading input file...\n");
  write(fd, buff, strlen(buff));
  while(getline(&line, &line_length, fp) > 0){
    if(strstr(line, "host_ip") != NULL){
      sscanf(line, "host_ip: %s\n", ip_addr);
    }
    else if(strstr(line, "port") != NULL){
      sscanf(line, "port: %d\n", &port);
    }
    else if(strstr(line, "range") != NULL){
      sscanf(line, "range: %d\n", &range);
    }
    else if(strstr(line, "rate") != NULL){
      sscanf(line, "rate: %d\n", &rate);
    }
    else{
      fprintf(stderr, "Unrecognized line found: %s. Ignoring.\n", line);
      sprintf(buff, "Unrecognized line found: %s. Ignoring.\n", line);
      write(fd, buff, strlen(buff));
    }
  }
  fclose(fp);
  /* FINISH READING INPUT FILE */
  
  printf("Connecting to: %s:%d\n", ip_addr, port);
  sprintf(buff, "Connecting to: %s:%d\n", ip_addr, port);
  write(fd, buff, strlen(buff));
  
  /* SET UP TLS COMMUNICATION */
  SSL_library_init();
  ctx = initialize_client_CTX();
  server = open_port(ip_addr, port);
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, server);
  /* FINISH SETUP OF TLS COMMUNICATION */
  
  /* SEND HEART RATE TO SERVER */
  if (SSL_connect(ssl) == -1){ //make sure connection is valid
    fprintf(stderr, "Error. TLS connection failure. Aborting.\n");
    sprintf(buff, "Error. TLS connection failure. Aborting.\n");
    write(fd, buff, strlen(buff));
    ERR_print_errors_fp(stderr);
    exit(1);
  }
  else {
    printf("Client-Server connection complete with %s encryption\n", SSL_get_cipher(ssl));
    sprintf(buff, "Client-Server connection complete with %s encryption\n", SSL_get_cipher(ssl));
    write(fd, buff, strlen(buff));
    
    display_server_certificate(ssl);
  }
  
  pthread_t pid;
  pthread_create(&pid, NULL, readfile, NULL);
  
  while(1){
    printf("Current settings: rate: %d, range: %d\n", rate, range);
    sprintf(buff, "Current settings: rate: %d, range: %d\n", rate, range);
    write(fd, buff, strlen(buff));
    heart_rate = 
      generate_random_number(AVERAGE_HEART_RATE-(double)range, AVERAGE_HEART_RATE+(double)range);
    memset(buf,0,sizeof(buf)); //clear out the buffer
    
    //populate the buffer with information about the ip address of the client and the heart rate
    sprintf(buf, "Heart rate of patient %s is %4.2f", my_ip_addr, heart_rate);
    sprintf(buff, "Heart rate of patient %s is %4.2f", my_ip_addr, heart_rate);
    write(fd, buff, strlen(buff));
    
    printf("Sending message '%s' to server...\n", buf);
    sprintf(buff, "Sending message '%s' to server...\n", buf);
    write(fd, buff, strlen(buff));
    SSL_write(ssl, buf, strlen(buf));
    sleep(rate);
  }
  /* FINISH HEART RATE TO SERVER */
  
  //clean up operations
  SSL_free(ssl);
  close(server);
  SSL_CTX_free(ctx);
  return 0;
}

void *readfile() {
  
  char buf[BUFSIZE];
  
  while(1) {
    
    memset(buf,0,sizeof(buf));
    receive_length = SSL_read(ssl, buf, sizeof(buf));
    if(strstr(buf, "new_rate: ") != NULL)
      {
	sscanf(buf, "Heart rate of patient %*s is %*f new_rate: %d", &rate);
	rate = rate;
	printf("New rate %d received from server.\n", rate);
	sprintf(buff, "New rate %d received from server.\n", rate);
	write(fd, buff, strlen(buff));
      }
    
    printf("Received message '%s' from server.\n\n", buf);
    sprintf(buff, "Received message '%s' from server.\n\n", buf);
    write(fd, buff, strlen(buff));
    sleep(rate);
    
  }
}
