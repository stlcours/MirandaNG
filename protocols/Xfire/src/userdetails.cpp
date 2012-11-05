/*
 *  Plugin of miranda IM(ICQ) for Communicating with users of the XFire Network. 
 *
 *  Copyright (C) 2010 by
 *          dufte <dufte@justmail.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 *  Based on J. Lawler              - BaseProtocol
 *			 Herbert Poul/Beat Wolf - xfirelib
 *
 *  Miranda ICQ: the free icq client for MS Windows 
 *  Copyright (C) 2000-2008  Richard Hughes, Roland Rabien & Tristan Van de Vreede
 *
 */

#include "stdafx.h"

#include "baseProtocol.h"
#include "Xfire_gamelist.h"
#include <string>

HWND ghwndDlg=NULL;
extern HANDLE XFireWorkingFolder;
extern Xfire_gamelist xgamelist;


//als funktion, damit es per thread geladen werden kann
void LoadProfilStatus(LPVOID lparam) {
	if(!lparam || !ghwndDlg)
		return;

	//dl
	char url[255]="http://miniprofile.xfire.com/bg/sh/type/1/";
	char* buf=NULL;
	unsigned int size=0;
	strcat_s(url,255,(char*)lparam);
	strcat_s(url,255,".png");

	//versuche das icon aus dem inet zulasen
	if(GetWWWContent2(url,NULL,FALSE,&buf,&size))
	{
		//aus dem buffer ein hicon erzeugen
		HBITMAP hbitmap=xgamelist.createHBITMAPfromdata(buf,size);
		//speicher freigeben
		delete[] buf;
		SendMessage(GetDlgItem(ghwndDlg,IDC_PROFILIMG),STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbitmap);
	}
	delete[] lparam;
}

void SetItemTxt(HWND hwndDlg,int feldid,char*feld,HANDLE hcontact,int type)
{
	DBVARIANT dbv;
	if(!DBGetContactSetting(hcontact,protocolname,feld,&dbv)) {
		if(type==1)
		{
			char temp[255];
			sprintf(temp,"%i",dbv.wVal);
			SetDlgItemText(hwndDlg,feldid,temp);
		}
		else
		{
			SetDlgItemText(hwndDlg,feldid,dbv.pszVal);
		}
		DBFreeVariant(&dbv);
		EnableDlgItem(hwndDlg,feldid,TRUE);
	}
	else
	{
		SetDlgItemText(hwndDlg,feldid,Translate("<not specified>"));
		EnableDlgItem(hwndDlg,feldid,FALSE);
	}
}

static int GetIPPortUDetails(HANDLE wParam,char* feld1,char* feld2)
{
	char temp[255];
    HGLOBAL clipbuffer;
	char* buffer;

	if(DBGetContactSettingWord((HANDLE)wParam, protocolname, feld2, -1)==0)
		return 0;

	DBVARIANT dbv;
	if(DBGetContactSettingString((HANDLE)wParam, protocolname, feld1,&dbv))
		return 0;

	sprintf(temp,"%s:%d",dbv.pszVal,DBGetContactSettingWord((HANDLE)wParam, protocolname, feld2, -1));

	DBFreeVariant(&dbv);

	if(OpenClipboard(NULL))
	{
		EmptyClipboard();

		clipbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(temp)+1);
		buffer = (char*)GlobalLock(clipbuffer);
		strcpy(buffer, LPCSTR(temp));
		GlobalUnlock(clipbuffer);

		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
	}

	return 0;
}

void addToList(HWND listbox,HANDLE hContact,char*key,char*val)
{
	DBVARIANT dbv;
	if(!DBGetContactSetting(hContact,protocolname,val,&dbv))
	{
		LVITEM lvitem;
		memset(&lvitem,0,sizeof(lvitem));
		lvitem.mask=LVIF_TEXT;
		lvitem.cchTextMax=255;
		lvitem.iItem=0;
		lvitem.iSubItem=0;
		lvitem.pszText=key;
		SendMessageA(listbox,LVM_INSERTITEM,0,(LPARAM)&lvitem);
		lvitem.iSubItem++;
		lvitem.pszText=dbv.pszVal;
		SendMessageA(listbox,LVM_SETITEM,0,(LPARAM)&lvitem);
		DBFreeVariant(&dbv);
	}
}

