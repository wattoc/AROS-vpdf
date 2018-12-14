
#if defined(__AROS__)
#define MUIMASTER_YES_INLINE_STDARG
#include <clib/arossupport_protos.h>
#endif

/// System includes
#define AROS_ALMOST_COMPATIBLE
#include <proto/muimaster.h>
#include <libraries/mui.h>

#include <libraries/asl.h>
#include <workbench/workbench.h>

#include <proto/exec.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <exec/libraries.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/locale.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>

#include <proto/alib.h>
#include <proto/utility.h>

#include <proto/datatypes.h>
#include <proto/dtclass.h>
#include <datatypes/pictureclass.h>
#include <devices/rawkeycodes.h>

#include <libraries/gadtools.h>

#if !defined(__AROS__)
#include <emul/emulregs.h>
#include <emul/emulinterface.h>
#endif
////

#include <private/vapor/vapor.h>
#include "util.h"
#include "poppler.h"
#include "locale.h"
#include "application.h"
#include "window.h"
#include "settings.h"
#include "system/functions.h"

#include "mcc/documentview_class.h"
#include "mcc/documentlayout_class.h"
#include "mcc/toolbar_class.h"
#include "mcc/renderer_class.h"
#include "mcc/title_class.h"

#define DEBUG 1
#include <aros/debug.h>

#define MAXRECENT 10

struct Data
{
    unsigned int id;
    char muiid[64];
    Object *grpRoot;
    Object *grpTitles;
    Object *btnErrors;
    char title[256];
    int entries;
    STRPTR* titleNames;
};

DEFNEW
{
    Object *grpTitles, *grpRoot;
	STRPTR titleNames[2];
	int entries=0;
	titleNames[0]="Dummy";
	titleNames[1]=NULL; 
    obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_Title,"VPDF",
    MUIA_Window_Width, MUIV_Window_Width_Screen(50),
    MUIA_Window_Height, MUIV_Window_Height_Screen(50),
    MUIA_Window_AppWindow, TRUE,
    MUIA_Window_RootObject,
    grpRoot = VGroup,
    MUIA_Background, MUII_RegisterBack,
    MUIA_Frame, MUIV_Frame_Register,
    MUIA_Group_PageMode, TRUE,
    Child, (IPTR) (grpTitles = RegisterGroup(titleNames),
    	Child, (IPTR)(RectangleObject, End),
     End),
    End,
    TAG_MORE, INITTAGS);

	D(kprintf("HONK\n"));

    if (obj != NULL)
    {
        GETDATA;
        memset(data, 0, sizeof(struct Data));
        data->id = GetTagData(MUIA_VPDFWindow_ID, 0, INITTAGS);
        data->grpTitles = grpTitles;
		data->entries = entries;
		data->titleNames = titleNames;
		data->grpRoot = grpRoot;
        /* setup window id */

        DoMethod(obj, MUIM_Notify, MUIA_AppMessage, MUIV_EveryTime, MUIV_Notify_Application, 3, MUIM_VPDF_HandleAppMessage, data->id, MUIV_TriggerValue);
        //DoMethod(grpRoot, MUIM_Notify, MUIA_Group_ActivePage, MUIV_EveryTime, obj, 2, MUIM_VPDFWindow_UpdateActive, MUIV_TriggerValue);

        DoMethod(obj, MUIM_VPDFWindow_CreateTab);
    }

    return (IPTR)obj;
}

DEFMMETHOD(VPDFWindow_CreateTab)
{
    GETDATA;

    /* place empty object into tab */

    if (DoMethod(data->grpTitles, MUIM_Group_InitChange)) {
    	data->entries=data->entries+1;
    	STRPTR newTabs[data->entries+1];
    	int i=0;
    	D(kprintf("Entries in Register: %d\n",data->entries));
       	for (; i<data->entries-1; i++) newTabs[i]=data->titleNames[i];
    	newTabs[i] = (STRPTR)LOCSTR(MSG_NOFILE);
    	newTabs[i+1] = NULL;
	
		data->titleNames=newTabs;
	
		D(kprintf("data->titlenames[0] = %s\n",newTabs[0]));
	
		DoMethod(data->grpTitles, OM_ADDMEMBER, (IPTR) (RectangleObject,End));
	
    	DoMethod(data->grpTitles, MUIM_Group_ExitChange);
	
    	/* return index of new group member */
	
    	set(data->grpTitles, MUIA_Group_ActivePage, xget(data->grpTitles, MUIA_Family_ChildCount) - 1);
    } else 
    {
    	D(kprintf("couldn't initchange\n"));
    }
    return xget(data->grpTitles, MUIA_Family_ChildCount) - 1;
}

