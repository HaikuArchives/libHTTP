#ifndef HTTP_FILE_SERVER_H
#define HTTP_FILE_SERVER_H

#include "LibHTTPBuild.h"

#include <Path.h>
#include "HTTPHandler.h"
#include "HTTPUtils.h"

class HTTPFileServer : public HTTPHandler
{
	public:
		HTTPFileServer( const char *defaultFileName );
		virtual ~HTTPFileServer(void);
		
		virtual bool MessageReceived( HTTPRequest *request );
		virtual HTTPHandler *NewCopy( void );
		status_t SetWebDirectory( const char *webDirectory );

	protected:
		virtual bool HandleDefault( void );
		virtual bool HandleGet( void );
		virtual bool HandleHead( void ) { return HandleGet(); };
		virtual bool HandlePost( void ) { return HandleDefault(); };
		virtual bool HandlePut( void ) { return HandleDefault(); };
		virtual bool HandleDelete( void ) { return HandleDefault(); };
		virtual bool HandleOptions( void ) { return HandleDefault(); };
		virtual bool HandleTrace( void ) { return HandleDefault(); };
		
	protected:
		char				defaultFileName[64];
		
		// Setup by MessageReceived() before dispatch
		HTTPRequest 		*request;
		HTTPResponse		*response;
		brokenURI			*brURI;
		BPath				webDirectory;
};

#endif