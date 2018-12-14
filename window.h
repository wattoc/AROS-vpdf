#ifndef WINDOW_MCC_H
#define WINDOW_MCC_H

#include "mcc/classes.h"

#define MUIA_VPDFWindow_ID           (MUIA_VPDFWindow_TagBase + 1)
#define MUIA_VPDFWindow_PDFDocument  (MUIA_VPDFWindow_TagBase + 2) // [..G]
#define MUIA_VPDFWindow_ActiveDocumentView  (MUIA_VPDFWindow_TagBase + 3) // [..G]

#define	MUIM_VPDFWindow_OpenFile	       (MUIM_VPDF_Dummy + 1)
#define	MUIM_VPDFWindow_CreateTab	       (MUIM_VPDF_Dummy + 2)
#define	MUIM_VPDFWindow_HandleAppMessage   (MUIM_VPDF_Dummy + 4)
#define	MUIM_VPDFWindow_UpdateActive       (MUIM_VPDF_Dummy + 5)
#define	MUIM_VPDFWindow_DetachView         (MUIM_VPDF_Dummy + 6)
#define	MUIM_VPDFWindow_AttachView         (MUIM_VPDF_Dummy + 7)
#define	MUIM_VPDFWindow_SaveFile	       (MUIM_VPDF_Dummy + 8)
#define	MUIM_VPDFWindow_CloseTab	       (MUIM_VPDF_Dummy + 9)

struct MUIP_VPDFWindow_OpenFile{ULONG MethodID; char *filename; int mode;};
struct MUIP_VPDFWindow_SaveFile{ULONG MethodID; char *filename; int mode;};
struct MUIP_VPDFWindow_CreateTab{ULONG MethodID;};
struct MUIP_VPDFWindow_CloseTab{ULONG MethodID;};
struct MUIP_VPDFWindow_UpdateActive{ULONG MethodID; int active;};
struct MUIP_VPDFWindow_DetachView{ULONG MethodID; int tabind; Object *docview; Object *title;};
struct MUIP_VPDFWindow_AttachView{ULONG MethodID; int tabind; Object *docview; Object *title;};

#define MUIV_VPDFWindow_OpenFile_CurrentTab 0
#define MUIV_VPDFWindow_OpenFile_NewTab 1
#define MUIV_VPDFWindow_OpenFile_CurrentTabIfEmpty 2

#define MUIV_VPDFWindow_DetachView_Active -1

#define MUIV_VPDFWindow_AttachView_CurrentTabIfEmpty -1

#if defined(__AROS__)
#define VPDFWindowObject BOOPSIOBJMACRO_START(getVPDFWindowClass())
#define VPDFTitleButtonObject BOOPSIOBJMACRO_START(getVPDFTitleButtonClass())
#else
#define	VPDFWindowObject   NewObject(getVPDFWindowClass(), NULL
#define	VPDFTitleButtonObject   NewObject(getVPDFTitleButtonClass(), NULL
#endif

DEFCLASS(VPDFWindow);
DEFCLASS(VPDFTitleButton);

#endif


