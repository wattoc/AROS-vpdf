#ifndef PRINTER_MCC_H
#define PRINTER_MCC_H

#include "mcc/classes.h"


#define	MUIM_VPDFPrinter_PrintDocument	       (MUIM_VPDFPrinter_Dummy + 1)
#define	MUIM_VPDFPrinter_Start			       (MUIM_VPDFPrinter_Dummy + 2)
#define MUIM_VPDFPrinter_Stop                  (MUIM_VPDFPrinter_Dummy + 3)
#define MUIM_VPDFPrinter_Done                  (MUIM_VPDFPrinter_Dummy + 4)
#define MUIM_VPDFPrinter_StatusUpdate          (MUIM_VPDFPrinter_Dummy + 5)
#define MUIM_VPDFPrinter_Close                 (MUIM_VPDFPrinter_Dummy + 6)
#define MUIM_VPDFPrinter_PrinterPrefs          (MUIM_VPDFPrinter_Dummy + 7)
#define MUIM_VPDFPrinter_HideCollate  	       (MUIM_VPDFPrinter_Dummy + 8)
#define MUIM_VPDFPrinter_Error                 (MUIM_VPDFPrinter_Dummy + 9)


#define MUIA_VPDFPrinter_PSName  	           (MUIM_VPDFPrinter_Dummy + 20)
#define MUIA_VPDFPrinter_PrintingMode  	       (MUIM_VPDFPrinter_Dummy + 21)
#define MUIA_VPDFPrinter_PSMode  	           (MUIM_VPDFPrinter_Dummy + 22)

struct MUIP_VPDFPrinter_PrintDocument{IPTR MethodID; APTR doc;};
struct MUIP_VPDFPrinter_Start{IPTR MethodID;};
struct MUIP_VPDFPrinter_Stop{IPTR MethodID;};
struct MUIP_VPDFPrinter_Done{IPTR MethodID;};
struct MUIP_VPDFPrinter_StatusUpdate{IPTR MethodID; ULONG page_number; ULONG actual_page;};
struct MUIP_VPDFPrinter_Close{IPTR MethodID;};
struct MUIP_VPDFPrinter_PrinterPrefs{IPTR MethodID;};
struct MUIP_VPDFPrinter_HideCollate{IPTR MethodID; ULONG copies;};
struct MUIP_VPDFPrinter_Error{IPTR MethodID;};

#if defined(__AROS__)
#define VPDFPrinterObject BOOPSIOBJMACRO_START(getVPDFPrinterClass())
#else
#define	VPDFPrinterObject   NewObject(getVPDFPrinterClass(), NULL
#endif

DEFCLASS(VPDFPrinter);

#endif
