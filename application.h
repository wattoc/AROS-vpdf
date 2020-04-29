#ifndef APPLICATION_MCC_H
#define APPLICATION_MCC_H

#include "mcc/classes.h"

#define MUIA_VPDF_Renderer           (MUIA_VPDF_TagBase + 1)
#define MUIA_VPDF_Locked             (MUIA_VPDF_TagBase + 2)

#define	MUIM_VPDF_OpenFile	     (MUIM_VPDF_Dummy + 1)
#define	MUIM_VPDF_CreateTab	     (MUIM_VPDF_Dummy + 2)
#define	MUIM_VPDF_CreateWindow 	     (MUIM_VPDF_Dummy + 3)
#define	MUIM_VPDF_HandleAppMessage   (MUIM_VPDF_Dummy + 4)
#define	MUIM_VPDF_RequestFile        (MUIM_VPDF_Dummy + 5)
#define	MUIM_VPDF_OpenRecentFile     (MUIM_VPDF_Dummy + 6)
#define	MUIM_VPDF_FindWindowByID     (MUIM_VPDF_Dummy + 7)
#define	MUIM_VPDF_FindActiveWindow   (MUIM_VPDF_Dummy + 8)
#define	MUIM_VPDF_CloseWindow        (MUIM_VPDF_Dummy + 9)
#define	MUIM_VPDF_LogMessage         (MUIM_VPDF_Dummy + 10)
#define	MUIM_VPDF_NewActiveWindow    (MUIM_VPDF_Dummy + 11)
#define	MUIM_VPDF_SettingsUse        (MUIM_VPDF_Dummy + 12)
#define	MUIM_VPDF_SettingsSave       (MUIM_VPDF_Dummy + 13)
#define	MUIM_VPDF_SettingsCancel     (MUIM_VPDF_Dummy + 14)
#define	MUIM_VPDF_SettingsApply      (MUIM_VPDF_Dummy + 15)
#define	MUIM_VPDF_About              (MUIM_VPDF_Dummy + 16)
#define	MUIM_VPDF_PrintDocument      (MUIM_VPDF_Dummy + 17)
#define	MUIM_VPDF_SelectionCopy      (MUIM_VPDF_Dummy + 18)
#define	MUIM_VPDF_SaveFile	     (MUIM_VPDF_Dummy + 19)
#define	MUIM_VPDF_CloseTab	     (MUIM_VPDF_Dummy + 20)

struct MUIP_VPDF_OpenFile{IPTR MethodID; int windowid; char *filename; int mode;};  // mode is MUIV_VPDFWindow_OpenFile_XXX
struct MUIP_VPDF_SaveFile{IPTR MethodID; int windowid; char *filename; int mode;};  // mode is MUIV_VPDFWindow_OpenFile_XXX
struct MUIP_VPDF_CreateTab{IPTR MethodID; int windowid;};
struct MUIP_VPDF_CreateWindow{IPTR MethodID;};

struct MUIP_VPDF_CloseTab{IPTR MethodID; int windowid;};


struct MUIP_VPDF_RequestFile{IPTR MethodID; int mode; char *initialfile; char *initialdir; char *initialpath;};
struct MUIP_VPDF_OpenRecentFile{IPTR MethodID; int windowid; int idx;};
struct MUIP_VPDF_FindWindowByID{IPTR MethodID; int windowid;};
struct MUIP_VPDF_FindActiveWindow{IPTR MethodID;};
struct MUIP_VPDF_HandleAppMessage{IPTR MethodID; int windowid; APTR appmessage;};
struct MUIP_VPDF_CloseWindow{IPTR MethodID; int windowid;};
struct MUIP_VPDF_LogMessage{IPTR MethodID; int severity; char *message;};
struct MUIP_VPDF_NewActiveWindow{IPTR MethodID; Object *window;};
struct MUIP_VPDF_SettingsUse{IPTR MethodID;};
struct MUIP_VPDF_SettingsSave{IPTR MethodID;};
struct MUIP_VPDF_SettingsCancel{IPTR MethodID;};
struct MUIP_VPDF_SettingsApply{IPTR MethodID; int sgroup;};
struct MUIP_VPDF_About{IPTR MethodID; int sgroup;};
struct MUIP_VPDF_PrintDocument{IPTR MethodID;};
struct MUIP_VPDF_SelectionCopy{IPTR MethodID;};

#define MUIV_VPDF_RequestFile_Load 1
#define MUIV_VPDF_RequestFile_Save 2
#define MUIV_VPDF_RequestFile_DirectoryOnly 4

#define MUIV_VPDF_FindWindowByID_Active 0
#define MUIV_VPDF_FindWindowByID_First -1

#define MUIV_VPDF_LogMessage_Message 0
#define MUIV_VPDF_LogMessage_Warning 1
#define MUIV_VPDF_LogMessage_Error 2

#if defined(__AROS__)
#define VPDFObject BOOPSIOBJMACRO_START(getVPDFClass())
#else
#define	VPDFObject   NewObject(getVPDFClass(), NULL
#endif
DEFCLASS(VPDF);

enum
{
	ID_VPDF_RECENT0 = 1000 /* keep it last entry */
};

#endif