void setGameInfo(HWND listbox,char *mbuf)
{
	int ii=0;
	char temp[255];
	char mod=0;
	char item=0;
	char *mbuf2=(char*)mbuf;
	LVITEM lvitem;
	memset(&lvitem,0,sizeof(lvitem));
	lvitem.mask=LVIF_TEXT;
	lvitem.cchTextMax=255;

	while(*mbuf2!=0)
	{
		if(*mbuf2==1&&mod==0)
		{
			temp[ii]=0;
			mod=1;
			lvitem.iItem=item;
			lvitem.iSubItem=0;
			lvitem.pszText=temp;
			SendMessageA(listbox,LVM_INSERTITEM,0,(LPARAM)&lvitem);
			item++;
			ii=-1;
		}
		else if(*mbuf2==2&&mod==1)
		{
			temp[ii]=0;
			mod=0;
			lvitem.iSubItem++;
			lvitem.pszText=temp;
			SendMessageA(listbox,LVM_SETITEM,0,(LPARAM)&lvitem);
			ii=-1;
		}
		else
			temp[ii]=*mbuf2;
		mbuf2++;
		ii++;
	}
}

static BOOL CALLBACK DlgProcUserDetails(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static char path[XFIRE_MAX_STATIC_STRING_LEN]="";
	static WCHAR wpath[256];
	static HICON gameicon=0;
	static HICON voiceicon=0;
	static HANDLE uhandle=0;
	static HWND listbox;
	LVCOLUMN pcol;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

			ghwndDlg=hwndDlg;

			listbox=GetDlgItem(hwndDlg,IDC_GAMEINFOLIST);
			pcol.mask=LVCF_WIDTH  | LVCF_SUBITEM | LVCF_TEXT;
			pcol.pszText="Key";
			pcol.cx=65;
			pcol.fmt=LVCFMT_LEFT;
			SendMessageA(listbox,LVM_INSERTCOLUMNA,1,(LPARAM)&pcol);
			pcol.cx=80;
			pcol.pszText="Value";
			SendMessageA(listbox,LVM_INSERTCOLUMNA,2,(LPARAM)&pcol);

			HFONT hFont;
			LOGFONT lfFont;

			memset(&lfFont, 0x00, sizeof(lfFont));
			memcpy(lfFont.lfFaceName, TEXT("Arial"), 8);

			lfFont.lfHeight   = 13;
			lfFont.lfWeight   = FW_BOLD;
			lfFont.lfCharSet  = ANSI_CHARSET;
			lfFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
			lfFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			lfFont.lfQuality  = DEFAULT_QUALITY;

			// Create the font from the LOGFONT structure passed.
			hFont = CreateFontIndirect (&lfFont);

			SendMessageA(listbox,WM_SETFONT,(WPARAM)hFont,TRUE);

			return TRUE;
		}
		case WM_CTLCOLORSTATIC:
			{
			break;
			}

		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom)
			  {
		        
			  case 0:
				{
				  switch (((LPNMHDR)lParam)->code)
				  {
		            
				  case PSN_INFOCHANGED:
					{
					  char* szProto;
					  HANDLE hContact = (HANDLE)((LPPSHNOTIFY)lParam)->lParam;
					  uhandle=hContact; //handle sichern

					  if (hContact == NULL)
						szProto = protocolname;
					  else
						szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

					  if (szProto == NULL)
						break;

					  //alle items aus der liste entfernen
					  SendMessage(listbox,LVM_DELETEALLITEMS,0,0);

					  if (hContact)
					  {
							DBVARIANT dbv;
							if(!DBGetContactSetting(hContact,protocolname,"Username",&dbv))
							{
								int usernamesize=strlen(dbv.pszVal)+1;
								char* username=new char[usernamesize];
								if(username)
								{
									strcpy_s(username,usernamesize,dbv.pszVal);
									mir_forkthread(LoadProfilStatus,(LPVOID)username);
								}
								//LoadProfilStatus
								DBFreeVariant(&dbv);
							}

							if(!DBGetContactSetting(hContact,protocolname,"GameInfo",&dbv))
							{
								setGameInfo(listbox,dbv.pszVal);
								DBFreeVariant(&dbv);
							}

							addToList(listbox,hContact,"Servername","ServerName");
							addToList(listbox,hContact,"GameType","GameType");
							addToList(listbox,hContact,"Map","Map");
							addToList(listbox,hContact,"Players","Players");		

							SetItemTxt(hwndDlg,IDC_DNICK,"Nick",hContact,0);
							SetItemTxt(hwndDlg,IDC_DUSERNAME,"Username",hContact,0);

							SetItemTxt(hwndDlg,IDC_GIP,"ServerIP",hContact,0);
							SetItemTxt(hwndDlg,IDC_VIP,"VServerIP",hContact,0);
							SetItemTxt(hwndDlg,IDC_GPORT,"Port",hContact,1);
							SetItemTxt(hwndDlg,IDC_VPORT,"VPort",hContact,1);

							SetItemTxt(hwndDlg,IDC_GAME,"RGame",hContact,0);
							SetItemTxt(hwndDlg,IDC_VNAME,"RVoice",hContact,0);

							//render icons
							{
								DBVARIANT dbv;

								if(!DBGetContactSetting(hContact,protocolname,"GameId",&dbv))
								{
									SendMessage(GetDlgItem(hwndDlg,IDC_GAMEICO),STM_SETICON,(WPARAM)xgamelist.iconmngr.getGameIcon(dbv.wVal),0);
									DBFreeVariant(&dbv);
								}							
								if(!DBGetContactSetting(hContact,protocolname,"VoiceId",&dbv))
								{
									SendMessage(GetDlgItem(hwndDlg,IDC_VOICEICO),STM_SETICON,(WPARAM)xgamelist.iconmngr.getGameIcon(dbv.wVal),0);
									DBFreeVariant(&dbv);
								}
								
								if(DBGetContactSetting(hContact,protocolname,"ServerIP",&dbv))
								{
									EnableWindow(GetDlgItem(hwndDlg,IDC_COPYGAME),FALSE);
									DBFreeVariant(&dbv);
								}
								if(DBGetContactSetting(hContact,protocolname,"VServerIP",&dbv))
								{
									EnableWindow(GetDlgItem(hwndDlg,IDC_COPYVOICE),FALSE);
									DBFreeVariant(&dbv);
								}
								
								//ShowWindow(GetDlgItem(hwndDlg,IDC_VOICEICO),FALSE)
							}
					  }
					}
					break;
				  }
				}
				break;
			  }
		}
		break;
		case WM_COMMAND:
			{
				switch(wParam)
				{
				case IDC_COPYGAME:
					GetIPPortUDetails(uhandle,"ServerIP","Port");
					break;
				case IDC_COPYVOICE:
					GetIPPortUDetails(uhandle,"VServerIP","VPort");
					break;
				}
			}
	}
	return FALSE;
}