DEFMMETHOD(VPDFWindow_OpenFile)
{
    GETDATA;
    char *filename = msg->filename;
    int mode = msg->mode;
    int newtab;

    if (mode == MUIV_VPDFWindow_OpenFile_CurrentTabIfEmpty)
    {
        int tabind = xget(data->grpTitles, MUIA_Group_ActivePage);
        Object *tcontents = (Object*)DoMethod(data->grpTitles, MUIM_Family_GetChild, tabind + 1); // +1 for title object
        if (xget(tcontents, MUIA_UserData) == TRUE)	// marker for empty page. fix
            newtab = FALSE;
        else
            newtab = TRUE;
    }
    else if (mode == MUIV_VPDFWindow_OpenFile_NewTab)
    {
        newtab = TRUE;
    }
    else // if (MUIV_VPDFWindow_OpenFile_CurrentTab)
    {
        newtab = FALSE;
    }

    {
        Object *document;
        int tabind, i;
        Object *contents, *tcontents;

        if (newtab)
            tabind = DoMethod(obj, MUIM_VPDFWindow_CreateTab);
        else
            tabind = xget(data->grpTitles, MUIA_Group_ActivePage);
		
		data->titleNames[tabind]=filename;

        /* dispose old contents and add new browser */

        contents = (Object*)DoMethod(data->grpTitles, MUIM_Family_GetChild, tabind); /*  +1 for title object */
        DoMethod(contents, MUIM_Group_InitChange);
        tcontents = (Object*)DoMethod(contents, MUIM_Family_GetChild, 0);

        DoMethod(contents, OM_REMMEMBER, tcontents);
        MUI_DisposeObject(tcontents);

        tcontents = VGroup,
        Child, (IPTR) (document = DocumentViewObject,
               MUIA_DocumentView_Renderer, xget(_app(obj), MUIA_VPDF_Renderer),
               MUIA_DocumentView_FileName, (IPTR) filename,
               MUIA_DocumentView_Layout, xget(_app(obj), MUIA_VPDFSettings_Layout),
               MUIA_DocumentView_Outline, xget(_app(obj), MUIA_VPDFSettings_Outline),
               MUIA_DocumentLayout_Scaling, xget(_app(obj), MUIA_VPDFSettings_Scaling),
               End),
               End;

        if (tcontents != NULL)
            DoMethod(contents, MUIM_Family_AddTail, tcontents);

        set(contents, MUIA_UserData, FALSE); // mark as not empty
        DoMethod(contents, MUIM_Group_ExitChange);
        DoMethod(obj, MUIM_VPDFWindow_UpdateActive, tabind);
        return tcontents != NULL;
    }
}

DEFMMETHOD(VPDFWindow_UpdateActive)
{
    GETDATA;

    Object *group = (Object*)DoMethod(data->grpTitles, MUIM_Family_GetChild, msg->active + 1); /* +1 for title object */
    group = (Object*)DoMethod(group, MUIM_Family_GetChild, 0);

    if (group != NULL)
    {
        void *doc = (APTR) xget(group, MUIA_DocumentView_PDFDocument);
        struct pdfAttribute *attr = NULL;

        if (doc != NULL)
            attr = pdfGetAttr(doc, PDFATTR_TITLE);

        if (attr != NULL && attr->value.s != NULL && attr->value.s[0] != '\0')
            snprintf(data->title, sizeof(data->title), "VPDF [%s]", attr->value.s);
        else if (xget(group, MUIA_DocumentView_FileName))
            snprintf(data->title, sizeof(data->title), "VPDF [%s]", FilePart((char*)xget(group, MUIA_DocumentView_FileName)));
        else
            snprintf(data->title, sizeof(data->title), "VPDF");

        set(obj, MUIA_Window_Title, data->title);
        pdfFreeAttr(doc, attr);
    }

    return 0;
}

