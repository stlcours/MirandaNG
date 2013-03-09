#define __MAJOR_VERSION				0
#define __MINOR_VERSION				8
#define __RELEASE_NUM				0
#define __BUILD_NUM					0

#define __FILEVERSION_STRING		__MAJOR_VERSION,__MINOR_VERSION,__RELEASE_NUM,__BUILD_NUM
#define __TOSTRING(x)			#x
#define __VERSION_STRING		__TOSTRING(__FILEVERSION_STRING)

#define __PLUGIN_NAME				"Send screenshot+"
#define __FILENAME					"SendSS.dll"
#define __DESCRIPTION 				"Take a screenshot and send it to a contact."
#define __AUTHOR					"Merlin"
#define __AUTHOREMAIL				"ing.u.horn@googlemail.com"
#define __AUTHORWEB					"http://miranda-ng.org/"
#define __COPYRIGHT					"� 2010 Merlin, 2004-2006 Sergio Vieira Rolanski"
#define __USER_AGENT_STRING			__PLUGIN_NAME##" v"##__VERSION_STRING