/*static BOOL CALLBACK DlgProcUserDetails2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char profil[2056]="";
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			return TRUE;
		}
		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom)
			{
				case 0:
				{
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_INFOCHANGED:
						{
							char* szProto;
							HANDLE hContact = (HANDLE)((LPPSHNOTIFY)lParam)->lParam;

							if (hContact == NULL)
								szProto = protocolname;
							else
								szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

							if (szProto == NULL)
								break;

							if (hContact) {
								DBVARIANT dbv;
								
								char img[256]="";
								char username[256]="";
								char nick[256]="";
								char status[256]="";
								char game[512]="";
								if(!DBGetContactSetting(hContact,"ContactPhoto","File",&dbv))
								{
									snprintf(img,256,"<img src=\"%s\">",dbv.pszVal);
									DBFreeVariant(&dbv);
								}
								if(!DBGetContactSetting(hContact,protocolname,"Username",&dbv))
								{
									snprintf(username,256,"<b>Username:</b> %s<br>",dbv.pszVal);
									DBFreeVariant(&dbv);
								}
								if(!DBGetContactSetting(hContact,protocolname,"Nick",&dbv))
								{
									snprintf(nick,256,"<b>Nick:</b> %s<br>",dbv.pszVal);
									DBFreeVariant(&dbv);
								}
								if(!DBGetContactSetting(hContact,protocolname,"XStatusMsg",&dbv))
								{
									snprintf(status,256,"<b>Status:</b> %s<br>",dbv.pszVal);
									DBFreeVariant(&dbv);
								}
								if(!DBGetContactSetting(hContact,protocolname,"RGame",&dbv))
								{
									snprintf(game,512,"<fieldset style='border:1px solid #0091d5;background-color:#0d2c3e;margin-bottom:8px;'><legend>Spiel</legend><table><tr><td valign=top style='font-family:Arial;font-size:11px;color:#fff;'><b><u>%s</u></b></td></tr></table></fieldset>",dbv.pszVal);
									DBFreeVariant(&dbv);
								}
								snprintf(profil,2056,"mshtml:<div style='position:absolute;top:0;left:0;border:1px solid #0091d5;background-color:#000;padding:6px;width:334px;height:249px'><table><tr><td valign=top>%s</td><td valign=top style='font-family:Arial;font-size:11px;color:#fff;'>%s%s%s</td></tr><tr><td valign=top colspan=\"2\" style='font-family:Arial;font-size:11px;color:#fff;'>%s%s</td></tr></table></div>",img,username,nick,status,game);
								HWND hWnd = ::CreateWindow("AtlAxWin", profil,
								WS_CHILD|WS_VISIBLE, 0, 0, 334, 249, hwndDlg, NULL,
								::GetModuleHandle(NULL), NULL);
							}
						}
					}
				}
			}
		}
	}
	return FALSE;
}*/

int OnDetailsInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};

	if (!IsXFireContact((HANDLE)lParam))
		return 0;

	odp.cbSize = sizeof(odp);
	odp.hIcon = NULL;
	odp.hInstance = hinstance;
	odp.pfnDlgProc = DlgProcUserDetails;
	odp.position = -1900000000;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_UD);
	odp.pszTitle = Translate("XFire");
	odp.pszGroup = NULL;

	UserInfo_AddPage(wParam, &odp);

	return 0;
}