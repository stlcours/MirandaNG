#define __MAJOR_VERSION				0
#define __MINOR_VERSION				5
#define __RELEASE_NUM				0
#define __BUILD_NUM					0

#define __FILEVERSION_STRING		__MAJOR_VERSION,__MINOR_VERSION,__RELEASE_NUM,__BUILD_NUM
#define __FILEVERSION_DOTS			__MAJOR_VERSION.__MINOR_VERSION.__RELEASE_NUM.__BUILD_NUM

#define __STRINGIFY_IMPL(x)			#x
#define __STRINGIFY(x)				__STRINGIFY_IMPL(x)
#define __VERSION_STRING			__STRINGIFY(__FILEVERSION_DOTS)

#define __PLUGIN_NAME				"FTP File YM"
#define __INTERNAL_NAME				"FTPFile"
#define __FILENAME					"FTPFile.dll"
#define __DESCRIPTION 				"FTP a file to a server and send the URL to your friend. Supported automatic zipping before upload and encryption via SFTP and FTPS."
#define __AUTHOR					"yaho"
#define __AUTHOREMAIL				"yaho@miranda-easy.net"
#define __AUTHORWEB					"http://miranda-ng.org/"
#define __COPYRIGHT					"� 2007-2010 Jan Holub"
