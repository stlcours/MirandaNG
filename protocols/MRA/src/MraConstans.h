#if !defined(AFX_MRA_CONSTANS_H__F58D13FF_F6F2_476C_B8F0_7B9E9357CF48__INCLUDED_)
#define AFX_MRA_CONSTANS_H__F58D13FF_F6F2_476C_B8F0_7B9E9357CF48__INCLUDED_

#include "proto.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define PROTOCOL_DISPLAY_NAME_ORIGA          "Mail.ru Agent"

#define MIRVER_UNKNOWN                       ("Mail.ru Agent unknown client")

#define MRA_PLUGIN_UPDATER_ID                2544 //���������� ����� ������� ������������ �� ����
#define MRA_SERVER_PORT_STANDART_NLB         2042
#define MRA_SERVER_PORT_STANDART             2041
#define MRA_SERVER_PORT_HTTPS                443
static const LPSTR lpcszMailRuDomains[] =
{
   ("mail.ru"),
   ("list.ru"),
   ("bk.ru"),
   ("inbox.ru"),
   ("corp.mail.ru"),
   NULL
};
#define MAILRU_CHAT_CONF_DOMAIN              "chat.agent"


#define MAX_EMAIL_LEN                        1024
#define MAX_FILEPATH                         32768 //internal
#define BUFF_SIZE_RCV                        65535 //internal
#define BUFF_SIZE_RCV_MIN_FREE               16384 //internal
#define BUFF_SIZE_BLOB                       16384 //internal
#define BUFF_SIZE_URL                        4096 //internal
#define NETLIB_SELECT_TIMEOUT                250 //internal // ����� �������� ������� �� ������
#define WAIT_FOR_THREAD_TIMEOUT              15 //internal // ����� �������� ���������� ������
#define THREAD_SLEEP_TIME                    100 //internal
#define THREAD_MAX_PING_TIME                 20 // sec, internal
#define THREAD_MAX_PING_FAIL_COUNT           3 // internal
#define SEND_QUEUE_TIMEOUT                   600 //internal // ����� �������� �������������� ��������� �� ������� ��������
#define ALLOCATED_COUNT                      32 //internal   // ����������� ���������
#define EMAILS_MIN_COUNT                     16 //internal   // ����������� ��������� ����������� ����������� ��� ���������� email ������ �� ���� �����
#define PHONES_MIN_COUNT                     4 //internal   // ����������� ��������� ����������� ����������� ��� ���������� email ������ �� ���� �����

#define MRA_ALARM_MESSAGE                    LPGENT("Your contact wakes you")

#define MRA_GOTO_INBOX                       "/GotoInbox"
#define MRA_GOTO_INBOX_STR                   LPGENT("Display &Inbox")
#define MRA_SHOW_INBOX_STATUS                "/ShowInboxStatus"
#define MRA_SHOW_INBOX_STATUS_STR            LPGENT("Display &Inbox status")
#define MRA_EDIT_PROFILE                     "/EditProfile"
#define MRA_EDIT_PROFILE_STR                 LPGENT("Edit &Profile")
#define MRA_MY_ALBUM                         "/MyAlbum"
#define MRA_MY_ALBUM_STR                     LPGENT("My Album")
#define MRA_MY_BLOG                          "/MyBlog"
#define MRA_MY_BLOG_STR                      LPGENT("My Blog")
#define MRA_MY_BLOGSTATUS                    "/MyBlogStatus"
#define MRA_MY_BLOGSTATUS_STR                LPGENT("My Blog Status")
#define MRA_MY_VIDEO                         "/MyVideo"
#define MRA_MY_VIDEO_STR                     LPGENT("My Video")
#define MRA_MY_ANSWERS                       "/MyAnswers"
#define MRA_MY_ANSWERS_STR                   LPGENT("My Answers")
#define MRA_MY_WORLD                         "/MyWorld"
#define MRA_MY_WORLD_STR                     LPGENT("My World")
#define MRA_ZHUKI                            "/Zhuki"
#define MRA_ZHUKI_STR                        LPGENT("Zhuki")
#define MRA_WEB_SEARCH                       "/WebSearch"
#define MRA_WEB_SEARCH_STR                   LPGENT("Web search")
#define MRA_UPD_ALL_USERS_INFO               "/UpdateAllUsersInfo"
#define MRA_UPD_ALL_USERS_INFO_STR           LPGENT("Update all users info")
#define MRA_CHK_USERS_AVATARS                "/CheckUpdatesUsersAvatars"
#define MRA_CHK_USERS_AVATARS_STR            LPGENT("Check updates users avatars")
#define MRA_REQ_AUTH_FOR_ALL                 "/ReqAuthForAll"
#define MRA_REQ_AUTH_FOR_ALL_STR             LPGENT("Request authorization for all")

