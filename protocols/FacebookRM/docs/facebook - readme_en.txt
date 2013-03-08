--------------------------------
| Facebook Protocol RM 0.0.9.5 |
|        for Miranda NG        |
|          (2.3.2013)          |
--------------------------------

Autor: Robyer
  E-mail: robyer@seznam.cz
  Jabber: robyer@jabbim.cz
  ICQ: 372317536
  Web: http://www.robyer.cz
 
Info: 
 - This plugin is based on Facebook Protocol (author jarvis) version 0.1.3.3 (open source).
 - His version you can find on: http://code.google.com/p/eternityplugins/

--------------------------------
   Information about statuses
--------------------------------
 - Online = connected to fb, chat is online
 - Invisible = connected to fb, but only for getting feeds and notifications - CHAT is OFFLINE
 - Offline = disconnected
 
--------------------------------
      Hidden settings
--------------------------------
"TimeoutsLimit" (Byte) - Errors limit (default 3) after which fb disconnects
"DisableLogout" (Byte) - Disables logout procedure, default 0
"PollRate" (Byte) - Waiting time between buddy list and newsfeed parsing.

--------------------------------
       Version history
--------------------------------

... TODO ...

=== OLD CHANGES (MIRANDA IM) ===

0.0.9.3 - 12.12.2012
 ! Fixed crash when no own avatar found on webpage
 * Plugin statically linked
 
0.0.9.2 - 23.10.2012
 ! Fixed contacts' avatar changes
 ! Fixed connection with one type of required setting machine name
 ! Reworked sending messages requests (should avoid rare ban from FB :))
 ! Create default group for new contacts on login if doesn't exists yet
 ! Fix for visibility changes from other client
 * Plugin linked to C++ 2008 runtimes again

0.0.9.1 - 16.10.2012
 ! Fixed not working login due to Facebook change
 * Plugin linked to C++ 2010 runtimes

0.0.9.0 - 11.6.2012 
 + Receiving friendship requests (check every +-20 minutes)
 + Reqorked authorizations - requesting, approving, rewoking friendships
 + Support for searching and adding people
 * Changes of some strings
 * Use same GUID for 32bit and 64bit versions
 ! Receiving messages with original timestamp
 ! Fixed removing avatars from "On the phone" contacts
 ! Unhooking OnModulesLoaded (thanks Awkward)
 ! Don't send typing notificationsto contacts that are offline
 ! Fixed avatars working (thanks borkra)
 ! SetWindowLong -> SetWindowLongPtr (thanks ghazan) 
 ! Working login with approving last login from unknown device
 ! Fixed sending messages into groups
 ! More internal fixes

0.0.8.1 - 26.4.2012
 ! Fixed getting notifications on login
 ! Fixed getting unread messages on login
 ! Getting unread messages on login with right timestamp
 ! Fixed getting newsfeeds
 ! Fixed related to deleting contacts from miranda/server
 + New newsfeed type option "Applications and Games"
 + Contacts now have MirVer "Facebook" (for Fingerpring plugin)
 + Getting attachements for unread messages on login
 ! Fixed avatars in Miranda 0.10.0#3 and newer (thanks borkra)
 ! Some small fixes (thanks borkra)
 
 x Doesn't work notification about friend requests

0.0.8.0 - 14.3.2012
 # For running plugin is required Miranda 0.9.43 or newer
 # Plugin is compiled with VS2005 (Fb x86) and a VS2010 (Fb x64)
 + Added 2 types of newsfeeds: Photos and Links
 * Reworked options
 ! Fixed setting to notify different type of newsfeeds
 ! Fixed and improved parsing newsfeeds
 ! Fixed getting groupchat messages which contains %
 ! Fixed not working login
 ! Improved deleting of contacts
 + Support for Miranda's EV_PROTO_ONCONTACTDELETED events
 + Added missing GUID for x64 version and updated user-agent
 ! Some other minor fixes or improvements
 ! Fixed item 'Visit Profile' when protocol menus are moved to Main menu
 * Updated language pack file (for translators) 
 - Disabled option for closing message windows on website (temporary doesnt work)

