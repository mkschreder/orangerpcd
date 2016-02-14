#pragma once

struct mimetype {
    const char *extn;
    const char *mime;
};

static const struct mimetype http_mime_types[] = {
    { ".txt",     "text/plain" },
    { ".log",     "text/plain" },
    { ".css",     "text/css" },
    { ".html",    "text/html" },
    { ".htm",     "text/html" },
    { ".diff",    "text/x-patch" },
    { ".patch",   "text/x-patch" },

    { ".bmp",     "image/bmp" },
    { ".gif",     "image/gif" },
    { ".png",     "image/png" },
    { ".jpg",     "image/jpeg" },
    { ".jpeg",    "image/jpeg" },
    { ".svg",     "image/svg+xml" },

    { ".json",    "application/json" },
    { ".jsonp",   "application/javascript" },
    { ".js",      "text/javascript" },
    { ".zip",     "application/zip" },
    { ".pdf",     "application/pdf" },
    { ".xml",     "application/xml" },
    { ".xsl",     "application/xml" },
    { ".doc",     "application/msword" },
    { ".ppt",     "application/vnd.ms-powerpoint" },
    { ".xls",     "application/vnd.ms-excel" },
    { ".odt",     "application/vnd.oasis.opendocument.text" },
    { ".odp",     "application/vnd.oasis.opendocument.presentation" },
    { ".pl",      "application/x-perl" },
    { ".sh",      "application/x-shellscript" },
    { ".php",     "application/x-php" },
    { ".deb",     "application/x-deb" },
    { ".iso",     "application/x-cd-image" },
    { ".tar.gz",  "application/x-compressed-tar" },
    { ".tgz",     "application/x-compressed-tar" },
    { ".gz",      "application/x-gzip" },
    { ".tar.bz2", "application/x-bzip-compressed-tar" },
    { ".tbz",     "application/x-bzip-compressed-tar" },
    { ".bz2",     "application/x-bzip" },
    { ".tar",     "application/x-tar" },
    { ".rar",     "application/x-rar-compressed" },

    { ".mp3",     "audio/mpeg" },
    { ".ogg",     "audio/x-vorbis+ogg" },
    { ".wav",     "audio/x-wav" },

    { ".mpg",     "video/mpeg" },
    { ".mpeg",    "video/mpeg" },
    { ".avi",     "video/x-msvideo" },

    { ".README",  "text/plain" },
    { ".log",     "text/plain" },
    { ".cfg",     "text/plain" },
    { ".conf",    "text/plain" },

    { ".pac",        "application/x-ns-proxy-autoconfig" },
    { ".wpad.dat",   "application/x-ns-proxy-autoconfig" },
    
    // put all shorter ones last so that we match the longer ones first!
    { ".c",       "text/x-csrc" },
    { ".h",       "text/x-chdr" },
    { ".o",       "text/x-object" },
    { ".ko",      "text/x-object" },

    { NULL, NULL }
};

static inline const char *mimetype_lookup(const char *ext){
	for(const struct mimetype *mime = &http_mime_types[0]; mime->extn; mime++){
		if(strcmp(mime->extn, ext) == 0) return mime->mime; 
	}
	return "application/octet-stream"; 
}
