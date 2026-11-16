#include "handlers.h"

static void system_reboot()
{
	aptInit();
	APT_HardwareResetAsync(); // reboot
	aptExit();
}

int is_system_request(http_request *request)
{
	return startWith(request->path, "/system/") && strlen(request->path) > 7;
}

http_response *get_system_handler_response(http_request *request)
{
	http_response *response = memalloc(sizeof(http_response));
	response->code = 200;
	const char content_type[] = "Content-Type: text/html\r\n";
	response->content_type = memdup(content_type, sizeof(content_type));
	char	*payload = memalloc(1024 * sizeof(char));
	if (!strcmp(request->path, "/system/info"))
	{
		sprintf(payload, "<html><title>System info</title><h1>Yes this is running on a New Nintendo 2DS XL</h1><p>Running firmware version %lu</p><p>Free ram: %lu / %lu kb</p><p>Internet connection: %u / 3</p></html>", osGetFirmVersion(), osGetMemRegionFree(0) / 1024 / 1024, osGetMemRegionSize(0) / 1024 / 1024, osGetWifiStrength());
	} else
	{
		sprintf(payload, "OK");
	}
	response->payload = payload;
	response->payload_len = strlen(response->payload);
	return response;
}

void pre_system_response(http_server *server, http_request *request)
{
	if (!strcmp(request->path, "/system/exit"))
	{
		printTop("Remote app exit triggered!");
		server->running = 0;
	}
	else if (!strcmp(request->path, "/system/reboot"))
	{
		printTop("System reboot asked!");
		system_reboot();
	}
}