0.0.7.0 - 19.1.2012
 + Support for group chats (EXPERIMENTAL!) - enable it in options
 ! Fixed loading contact list
 ! Fixed potential freeze.

0.0.6.1 - 6.1.2012
 + Returned option to close chat windows (on website)
 + New option to map non-standard statuses to Invisible (insetad of Online)
 + New option to load contacts, which have "On the Phone" status
 ! Fixed changing chat visibility
 ! Very long messages are no longer received duplicitely
 ! Changes and fixes related to multiuser messages and messages from people, which are not in server-list

0.0.6.0 - 18.11.2011
 * Reworked Facebook options
  + Option for use https connection also for "channel" requests
  + Option for use bigger avatars
	+ Option for getting unread messages after login (EXPERIMENTAL!)
	+ Option fot disconnect chat when going offline in Miranda
  - Removed option for setting User-Agent
  - Removed option for show Cookies
 * When contact is deleted, in database you can found datetime of this discovery (value "Deleted" of type DWORD)
 + Option in contact menu for delete from server
 + Option in contact menu for request friendship
 + When deleting contact is showed dialog with option to delete contact also from server
 ! Fixed not working login

0.0.5.5 - 16.11.2011
 ! Fixed contacts not going offline

0.0.5.4 - 16.11.2011 
 ! Fixed few problems with connecting
 ! Fixes for some crashes, memory leaks and communication (thanks borkra)
 @ If is your Facebook disconnecting and you have enabled HTTPS connection, disable option "Validate SSL certificates" for Facebook in Options/Networks.

0.0.5.3 - 31.10.2011
 ! Fixed issue with login for some people
 ! Fixed not receiving some messages
 ! Fixed broken getting own name
 * Faster sending messages

0.0.5.2 - 31.10.2011
 ! Fix for connecting and crashing problem.

0.0.5.1 - 30.10.2011
 ! Work-around for sending messages with url links.

0.0.5.0 - 29.10.2011
 + Notification about friends, which are back on serverlist.
 * Completely reworked avatar support.
 * Using small square avatars by default (can be changed by hidden setting "UseBigAvatars")
 ! Fixed use of hidden setting "UseBigAvatars" 
 ! Fixed setting status message.
 ! Fixed crash with MetaContacts.
 ! Fixed login for some people.
 ! Fixes related to popups on login.
 ! Fixed memory leak related to popups.
 ! Fixed getting unread messages on login (if used hidden setting "ParseUnreadMessages")
 ! Fixed login with setting Device name.
 ! Various fixes, improvements and code cleanup.
 - Removed hidden key "OldFeedsTimestamp"
 @ Thanks borkra for helping me with many things.

0.0.4.3 - 22.9.2011
 ! Fix for new facebook layout.
 ! Fix for getting contact name for new contacts.

0.0.4.2 - 15.9.2011
 ! Fixed not enabling items in status menu.
 ! Don't automatically set contact's status to Online when we got message from him.
 ! Fixed message's sending error codepage.
 + Added support for sending messages in invisible status.
 + Protocol status respects changes of chat status on website.
 + Notify concrete unread notifications after login
 + Added hidden key (ParseUnreadMessages) for getting unread messages after login << WARNING: not fully working!!!

0.0.4.1 - 13.9.2011
 ! Reverted change that made contacts not going offline.

0.0.4.0 - 12.9.2011
 * Internal changes about changing status
 - Removed support for "Away" status
 ! Fixed parsing newsfeeds
 - Removed notification about unread messages in "Invisible" status
 + In "Invisible" status are inbox messages parsed directly to messages
 + Getting gender of contacts
 + Getting all contacts from server at once (not only these, which are online right now)
 + Notification when somebody cancel our friendship (= or when disables his facebook account)

