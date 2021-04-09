#include "http.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

int http_request_parse_1(char* buf, int len, char** method, char** url, char** version){
	int i = 0;
	int j = 0;
	
	// xxx yyy zzz\r\n
	// ^^^
	while(j < len && !isspace(buf[j]))
		j++;

	if(j >= len)
		return -1;
	else{
		*method = (char*)malloc(j - i + 1);
		if(*method == NULL){
			perror("error malloc\n");
			exit(-1);
		}
		for(int k = i; k < j; k++)
			(*method)[k - i] = buf[k];
		(*method)[j] = 0;
	}

	// xxx yyy zzz\r\n
	//    ^
	while(j < len && isspace(buf[j]))
		j++;
	i = j;
	
	// xxx yyy zzz\r\n
	//     ^^^
	while(j < len && !isspace(buf[j]))
		j++;

	if(j >= len)
		return -1;
	else{
		*url = (char*)malloc(j - i + 1);
		if(*url == NULL){
			perror("error malloc\n");
			exit(-1);
		}
		for(int k = i; k < j; k++)
			(*url)[k - i] = buf[k];
		(*url)[j] = 0;
	}

	// xxx yyy zzz\r\n
	//        ^
	while(j < len && isspace(buf[j]))
		j++;
	i = j;

	// xxx yyy zzz\r\n
	//         ^^^
	while(j < len && !isspace(buf[j]))
		j++;

	if(j >= len)
		return -1;
	else{
		*version = (char*)malloc(j - i + 1);
		if(*version == NULL){
			perror("error malloc\n");
			exit(-1);
		}
		for(int k = i; k < j; k++)
			(*version)[k - i] = buf[k];
		(*version)[j] = 0;
	}
	

	printf("Http request parse:\n");
	printf("method: %s\n", *method);
	printf("url: %s\n", *url);
	printf("version: %s\n", *version);
	return 0;
}