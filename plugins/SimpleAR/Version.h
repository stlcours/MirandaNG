#define __MAJOR_VERSION				2
#define __MINOR_VERSION				0
#define __RELEASE_NUM				2
#define __BUILD_NUM					6

#define __FILEVERSION_STRING		__MAJOR_VERSION,__MINOR_VERSION,__RELEASE_NUM,__BUILD_NUM
#define __FILEVERSION_DOTS			__MAJOR_VERSION.__MINOR_VERSION.__RELEASE_NUM.__BUILD_NUM

#define __STRINGIFY_IMPL(x)			#x
#define __STRINGIFY(x)				__STRINGIFY_IMPL(x)
#define __VERSION_STRING			__STRINGIFY(__FILEVERSION_DOTS)


#define __PLUGIN_NAME				"SimpleAR"
#define __INTERNAL_NAME				"Simple Auto Replier"
#define __FILENAME					"SimpleAR.dll"
#define __DESCRIPTION 				"Simple Auto Replier."
#define __AUTHOR					"Stark Wong, Mataes, Mikel-Ard-Ri"
#define __AUTHOREMAIL				"mikelardri@gmail.com"
#define __AUTHORWEB					"http://nightly.miranda.im/"
#define __COPYRIGHT					"� 2012"