DEFMMETHOD(VPDFWindow_DetachView)
{
    GETDATA;

    if (msg->tabind == MUIV_VPDFWindow_DetachView_Active)
    {
        int tabind = xget(data->grpTitles, MUIA_Group_ActivePage);
        Object *contents = (Object*)DoMethod(data->grpTitles, MUIM_Family_GetChild, tabind);

        DoMethod(data->grpTitles, MUIM_Group_InitChange);

        DoMethod(data->grpTitles, OM_REMMEMBER, contents);

        DoMethod(data->grpTitles, MUIM_Group_ExitChange);

        msg->docview = contents;
        
        /* hmm, should we really close the window here? */

        if (xget(data->grpTitles, MUIA_Family_ChildCount) == 0)
            DoMethod(_app(data->grpTitles), MUIM_Application_PushMethod, obj, 3, MUIM_Set, MUIA_Window_CloseRequest, TRUE);

        return TRUE;
    }

    return FALSE;
}

DEFMMETHOD(VPDFWindow_AttachView)
{
    GETDATA;
    int tabind = msg->tabind;
    int newtab;
    Object *contents, *tcontents;

    if (tabind == MUIV_VPDFWindow_OpenFile_CurrentTabIfEmpty)
    {
        int tabind = xget(data->grpTitles, MUIA_Group_ActivePage);
        Object *tcontents = (Object*)DoMethod(data->grpTitles, MUIM_Family_GetChild, tabind); // +1 for title object
        if (xget(tcontents, MUIA_UserData) == TRUE)	// marker for empty page. fix
            newtab = FALSE;
        else
            newtab = TRUE;
    }
    else if (tabind == MUIV_VPDFWindow_OpenFile_NewTab)
    {
        newtab = TRUE;
    }
    else // if (MUIV_VPDFWindow_OpenFile_CurrentTab)
    {
        newtab = FALSE;
    }

    if (newtab)
        tabind = DoMethod(obj, MUIM_VPDFWindow_CreateTab);
    else
        tabind = xget(data->grpTitles, MUIA_Group_ActivePage);

	/* dispose old contents and add new browser */

    contents = (Object*)DoMethod(data->grpTitles, MUIM_Family_GetChild, tabind); /*  +1 for title object */
    DoMethod(data->grpTitles, MUIM_Group_InitChange);
    DoMethod(data->grpTitles, OM_REMMEMBER, contents);
    MUI_DisposeObject(contents);
    DoMethod(data->grpTitles, MUIM_Family_AddTail, msg->docview);
    DoMethod(data->grpTitles, MUIM_Group_ExitChange);

    DoMethod(obj, MUIM_VPDFWindow_UpdateActive, tabind);
    return TRUE;
}

DEFGET
{
    GETDATA;

    switch (msg->opg_AttrID)
    {
    case MUIA_VPDFWindow_ID:
        *(ULONG*)msg->opg_Storage = data->id;
        return TRUE;

    case MUIA_VPDFWindow_PDFDocument:
    {
        void *doc = NULL;
        Object *group = (Object*)DoMethod(data->grpTitles, MUIM_Family_GetChild, xget(data->grpTitles, MUIA_Group_ActivePage)); /* +1 for title object */
        group = (Object*)DoMethod(group, MUIM_Family_GetChild, 0);

        if (group != NULL)
            doc = (APTR) xget(group, MUIA_DocumentView_PDFDocument);

        *(ULONG*)msg->opg_Storage = (ULONG)doc;
        return TRUE;
    }

    case MUIA_VPDFWindow_ActiveDocumentView:
    {
        Object *group = (Object*)DoMethod(data->grpTitles, MUIM_Family_GetChild, xget(data->grpTitles, MUIA_Group_ActivePage)); /* +1 for title object */
        group = (Object*)DoMethod(group, MUIM_Family_GetChild, 0);

        if (group != NULL)
        {
            *(ULONG*)msg->opg_Storage = (ULONG)group;
            return TRUE;
        }
        break;
    }
    }

    return DOSUPER;
}

BEGINMTABLE
DECNEW
DECGET
DECMMETHOD(VPDFWindow_CreateTab)
DECMMETHOD(VPDFWindow_OpenFile)
DECMMETHOD(VPDFWindow_UpdateActive)
DECMMETHOD(VPDFWindow_DetachView)
DECMMETHOD(VPDFWindow_AttachView)

ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, VPDFWindowClass)

struct VPDFTitleButton_Data
{
    Object *obj;
};

BEGINMTABLE2(VPDFTitleButtonClass)
ENDMTABLE

DECSUBCLASS2_NC(MUIC_Group, VPDFTitleButtonClass)