#define MRA_MPOP_AUTH_URL                    "http://swa.mail.ru/cgi-bin/auth?Login=%s&agent=%s&page=%s"

#define MRA_WIN_INBOX_URL                    "http://win.mail.ru/cgi-bin/start"
#define MRA_PDA_INBOX_URL                    "http://pda.mail.ru/cgi-bin/start"
#define MRA_EDIT_PROFILE_URL                 "http://win.mail.ru/cgi-bin/userinfo?mra=1"
#define MRA_CHAT_URL                         "http://chat.mail.ru"
#define MRA_ZHUKI_URL                        "http://zhuki.mail.ru"
#define MRA_SEARCH_URL                       "http://go.mail.ru"

// used spesialy! added: /domain/user
#define MRA_BLOGS_URL                        "http://blogs.mail.ru"
#define MRA_FOTO_URL                         "http://foto.mail.ru"
#define MRA_VIDEO_URL                        "http://video.mail.ru"
#define MRA_ANSWERS_URL                      "http://otvet.mail.ru"
#define MRA_WORLD_URL                        "http://my.mail.ru"

// without auth
#define MRA_REGISTER_URL                     "http://win.mail.ru/cgi-bin/signup"
#define MRA_FORGOT_PASSWORD_URL              "http://win.mail.ru/cgi-bin/passremind"


// wParam = (WPARAM)hContact
#define MRA_REQ_AUTH                         "/ReqAuth"
#define MRA_REQ_AUTH_STR                     LPGENT("Request authorization")
#define MRA_GRANT_AUTH                       "/GrantAuth"
#define MRA_GRANT_AUTH_STR                   LPGENT("Grant authorization")
#define MRA_SEND_POSTCARD                    "/SendPostcard"
#define MRA_SEND_POSTCARD_STR                LPGENT("&Send postcard")
#define MRA_VIEW_ALBUM                       "/ViewAlbum"
#define MRA_VIEW_ALBUM_STR                   LPGENT("&View Album")
#define MRA_READ_BLOG                        "/ReadBlog"
#define MRA_READ_BLOG_STR                    LPGENT("&Read Blog")
#define MRA_REPLY_BLOG_STATUS                "/ReplyBlogStatus"
#define MRA_REPLY_BLOG_STATUS_STR            LPGENT("Reply Blog Status")
#define MRA_VIEW_VIDEO                       "/ViewVideo"
#define MRA_VIEW_VIDEO_STR                   LPGENT("View Video")
#define MRA_ANSWERS                          "/Answers"
#define MRA_ANSWERS_STR                      LPGENT("Answers")
#define MRA_WORLD                            "/World"
#define MRA_WORLD_STR                        LPGENT("World")
#define MRA_SEND_NUDGE                       "/SendNudge"
#define MS_NUDGE                             "/Nudge"
#define MRA_SENDNUDGE_STR                    LPGENT("Send &Nudge")


#define ADV_ICON_DELETED                     0
#define ADV_ICON_DELETED_ID                  "ADV_ICON_DELETED"
#define ADV_ICON_DELETED_STR                 LPGENT("Mail box deleted")
#define ADV_ICON_NOT_ON_SERVER               1
#define ADV_ICON_NOT_ON_SERVER_ID            "ADV_ICON_NOT_ON_SERVER"
#define ADV_ICON_NOT_ON_SERVER_STR           LPGENT("Contact not on server")
#define ADV_ICON_NOT_AUTHORIZED              2
#define ADV_ICON_NOT_AUTHORIZED_ID           "ADV_ICON_NOT_AUTHORIZED"
#define ADV_ICON_NOT_AUTHORIZED_STR          LPGENT("Not authorized")
#define ADV_ICON_PHONE                       3
#define ADV_ICON_PHONE_ID                    "ADV_ICON_PHONE"
#define ADV_ICON_PHONE_STR                   LPGENT("Phone/SMS only contact")
#define ADV_ICON_BLOGSTATUS                  4
#define ADV_ICON_BLOGSTATUS_ID               "ADV_ICON_BLOGSTATUS"
#define ADV_ICON_BLOGSTATUS_STR              LPGENT("Blog status message")

#define MRA_SOUND_NEW_EMAIL                   LPGEN("New E-mail available in Inbox")

