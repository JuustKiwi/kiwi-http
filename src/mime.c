#include <string.h>
#include "../include/mime.h"

const char *mime_get_type( const char *filepath ){
	const char *dot;
	
	dot = strrchr( filepath, '.' );
	if( dot == NULL ){
		return "text/plain";
	}

	if( strcmp( dot, ".html" ) == 0 || strcmp( dot, ".htm" ) == 0 ){
		return "text/html";
	}
	if( strcmp( dot, ".css" ) == 0 ){
		return "text/css";
	}
	if( strcmp( dot, ".js" ) == 0 ){
		return "application/javascript";
	}
	if( strcmp( dot, ".png" ) == 0 ){
		return "image/png";
	}
	if( strcmp( dot, ".jpg" ) == 0 || strcmp( dot, ".jpeg" ) == 0 ){
		return "image/jpeg";
	}
	if( strcmp( dot, ".gif" ) == 0 ){
		return "image/gif";
	}
	if( strcmp( dot, ".svg" ) == 0 ){
		return "image/svg+xml";
	}
	if( strcmp( dot, ".json" ) == 0 ){
		return "application/json";
	}
	if( strcmp( dot, ".ico" ) == 0 ){
		return "image/x-icon";
	}

	return "application/octet-stream";
}
