void ReadSettingBlob(HCONTACT hContact, char *ModuleName,
					 char *SettingName, WORD *pSize, void **pbBlob);
void FreeSettingBlob(WORD pSize,void * pbBlob);
BOOL ReadSettingBool(HCONTACT hContact,char *ModuleName,
					 char *SettingName,BOOL Default);
void WriteSettingBool(HCONTACT hContact,char *ModuleName,
					  char *SettingName,BOOL Value);
void WriteSettingIntArray(HCONTACT hContact,char *ModuleName,
					 char *SettingName,const int *Value, int Size);
bool ReadSettingIntArray(HCONTACT hContact,char *ModuleName,
				   char *SettingName,int *Value, int Size);

WORD ConvertHotKeyToControl(WORD HK);
WORD ConvertControlToHotKey(WORD HK);

typedef struct {
	void *ptrdata;
	void *next;
} TREEELEMENT;

void TreeAdd(TREEELEMENT **root,void *Data);
void TreeAddSorted(TREEELEMENT **root,void *Data,int (*CompareCb)(TREEELEMENT*,TREEELEMENT*));
void TreeDelete(TREEELEMENT **root,void *Item);
void *TreeGetAt(TREEELEMENT *root,int Item);
int TreeGetCount(TREEELEMENT *root);

static void __inline SAFE_FREE(void** p)
{
	if (*p)
	{
		free(*p);
		*p = NULL;
	}
}