0.0.3.3 - 17.6.2011
 ! Fix for communication (getting seq number)
 ! Fixed notification with unread events after login

0.0.3.2 - 8.6.2011
 ! Mark chat messages on facebook read.
 * Disabled channel refresh notification

0.0.3.1 - 23.5.2011
 ! Fixed notify about timeout when sending message.

0.0.3.0 - 22.5.2011
 ! Fixed downloading avatars
 ! Fixed loading avatars in SRMM
 ! (Maybe) Fixed not loading for some people with miranda 0.9.17
 ! Fixed crashes and freezes when deleting account
 ! Few fixes and improvements related to login procedure
 ! Fixed parsing some newsfeeds
 ! (Hopefully) Fixed some other crashes
 ! Fix for logon crash when notify unread events.
 * Improvement in getting contact avatars (2x faster)
 * Optimized compiler settings -> 2x smaller file (thanks borkra)
 + Used persistent http connection (thanks borkra)
 + Destroy service and hook on exit (thanks FREAK_THEMIGHTY)
 + Support for per-plugin-translation (for MIM 0.10#2) (thanks FREAK_THEMIGHTY)
 + Support for EV_PROTO_ONCONTACTDELETED (for MIM 0.10#2) (thanks FREAK_THEMIGHTY)
 - Do not translate options page title, since it is the account name
 - Disabled close chat "optimalization"
 ! Fixed sending typing notifications
 ! Fixed parsing links from newsfeeds
 * Enabled sending offline messages
 ! Fixed getting errors from sending messages (+ show concrete error)
 + 5 attempts to send message before showing error message

 
0.0.2.4 - 6.3.2011
 ! Fixed duplicit messages and notifications
 * Limit for popup messages from group chats - max. one per 15 seconds
 * Optimalization for sending typing notify
 * Optimalization for closing chat windows on website
 + Notify when is possible that we didnt received some chat message(s)
 + Show popup when try to send offline message and open website for send private message when onclick

0.0.2.3 - 5.3.2011
 ! Fixed loading contact names
 ! Fixed updater support for x64 versions
 + Added 32px status icons
 + Added option for chose type of newsfeeds to notify
 * Changed user-agent names to user-friendlier
 * Enhanced parsing newsfeeds

0.0.2.2 - 2.3.2011
 + Updater support
 + Added item in contact and status menu for open contact profile on website (+ saving in db as Homepage key)
 + Notify about new private (not chat) messages in invisible
 + Automatically set https when connecting if required
 * Optimalization for downloading avatars of contacts
 ! Fix for \n in newsfeeds popups
 ! Fix for html tags in connecting error message
 ! Fix for broken sending messages

0.0.2.1 - 21.2.2011
 ! Fixes for statuses (cannot switch to Away, work in threads)
 ! Fix for loading own avatar when changed
 * Sounds are using account name (thanks FREAK_THEMIGHTY)
 ! Fixes for x64 version (thanks FREAK_THEMIGHTY)
 ! Fix for thread synchronization
 ! Fixed order of outgoing messages and notify about delivery errors
 ! Fix few things which was causing not delivering all incoming messages
 ! Fixes for internal list implementation
 * Refactoring and simplify few things
 + Added x64 version of plugin
 + Notify about new notifications after login
 ! Fixed parsing few types of newsfeeds
 + 1. stage of groupchats - notifying incomming messages

0.0.2.0 - 13.2.2011
 * Guided as new plugin Facebook RM + new readme and folder structure
 x Temporarily disabled updater support (and notification about jarvis's new "fb api")
 ! Fix for load own avatar
 ! Fix for loading newsfeeds and their better parsing
 + Added Away and Invisible statuses
 * Using away status as idle flag
 ! Fixed idle control - facebook falls into idle only when Away status

--------------------------------
      Old version history
 (Already in official version)
--------------------------------
0.0.2.0 (0.1.3.0) - 20.12.2010 (not released)
 ! Oprava zobrazov�n� bublinov�ho ozn�men�
 ! Oprava odhla�ovac� procedury
 ! Oprava zobrazen� po�tu nov�ch zpr�v
 ! Oprava spr�vy ne�innosti  TODO
 ! Oprava na��t�n� stavov�ch zpr�v TODO
 ! Oprava na��t�n� avatar� TODO
 + Kontrola �sp�n�ho odesl�n� zpr�vy
 * Aktualizov�na modifikovan� miranda32.exe na nejnov�j�� verzi
 * Rozd�leno readme na �esk� a anglick�
1.21 - 27.11.2010
 + Notifying about received new "private" (not chat messages) messages
 + Pseudo idle management (when idle, miranda let facebook fall into his own idle)
 + Hidden key for ignoring channel timeouts (add key "DisableChannelTimeouts" (byte) with value 1)
 * Rewrited and edited few things (maybe fixed duplicit messages, maybe added some bugs, etc.)
 ! Fixed idle change of connected contacts
 ! Fixed writing time in log
1.20 - 22.11.2010
 + Option for use balloon notify instead of popups
 + Option for use https protocol (can help with some firewalls)
 + Edited miranda32.exe is added for correct work without timeouts, until somebody rewrite http communication :)
1.19 - 20.11.2010
 ! Fixed downloading and updating contact avatars
 ! Dont set "Clist\MyHandle" to contacts
 ! Fixed html tags in status messages
 ! Show error message when try to send message in offline/to offline contact (only ugly workaround)
 ! Try to fix crashes with too long text from wall post
 ! Temporary ignore timeout errors (until fixed in miranda core)
1.18 - 28.9.2010
 ! Fix for force reconnect.
 ! Dont popup contacts disconnect when self disconnect
1.17 - 23.9.2010
 ! Try to workaround duplicit messages
1.16 - 10.9.2010
 * No old news feeds after login (for old behavior create BYTE key OldFeedsTimestamp with value  1)
 ! Fix for empty news feed (empty feeds will not be notified)
1.15 - 9.9.2010
 ! Fix for setting -1 (infinity) timeout
1.14 - 9.9.2010
 ! Fixed /span> in popups
1.13 - 9.9.2010
 ! Closing fb message window (when receiving message) is in other thread
1.12 - 8.9.2010
 ! Fixed bug when last contact went offline.
1.11 - 7.9.2010
 ! Fix for duplicated wall events without link (change photo, etc.)
1.10 - 7.9.2010
 + Basic opening facebook urls on left mouse btn click
 + Added popup options (colors, timeouts,...)
 + Info about CONCRETE error message when unsuccessful login
1.9 - 4.9.2010
 + Typing notifications
1.8 - 4.9.2010
 + Option to automatically close chat windows (on website)
1.7 (0.1.2.0) - 7.8.2010
 ! Don't show Miranda's "Set status message" dialog when disabled "Set Facebook 'Whats on my mind' status through Miranda status"
 ! Fixed some memory leaks, but some can be still there...
 + Added timestamps into facebook debug log.
1.6 (0.1.1.0) - 15.6.2010
 ! Fixed (again) not working login when "Notification from new devices" enabled.
1.5 - 10.6.2010
 ! Fixed popup with new facebook api available
 ! Fixed bug with sending messages through metacontacts
 ! Fixed one small bug
1.4 - 7.6.2010 14:45
 ! Fixed GetMyAwayMsg for ansi plugins (e.g mydetails)
 + Added sound and basic FB Icon to popups
1.3 - 7.6.2010 01:00
 ! Fixed infinite "contact info refresh" "loop"
 ! Fixed bug when global status icon was set to offline when facebook was switched to offline
 ! Fixed feeds popups
1.2
 ! Login should work even if "Notification from new devices" is enabled.
1.1
 ! Fix for not working login
1.0
 + Added hidden key in database: "Folder" Facebook, key DisableStatusNotify (type Byte), value 1. (Newly added contacts will have set "Ignore status notify" flag (in Options / Events / Filter...)


