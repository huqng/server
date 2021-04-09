#include "http.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

int http_request_parse(char* buf, int len, char* method, char* url, char* version){
	int i = 0;
	int j = 0;
	
	while(j < len && !isspace(buf[j]))
		j++;

	if(j >= len)
		return -1;
	else{
		method = (char*)malloc(j - i + 1);
		if(method == NULL){
			perror("malloc\n");
			exit(-1);
		}
		for(int k = i; k < j; k++)
			method[k - i] = buf[k];
		method[j] = 0;
	}
	while(j < len && isspace(buf[j]))
		j++;
	i = j;
	
	while(j < len && !isspace(buf[j]))
		j++;

	if(j >= len)
		return -1;
	else{
		url = (char*)malloc(j - i + 1);
		if(url == NULL){
			perror("malloc\n");
			exit(-1);
		}
		for(int k = i; k < j; k++)
			url[k - i] = buf[k];
		url[j] = 0;
	}
	while(j < len && isspace(buf[j]))
		j++;
	i = j;

	while(j < len && !isspace(buf[j]))
		j++;

	if(j >= len)
		return -1;
	else{
		version = (char*)malloc(j - i + 1);
		if(version == NULL){
			perror("malloc\n");
			exit(-1);
		}
		for(int k = i; k < j; k++)
			version[k - i] = buf[k];
		version[j] = 0;
	}
	
	printf("method: %s - %ld\n", method, strlen(method));
	printf("url: %s - %ld\n", url, strlen(url));
	printf("version: %s - %ld\n", version, strlen(version));
	return 0;
}