#define MAILRU_SERVER_TIME_ZONE              -180 //internal // +0300
#define MAILRU_CONTACTISTYPING_TIMEOUT       10 //internal
#define MRA_DEFAULT_SERVER                   "mrim.mail.ru"
#define MRA_DEFAULT_SERVER_PORT              MRA_SERVER_PORT_STANDART_NLB
#define MRA_MAX_MRIM_SERVER                  46
#define MRA_DEFAULT_TIMEOUT_CONN_MRIM        20
#define MRA_DEFAULT_CONN_RETRY_COUNT_MRIM    2
#define MRA_DEFAULT_TIMEOUT_CONN_NLB         20
#define MRA_DEFAULT_CONN_RETRY_COUNT_NLB     3
#define MRA_DEFAULT_TIMEOUT_RECV_NLB         20
#define MRA_DEFAULT_CONN_RETRY_COUNT_FILES   2
#define MRA_DEFAULT_CONN_RETRY_COUNT_MRIMPROXY 3
#define MRA_TIMEOUT_DIRECT_CONN              30 //internal; ����� ����� �������� ������ ������� ��� � ���� ����������� �� �������
#define MRA_TIMEOUT_CONN_MIN                 2 //internal; ����������� ����� �������� ���� ��������������� ��������� �����������
#define MRA_TIMEOUT_CONN_���                 16 //internal; ������������ ����� �������� ���� ��������������� ��������� �����������
#define MRA_MAXLENOFMESSAGE                  32768 //internal
#define MRA_MAXCONTACTSPERPACKET             256 //internal
#define MRA_CODE_PAGE                        1251 //internal
#define MRA_FEATURE_FLAGS                    (FEATURE_FLAG_BASE_SMILES|FEATURE_FLAG_ADVANCED_SMILES|FEATURE_FLAG_CONTACTS_EXCH|FEATURE_FLAG_WAKEUP|FEATURE_FLAG_MULTS|FEATURE_FLAG_FILE_TRANSFER|FEATURE_FLAG_GAMES)

#define MRA_DEFAULT_POPUPS_ENABLED           TRUE
#define MRA_DEFAULT_POPUPS_EVENT_FILTER      -1
#define MRA_DEFAULT_POPUP_TIMEOUT            8
#define MRA_DEFAULT_POPUP_USE_WIN_COLORS     TRUE
#define MRA_DEFAULT_POPUP_COLOR_BACK         RGB(191, 0, 0)   	//Red
#define MRA_DEFAULT_POPUP_COLOR_TEXT         RGB(255, 245, 225)	//Yellow

#define MRA_DEFAULT_NLB_FAIL_DIRECT_CONNECT     FALSE // hidden option
#define MRA_DEFAULT_AUTO_ADD_CONTACTS_TO_SERVER TRUE
#define MRA_DEFAULT_AUTO_AUTH_REQ_ON_LOGON      FALSE
#define MRA_DEFAULT_AUTO_AUTH_GRAND_IN_CLIST    FALSE
#define MRA_DEFAULT_AUTO_AUTH_GRAND_NEW_USERS   FALSE
#define MRA_DEFAULT_AUTO_AUTH_GRAND_NEW_USERS_DISABLE_SPAM_CHECK	FALSE
#define MRA_DEFAULT_SLOW_SEND                TRUE
#define MRA_DEFAULT_CVT_SMILES_TO_TAGS       TRUE
#define MRA_DEFAULT_MIRVER_RAW               FALSE // hidden option
#define MRA_DEFAULT_AUTH_MESSAGE             LPGENT("Please authorize me")

#define MRA_DEFAULT_RTF_RECEIVE_ENABLE       FALSE
#define MRA_DEFAULT_RTF_SEND_ENABLE          TRUE
#define MRA_DEFAULT_RTF_SEND_SMART           TRUE
#define MRA_DEFAULT_RTF_BACKGROUND_COLOUR    RGB(255, 255, 255)
#define MRA_DEFAULT_RTF_FONT_COLOUR          RGB(255, 0, 0)
#define MRA_DEFAULT_RTF_FONT_SIZE            12
#define MRA_DEFAULT_RTF_FONT_CHARSET         RUSSIAN_CHARSET
#define MRA_DEFAULT_RTF_FONT_NAME            L"Tahoma"

#define MRA_DEFAULT_SEARCH_REMEMBER          TRUE
#define MRA_DEFAULT_LAST_SEARCH_COUTRY       0


#define MRA_DEFAULT_FILE_SEND_BLOCK_SIZE     8192 //hidden, ������ ����� ��� �������� �����

#define MRA_DEFAULT_HIDE_MENU_ITEMS_FOR_NON_MRA     FALSE

#define MRA_DEFAULT_INC_NEW_MAIL_NOTIFY             FALSE
#define MRA_DEFAULT_TRAYICON_NEW_MAIL_NOTIFY        FALSE
#define MRA_DEFAULT_TRAYICON_NEW_MAIL_CLK_TO_INBOX  FALSE

