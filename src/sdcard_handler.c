#include "handlers.h"
#include <3ds.h>
#include <strings.h>
#include <sys/stat.h>

#define WWW_ROOT "sdmc:/3ds/www"

static const struct { const char *ext; const char *type; } mime_types[] = {
	{ "html",  "text/html; charset=utf-8" },
	{ "htm",   "text/html; charset=utf-8" },
	{ "css",   "text/css; charset=utf-8" },
	{ "js",    "text/javascript; charset=utf-8" },
	{ "mjs",   "text/javascript; charset=utf-8" },
	{ "json",  "application/json" },
	{ "txt",   "text/plain; charset=utf-8" },
	{ "xml",   "application/xml" },
	{ "svg",   "image/svg+xml" },
	{ "png",   "image/png" },
	{ "jpg",   "image/jpeg" },
	{ "jpeg",  "image/jpeg" },
	{ "gif",   "image/gif" },
	{ "webp",  "image/webp" },
	{ "ico",   "image/x-icon" },
	{ "woff",  "font/woff" },
	{ "woff2", "font/woff2" },
	{ "wasm",  "application/wasm" },
	{ "mp3",   "audio/mpeg" },
	{ "wav",   "audio/wav" },
	{ "mp4",   "video/mp4" },
	{ "pdf",   "application/pdf" },
};

static const char *get_mime_type(const char *path)
{
	const char *slash = strrchr(path, '/');
	const char *dot = strrchr(path, '.');

	// no extension, or the dot belongs to a folder name
	if (!dot || (slash && dot < slash))
		return "application/octet-stream";

	dot++;
	for (size_t i = 0; i < sizeof(mime_types) / sizeof(mime_types[0]); i++)
		if (strcasecmp(dot, mime_types[i].ext) == 0)
			return mime_types[i].type;

	return "application/octet-stream";
}

int is_sdcard_handler(http_request *request)
{
	if (startWith(request->path, "/system/") && strlen(request->path) > 7)
		return 0;

	return 1;
}

static char *do_sdcard_request(char *path, size_t *out_size)
{
	FILE *fptr = fopen(path, "rb");
	if (fptr == NULL)
		return NULL;

	fseek(fptr, 0, SEEK_END);
	size_t fileSize = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	char *buffer = (char *)memalloc(fileSize + 1);
	size_t bytesRead = fread(buffer, 1, fileSize, fptr);
	fclose(fptr);
	if (fileSize != bytesRead)
		failExit("Error reading file");

	buffer[fileSize] = 0;
	*out_size = fileSize;
	return buffer;
}

// Accept only absolute paths with no ".." segment and no backslash.
static int path_is_safe(const char *p)
{
	if (p[0] != '/' || strchr(p, '\\'))
		return 0;

	const char *seg = p;
	while (*seg)
	{
		const char *end = strchr(seg, '/');
		size_t len = end ? (size_t)(end - seg) : strlen(seg);

		if (len == 2 && seg[0] == '.' && seg[1] == '.')
			return 0;
		if (!end)
			break;
		seg = end + 1;
	}
	return 1;
}

static http_response *make_response(int code, const char *mime, char *payload, size_t len)
{
	char header[96];
	int hlen = snprintf(header, sizeof(header), "Content-Type: %s\r\n", mime);

	http_response *response = memalloc(sizeof(http_response));
	response->code = code;
	response->content_type = memdup(header, hlen + 1);
	response->payload = payload;
	response->payload_len = len;
	return response;
}

static http_response *make_error(int code, const char *msg)
{
	size_t len = strlen(msg);
	return make_response(code, "text/plain; charset=utf-8", memdup(msg, len + 1), len);
}

http_response *get_sdcard_response(http_request *request)
{
	char rel[200];
	snprintf(rel, sizeof(rel), "%s", request->path);
	rel[strcspn(rel, "?#")] = '\0';

	if (!path_is_safe(rel))
		return make_error(403, "Forbidden");

	char path[256];
	int n = snprintf(path, sizeof(path), WWW_ROOT "%s", rel);
	if (n < 0 || (size_t)n >= sizeof(path))
		return make_error(414, "Request-URI Too Long");

	// Directories (including "/") serve index.html
	struct stat st;
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
	{
		const char *suffix = (path[n - 1] == '/') ? "index.html" : "/index.html";
		if ((size_t)n + strlen(suffix) >= sizeof(path))
			return make_error(414, "Request-URI Too Long");
		strcat(path, suffix);
	}

	size_t size = 0;
	char *payload = do_sdcard_request(path, &size);
	if (payload == NULL)
		return make_error(404, "Not Found");

	return make_response(200, get_mime_type(path), payload, size);
}