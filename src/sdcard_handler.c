#include "handlers.h"
#include <3ds.h>

int is_sdcard_handler(http_request *request)
{
	if (startWith(request->path, "/system/") && strlen(request->path) > 7)
		return 0;

	return 1;
}
char *do_sdcard_request(char *path)
{
	const char *realPath = path;
	
	FILE *fptr = fopen(realPath,"r");
	//Thanks to Evie for helping me clean this clusterf*ck
	if (fptr == NULL)
	{
		return "";
	}
	fseek(fptr,0,SEEK_END);
	size_t fileSize = ftell(fptr);
	//printTop("size_t: %u",fileSize);
	fseek(fptr, 0, SEEK_SET);
	
	/*
	Handle fptr;
	u64 fileSize;
	FSUSER_OpenFileDirectly(&fptr,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""),fsMakePath(PATH_ASCII,p),FS_OPEN_READ,0);
	FSFILE_GetSize(fptr,&fileSize);
	FSFILE_Read(fptr,NULL,1,buffer,sizeof(buffer));
	*/

	char *buffer = (char *)memalloc(fileSize + 1);
	buffer[fileSize] = 0;
	size_t bytesRead = fread(buffer,1,fileSize,fptr);
	//printTop("Sent: %s\n",buffer);
	if(fileSize!=bytesRead) failExit("Error reading file");
	//FSFILE_Close(fptr);
	fclose(fptr);
	return buffer;
}
http_response *get_sdcard_response(http_request *request)
{
	http_response *response = memalloc(sizeof(http_response));
	char path[256];
	snprintf(path, sizeof(path), "sdmc:/3ds/www%s", request->path);
	char *payload = do_sdcard_request(path);
	response->code = (payload == NULL) ? 200 : 404;
	// const char content_type[] = "Content-Type: text/html\r\n";
	// response->content_type = memdup(content_type, sizeof(content_type));
	response->payload = payload;
	response->payload_len = strlen(payload);
	return response;    
}