#define MRA_ANTISPAM_DEFAULT_ENABLE                 TRUE
#define MRA_ANTISPAM_DEFAULT_CHK_TEMP_CONTACTS      TRUE
#define MRA_ANTISPAM_DEFAULT_DELETE_SPAMBOT_CONTACT FALSE
#define MRA_ANTISPAM_DEFAULT_CLN_NON_ALPHNUM        TRUE
#define MRA_ANTISPAM_DEFAULT_MAX_LNG_CHANGES        5
#define MRA_ANTISPAM_DEFAULT_SHOWPOP                TRUE
#define MRA_ANTISPAM_DEFAULT_WRITETOSYSTEMHISTORY   TRUE
#define MRA_ANTISPAM_DEFAULT_SEND_SPAM_REPORT_TO_SERVER TRUE

#define MRA_AVT_DEFAULT_ENABLE               TRUE
#define MRA_AVT_DEFAULT_WRK_THREAD_COUNTS    4 // hidden
#define MRA_AVT_DEFAULT_SERVER               "obraz.foto.mail.ru"
#define MRA_AVT_DEFAULT_SERVER_PORT          80
#define MRA_AVT_DEFAULT_TIMEOUT_CONN         10
#define MRA_AVT_DEFAULT_CONN_RETRY_COUNT     3
#define MRA_AVT_DEFAULT_TIMEOUT_RECV         30
#define MRA_AVT_DEFAULT_USE_KEEPALIVE_CONN   TRUE
#define MRA_AVT_DEFAULT_CHK_INTERVAL         60
#define MRA_AVT_DEFAULT_AVT_FILENAME         _T("_default.jpg")
#define MRA_AVT_DEFAULT_RET_ABC_PATH         TRUE
#define MRA_DELETE_AVT_ON_CONTACT_DELETE     TRUE
#define MRA_AVT_DEFAULT_QE_CHK_INTERVAL      1000 //internal

#define MRA_FILES_QUEUE_PROGRESS_INTERVAL    250 //internal
#define MRA_FILES_NULL_ADDRR                 "192.168.0.1:26666;"
#define MRA_DEF_FS_TIMEOUT_RECV              600
#define MRA_DEF_FS_ENABLE_DIRECT_CONN        TRUE
#define MRA_DEF_FS_NO_OUT_CONN_ON_RCV        FALSE
#define MRA_DEF_FS_NO_OUT_CONN_ON_SEND       FALSE
#define MRA_DEF_FS_IGNORY_ADDITIONAL_PORTS   FALSE
#define MRA_DEF_FS_HIDE_MY_ADDRESSES         FALSE
#define MRA_DEF_FS_ADD_EXTRA_ADDRESSES       FALSE
#define MRA_DEF_FS_ENABLE_MRIM_PROXY_CONS    TRUE

#define PS_GETMYAVATAR "/GetMyAvatar"
//wParam = (char *)Buffer to file name
//lParam = (int)Buffer size
//return = 0 for sucess

#define ICQACKTYPE_SMS      1001
#define ICQEVENTTYPE_SMS    2001    //database event type
#define MS_ICQ_SENDSMS      "/SendSMS"

// Database setting names
#define DBSETTING_CAPABILITIES    "caps"
#define DBSETTING_XSTATUSID       "XStatusId"
#define DBSETTING_XSTATUSNAME     "XStatusName"
#define DBSETTING_XSTATUSMSG      "XStatusMsg"
#define DBSETTING_BLOGSTATUSTIME  "BlogStatusTime"
#define DBSETTING_BLOGSTATUSID    "BlogStatusID"
#define DBSETTING_BLOGSTATUS      "BlogStatus"
#define DBSETTING_BLOGSTATUSMUSIC "ListeningTo"

extern const LPSTR  lpcszStatusUri[];
extern const LPWSTR lpcszXStatusNameDef[];

#define MRA_XSTATUS_MENU        "/menuXStatus"
#define MRA_XSTATUS_COUNT       50
#define MRA_MIR_XSTATUS_NONE    0
#define MRA_MIR_XSTATUS_UNKNOWN MRA_XSTATUS_COUNT

#define MRA_XSTATUS_OFFLINE     0
#define MRA_XSTATUS_ONLINE      1
#define MRA_XSTATUS_AWAY        2
#define MRA_XSTATUS_INVISIBLE   3
#define MRA_XSTATUS_DND         4
#define MRA_XSTATUS_CHAT        5
#define MRA_XSTATUS_UNKNOWN     55

#define MRA_XSTATUS_INDEX_OFFSET	6

#endif // !defined(AFX_MRA_CONSTANS_H__F58D13FF_F6F2_476C_B8F0_7B9E9357CF48__INCLUDED_)
