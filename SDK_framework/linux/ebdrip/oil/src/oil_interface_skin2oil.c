/* Copyright (C) 2007-2012 Global Graphics Software Ltd. All rights reserved.
 *
 * This example is provided on an "as is" basis and without
 * warranty of any kind. Global Graphics Software Ltd. does not
 * warrant or make any representations regarding the use or results
 * of use of this example.
 *
 * $HopeName: SWebd_OIL_example_gg!src:oil_interface_skin2oil.c(EBDSDK_P.1) $
 *
 */

/*! \file
 *  \ingroup OIL
 *  \brief This file defines the callback functions supplied to the Skin
 *  by the OIL.
 *
 * These functions allow the Skin and RIP to pass data to the OIL.
 * As well as passing raster data to the OIL, the Skin may pass text messages,
 * via OIL_MonitorCallback,
 */

#include "oil.h"
#include "oil_malloc.h"
#include "oil_interface_skin2oil.h"
#include "oil_interface_oil2pms.h"
#include "oil_page_handler.h"
#include "oil_probelog.h"
#include "oil_pjl.h"
#include "oil_utils.h"
#ifndef PMS_OIL_MERGE_DISABLE_JS
#include "gw_gps.h"
#endif

#include <stdio.h>
#include <string.h>     /* for memset and memcpy */
#include "pms_export.h"

#ifdef DIRECTPRINTPCLOUT

#include "oil_entry.h"
#include "oil_pcl5rast.h"
#include "oil_pcl6rast.h"
#endif
#ifndef PMS_OIL_MERGE_DISABLE_JS
#ifdef USE_PJL
#undef USE_PJL
#endif
#endif
#define INCH_TO_0DOT1MM_FACTOR 254 /* (25.4*10) TO CONVERT INCH TO 0.1MM  */
#define LEFT_MARGIN 100
#define RIGHT_MARGIN 100
#define TOP_MARGIN 100
#define BOTTOM_MARGIN 100

/* extern variables */
extern OIL_TyPage *g_pstCurrentPage;
extern OIL_TyJob *g_pstCurrentJob;
extern OIL_TyConfigurableFeatures g_ConfigurableFeatures;
extern OIL_TySystem g_SystemState;
extern unsigned char *gFrameBuffer;
extern void OIL_JobCancel(void);
extern PMS_TyBandPacket *ptBandPacket;
extern int bc_in_createbandpacket;

#ifndef PMS_OIL_MERGE_DISABLE_JS
extern gwmsg_client_t     *gps_client ;
extern PlotSet_plotid;
extern gps_color_rid_t rID[]; //Updating ra_ variables from Color_getRID()......
#endif
void DummyPageforDuplex(void *pJobContext);
void BlankBandsToPMS(void *pJobContext, struct rasterDescription *ptRasterDescription);
unsigned int gps_frameid;
/* save last value for end of job checking */
static BOOL l_PageDuplex = 0;
#ifdef DIRECTPRINTPCLOUT
static PMS_TySystem pmsSysInfo;
#endif

/**
 * Raster layout data. If PMS provides band memory for the rip, we store
 * the details here.
 */
static RASTER_LAYOUT sRasterLayout;

extern gwmsg_GpsPage_FrameGetBand_Res_t pFGBR;
  gwmsg_GpsPage_FrameFlushBand_Res_t pFFBR;
  gps_pageinfo2_t       pageinfo2;
  gps_pagemode2_t       pagemode2;
  gps_pagemode_t        pagemode;
  gps_pageinfo_t        pageinfo;


/**
 * \brief Callback for the Skin to provide text messages to the OIL.
 *
 * This function provides a mechanism for the Skin to provide text data to the OIL.
 * Typically, this data consists of messages from the interpreter providing errors or
 * warnings about the job, or diagnostic information from the interpreter or job.
 * \param[in]  nBufLen The length of the string data in pBuffer.
 * \param[in]  pBuffer A string buffer containing the data to be output.
 */
void OIL_MonitorCallback(int nBufLen, unsigned char * pBuffer)
{
  int iIndex;
  char ch;
  static char linebuff[LINEBUFF_LEN+1];
  static int linebufflen = 0;

  /* Skip UTF-8 BOM if provided */
  if ( nBufLen >= 3 && pBuffer[0] == 0xef && pBuffer[1] == 0xbb && pBuffer[2] == 0xbf ) {
    nBufLen -= 3 ;
    pBuffer += 3 ;
  }

  for (iIndex = 0; iIndex < nBufLen; iIndex++)
  {
    ch = (char)pBuffer[iIndex];
    linebuff[linebufflen++] = ch;

    if (ch == '\n' || linebufflen == LINEBUFF_LEN)
    {
      if ( linebufflen > 0 )
      {
        linebuff[linebufflen] = 0;
        PMS_PutDebugMessage(linebuff);
        linebufflen = 0;
      }
    }
  }
}



/**
 * \brief A wrapper function which calls through to the corresponding
 * PMS function, \see PMS_RasterLayout.
 */

#ifdef GG_64395
int32 OIL_RasterLayout(RASTER_LAYOUT* pLayout)
{
  int32 result = PMS_RasterLayout( pLayout->page.width,
                                   pLayout->page.height,
                                   pLayout->page.bit_depth,
                                   pLayout->page.components,
                                   &pLayout->valid,
                                   &pLayout->line_bytes,
                                   &pLayout->line_stride,
                                   &pLayout->colorant_stride );

  /* Store a copy of the specified layout. */
  sRasterLayout = *pLayout;
  return result;
}
#else
int32 RIPCALL OIL_RasterStride(void *pJobContext,
                               unsigned int *puStride)
{
  UNUSED_PARAM(void *, pJobContext);

  *puStride = (*puStride+3) & ~3;  /* 4 byte align */
  /* needs to be set as used in the RasterCallback function */
  sRasterLayout.colorant_stride = 0;
  return 0;
}
#endif
/**
 * \brief A function which theoretically should call through to the corresponding
 * PMS function, \see PMS_RasterRequirements, if necessary.
 *
 * We save the raster requirements data in order to make a call from
 * the OIL_RasterDestination function when the first band buffer is called for.
 * This filters out the extra calls that occur with the
 * flag set such as via the Partial Paint mechanism which temporarily stores data
 * in the band memory that has been allocated by PMS.
 *
 */
static RASTER_REQUIREMENTS LocalRasterRequirements;

int32 RIPCALL OIL_RasterRequirements(void *pJobContext,
                                     RASTER_REQUIREMENTS *pRasterRequirements,
                                     int32 fRenderingStarting)
{
  UNUSED_PARAM(void *, pJobContext);
  pRasterRequirements->have_framebuffer = ((g_tSystemInfo.eBandDeliveryType == PMS_PUSH_BAND_DIRECT_FRAME) || (g_tSystemInfo.eBandDeliveryType == PMS_PUSH_BAND_DIRECT_SINGLE));
  printf("\nIn OIL_RasterRequirements \n");
  /* We save the required raster requirements data for later use*/
  if ( fRenderingStarting == 1 )
    LocalRasterRequirements = *pRasterRequirements;
  else {
    /**
     * The 'have_framebuffer' argument does not actually mean what it says.
     * It equates to "something other that the RIP core will be allocating
     * and managing RIP band buffers". So we have to set it if the final
     * OIL/PMS client code is providing a buffer, whether that be a band or
     * frame buffer. In the case that OIL/PMS is provding just a single band
     * buffer, someone has to provide a mechanism for partial-paint to be able
     * write and read-back an arbitrary band. Well the skin code in pgbdev.c
     * already has logic to do this, but disables it if it sees that
     * have_framebuffer is true, as it assumes this means that OIL/PMS has
     * a framebuffer. We need a second line of communication between the
     * OIL/PMS layer and the skin to say that OIL/PMS is only provding a
     * single band, and the skin code needs to allocate and manage bands to
     * ensure Partial-paint read-back works.
     * To prove the point and allow testing, just abuse the have_framebuffer
     * Bool between OIL/PMS and skin, and stick an extra flag into it to
     * indicate OIL/PMS is just allocating a single buffer.
     *
     * \todo BMJ 04-Dec-13 : Fix and rename have_framebuffer between OIL/PMS
     *                       and skin.
     */
    if ( g_tSystemInfo.eBandDeliveryType == PMS_PUSH_BAND_DIRECT_SINGLE )
      ((long *)(&pRasterRequirements->have_framebuffer))[0] |= 0x10000;
  }
  return 0;
}

/**
 * \brief A wrapper function which calls through to the corresponding
 * PMS function, \see PMS_RasterDestination.
 * The PMS_RasterRequirements function is called for the first band
 * as part of the filtering process to remove unwanted calls to PMS.
 */
int32 RIPCALL OIL_RasterDestination(void *pJobContext,
                                    RASTER_DESTINATION * pRasterDestination,
                                    int32 nFrameNumber)
{
  int32 nRetVal = 0;
  RASTER_REQUIREMENTS *pRasterRequirements = &LocalRasterRequirements;
  OIL_eBandDeliveryType bdt = g_tSystemInfo.eBandDeliveryType;

  UNUSED_PARAM(void *, pJobContext);

  if ( (gFrameBuffer == NULL) && (bdt == PMS_PUSH_BAND_DIRECT_SINGLE ||
                                  bdt == PMS_PUSH_BAND_DIRECT_FRAME) ) {
    nRetVal = PMS_RasterRequirements(pRasterRequirements->page_width,
                                     pRasterRequirements->page_height,
                                     pRasterRequirements->bit_depth,
                                     pRasterRequirements->components,
                                     pRasterRequirements->components_per_band,
                                     pRasterRequirements->minimum_bands,
                                     pRasterRequirements->band_height,
                                     pRasterRequirements->line_bytes,
                                     1,
                                     &pRasterRequirements->have_framebuffer );
  }
  if(nRetVal == 0)
  {
    nRetVal = PMS_RasterDestination(nFrameNumber, pRasterDestination->bandnum,
                                    &pRasterDestination->memory_base,
                                    &pRasterDestination->memory_ceiling);
  }
  return nRetVal;
}

 void set_paper_type()
{
  
  if( strcmp( (const char *)  g_pstCurrentJob->tCurrentJobMedia.szMediaType, "Thick" ) == 0 )
  {
    pageinfo.paper_type = GPS_PAPER_MIDDLETHICK ;
  }
  else if( strcmp( (const char *) g_pstCurrentJob->tCurrentJobMedia.szMediaType, "Thin" ) == 0 )
  {
    pageinfo.paper_type = GPS_PAPER_THIN;
  }
  else if( strcmp( (const char *)  g_pstCurrentJob->tCurrentJobMedia.szMediaType, "Bond" ) == 0 )
  {
   pageinfo.paper_type  = GPS_PAPER_BOND;
  }
  else if( strcmp( (const char *)  g_pstCurrentJob->tCurrentJobMedia.szMediaType, "Label" ) == 0 )
  {
    pageinfo.paper_type  = GPS_PAPER_LABEL;
  }
  else if( strcmp( (const char *)  g_pstCurrentJob->tCurrentJobMedia.szMediaType, "Transparency" ) == 0 )
  {
   pageinfo.paper_type = GPS_PAPER_OHP;
  }
  else if( strcmp( (const char *)  g_pstCurrentJob->tCurrentJobMedia.szMediaType, "Envelope" ) == 0 )
  {
    pageinfo.paper_type = GPS_PAPER_ENVELOPE;
  }
  else if( strcmp( (const char *) g_pstCurrentJob->tCurrentJobMedia.szMediaType, "Preprinted" ) == 0 )
  {
    pageinfo.paper_type = GPS_PAPER_PREPRINT;
  }
  else if( strcmp( (const char *)  g_pstCurrentJob->tCurrentJobMedia.szMediaType, "Letterhead" ) == 0 )
  {
    pageinfo.paper_type  = GPS_PAPER_LETTER_HEAD;
  }
  else if( strcmp( (const char *)  g_pstCurrentJob->tCurrentJobMedia.szMediaType, "Recycled" ) == 0 )
  {
    pageinfo.paper_type  = GPS_PAPER_RECYCLE;
  }
  else
  {
    pageinfo.paper_type = GPS_PAPER_AUTO;
  }

 }

/**
 * \brief Callback for Skin to provide raster data to the OIL.
 *
 * This is the crucial mechanism whereby raster data is provided to the OIL.
 *
 * If this is the first call to this function for a given page, memory is allocated
 * for the page structure and CreateOILPage() is called to fill populate it.
 * If a page already exists, the raster data is added to that page.
 *
 * When a complete page is available, a PMS page structure is created and populated
 * by calling CreatePMSPage(). The PMS is then notified of the completed page by
 * calling PMS_CheckinPage().
 *
 * <I>The following raster formats are supported: </I>\n
 * Colorant Family: DeviceGray, DeviceCMYK, DeviceRGB. \n
 * Interleaving Style:  for DeviceRGB - 3(Band) and 4(Frame), for DeviceCMYK - 3(Band)
 * and 4(Frame), for DeviceGray - 1(Pixel), 3(Band) and 4(Frame). \n
 * Bits per pixel: 1, 2, 4, 8 and 16. \n
 * Separations are also supported. \n
 * Colorant data can be submitted in any order; for example, magenta before cyan.\n
 * Hardware resolution: all. \n
 *
 * <B>Example</B> Configuration of separated CMYK colored job with band interleaving, 8 bpp, 300dpi  \n
 * and colorants in the order - M C K Y : \n
 * <pre>
 * <<
 *   /HWResolution [ 300 300 ]
 *   /Separations true
 *   /InterleavingStyle 3
 *   /ProcessColorModel /DeviceCMYK
 *   /SeparationDetails
 *   <<
 *     /SeparationStyle 2
 *     /SeparationOrder [ /Magenta /Cyan /Black /Yellow]
 *   >>
 *   /ValuesPerComponent 256
 * >> setpagedevice
 * </pre>
 *
 * \param[in]   pJobContext         Unused parameter.  A pointer to the job context data.
 * \param[in]   ptRasterDescription A pointer to the raster description structure for the current page.
 * \param[in]   pBuffer             A pointer to the buffer containing the raster data.  A null pointer
 *                                  indicates that the current page is complete.
 * \return      Returns TRUE on successful completion; FALSE if any errors are encountered.
 */
int32 RIPCALL OIL_RasterCallback(void *pJobContext,
                                 RasterDescription *ptRasterDescription,
                                 unsigned char * pBuffer )
{
  static int iBandNumber = 0, nColorFamilyOffset;
  static int nBytesPerLineSrc, nRemainingLinesToWrite, nLinesAlreadyWritten = 0, nLinesInFrame;
  static int nColorsInThisBand = 1, nFramesToWrite = 1, nLinesInSrcBand;
  unsigned char *pBandBuffer = NULL, *pDestPtr = NULL, *pSrcPtr;
  static int nOutputBytesPerLine, nOutputLinesPerBand = 200;
  static int nOutputPixelsPerLineIncRIPPadding, nOutputPixelsPerLineNoPadding;
  // static PMS_TyBandPacket *ptBandPacket, *ptCurrentBandPacket;
  static PMS_TyBandPacket *ptCurrentBandPacket;
  int y;
  unsigned char *pTemp =NULL,*pTemp1=NULL;
  PMS_TyBandInfo stPMSBandInfo;
  static OIL_eTyRasterState stRasterNextState = OIL_Ras_ProcessNewPage;
  static BOOL bPartialBand = FALSE;
  int nColorantOffset, nLinesThisTime, i, line, nColorants;
  RasterColorant *ptColor;
  static short Map[OIL_MAX_PLANES_COUNT];
  static int j = 0;
  int x, a;
#ifdef USE_64
  ras64 *pDestPtr64;
#else
#endif
  ras32 *pDestPtr32;
  unsigned char *p, *pDst8;
  int nRIPDepth;
  int nOutputDepth;
  static int color = 1;

#ifndef PMS_OIL_MERGE_DISABLE_JS
  int	FGband_frameid=1, FGband_bandid=0, FGband_planeid=0;
  int FFB_frameid=1, FFB_bandid=0, FFB_planeid=0, FFB_drawn=1, FFB_flush=0;
  int    FP_frameid=1;
  unsigned long FP_flag;
  gwmsg_GpsPage_FrameCreate_Res_t	pFCR;
  int                *framecreate_result;

  int    GPSFrameGetBandResretval;
  int    GPSFrameCreateExretval;
  int    GPSFrameFlushBandResRetVal;
  int    GPSFramePrintInforetval;
  int    GPSFrameCreateInfoResretval;
  int prevband=0; //VENKAT ADDED
  int GPSMaxBandNum= 0;

  #endif


#ifdef DIRECTPRINTPCLOUT
  static RASTER_handle rasterFile = NULL;
  static int32 nLinesToWrite;   /* for this frame */
  static int32 nLinesPerFrame;

  static int32 channelsPerBand, bytesPerLine, bytesToWritePerLine;
  static int32 channelBase ;
  int32 nErr;
#endif
  UNUSED_PARAM(void *, pJobContext);

  OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_ENTER, (intptr_t)0);

  if (pBuffer==NULL) /*indicates job finished*/
  {
    /*Reset*/
    iBandNumber = 0;
    stRasterNextState = OIL_Ras_ProcessNewPage;
#ifdef DIRECTPRINTPCLOUT
    if(pmsSysInfo.eOutputType == PMS_DIRECT_PRINT)
          PCL5_RASTER_job_end(&rasterFile);
    else if(pmsSysInfo.eOutputType == PMS_DIRECT_PRINTXL)
          PCL6_RASTER_job_end(&rasterFile);
    else
#endif
    {
      DummyPageforDuplex(pJobContext);
      PMS_CheckinPage(NULL);
    }
#ifdef USE_PJL
    OIL_ResetPJLAppData();
#endif
    OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)1);
    return TRUE; /* job finished */
  }

#ifdef LOWBYTEFIRST
  if((ptRasterDescription->rasterDepth == 1) || (ptRasterDescription->rasterDepth == 2) || (ptRasterDescription->rasterDepth == 4))
  {
    if((g_SystemState.nOutputType != PMS_NONE) || (g_ConfigurableFeatures.bRasterByteSWap)) /* theoretically should not use PMS_NONE */
    {
      int nBytesPerLine = (ptRasterDescription->dataWidth >> 3);
      int nLinesPerBand = ptRasterDescription->bandHeight;
      int Colorants = ptRasterDescription->numChannels;
      int nTotalWords = (nBytesPerLine >> 2U) * nLinesPerBand * Colorants;

      OIL_SwapBytesInWord(pBuffer, nTotalWords);
    }
  }
#endif

  while (1)
  {
    switch(stRasterNextState)
    {
    case OIL_Ras_ProcessNewPage: /*will enter here at the start of each page*/
      {
         printf("\nIn case OIL_Ras_ProcessNewPage \n");
        /* check for unsupported rasterdepth*/
        if((ptRasterDescription->rasterDepth != 1)
          && (ptRasterDescription->rasterDepth != 2)
          && (ptRasterDescription->rasterDepth != 4)
          && (ptRasterDescription->rasterDepth != 8)
          && (ptRasterDescription->rasterDepth != 16))
        {
          HQFAILV((" %d bpp output is not supported.", ptRasterDescription->rasterDepth));
          OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)0);
          return FALSE;
        }
#ifdef DIRECTPRINTPCLOUT
        PMS_GetSystemInfo(&pmsSysInfo, 0);
        if((pmsSysInfo.eOutputType != PMS_DIRECT_PRINT) && (pmsSysInfo.eOutputType != PMS_DIRECT_PRINTXL))
#endif
        {
          /* reset page-level parameters */
          iBandNumber = 0;

          /* create oil page skeleton */
          g_pstCurrentPage = CreateOILPage(ptRasterDescription);
          /* save for checking at the end of the job for odd pages */
          l_PageDuplex = g_pstCurrentPage->bDuplex;
          /* initialize some page-level parameters */
          nLinesInFrame = g_pstCurrentPage->nPageHeightPixels;
          nRemainingLinesToWrite = nLinesInFrame;
          nFramesToWrite = ptRasterDescription->nSeparations;

          /* some colors may be omitted or may come in different order. create array for mapping colors to correct planes*/
          ptColor = ptRasterDescription->pColorants;

          /* initialize mapping table */
          for(i=0; i< OIL_MAX_PLANES_COUNT; i++)
            Map[i] = OIL_InvalidColor;

          nColorants = 0;
          if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_CMYK)
          {
            while(ptColor != NULL)
            {
              if(!strcmp((char*)ptColor->colorantName, "Cyan"))
                Map[ptColor->nChannel] = 0;
              else if(!strcmp((char*)ptColor->colorantName, "Magenta"))
                Map[ptColor->nChannel] = 1;
              else if(!strcmp((char*)ptColor->colorantName, "Yellow"))
                Map[ptColor->nChannel] = 2;
              else if(!strcmp((char*)ptColor->colorantName, "Black"))
                Map[ptColor->nChannel] = 3;
              else if(!strcmp((char*)ptColor->colorantName, "Gray"))
                Map[ptColor->nChannel] = 3;
              else
              {
                HQFAILV(("Colorant %s not supported", (char*)ptColor->colorantName));
              }
              ptColor = ptColor->pNext;
              nColorants++;
            }
            nColorFamilyOffset = CMYK_OFFSET;
          }
          else if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_RGB)
          {
            while(ptColor != NULL)
            {
              if(!strcmp((char*)ptColor->colorantName, "Red"))
                Map[ptColor->nChannel] = 0;
              else if(!strcmp((char*)ptColor->colorantName, "Green"))
                Map[ptColor->nChannel] = 1;
              else if(!strcmp((char*)ptColor->colorantName, "Blue"))
                Map[ptColor->nChannel] = 2;
              else
              {
                HQFAILV(("Colorant %s not supported", (char*)ptColor->colorantName));
              }
              ptColor = ptColor->pNext;
              nColorants++;
            }
            nColorFamilyOffset = RGB_OFFSET;
          }
          else if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_Gray)
          {
            while(ptColor != NULL)
            {
              if(!strcmp((char*)ptColor->colorantName, "Gray"))
                Map[ptColor->nChannel] = 3;
              else
              {
                HQFAILV(("Colorant %s not supported", (char*)ptColor->colorantName));
              }
              ptColor = ptColor->pNext;
              nColorants++;
            }
            nColorFamilyOffset = GRAY_OFFSET;
          }
          else
          {
            HQFAILV(("ColorantFamily %s not supported", (char*)ptRasterDescription->colorantFamily));
            OIL_JobCancel();
            OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)0);
            DeleteOILPage(g_pstCurrentPage);
            return FALSE ;
          }
          /* at this point nColorants contains number of colorants actually present */

          if(nColorants == 0)
          {
            /* the page is blank. simply return. Blank pages will be handled later in SubmitPageToPMS */
            /* Since we will be returning TRUE, we may get more calls to OIL_RasterCallback. Hence change
            the state before returning to avoid multiple calls to CreateOILPage causing memory leak */
            stRasterNextState = OIL_Ras_ProcessBands;
          }

          nColorsInThisBand = ptRasterDescription->numChannels;

          if(ptRasterDescription->nSeparations == 1) /*composite job*/
          {
            /* for composite job iteration variable nColorants contains number of colorants actually present */
            /* Note: for separations job g_pstCurrentPage->nColorants is already set to nSeparations in CreateOilPage */
            g_pstCurrentPage->nColorants = nColorants;
            if(ptRasterDescription->interleavingStyle == interleavingStyle_frame)
            {
              /* If it is frame interleaved, all single color frames will come before frames of next colorant */
              nColorsInThisBand = 1;
              nFramesToWrite = ptRasterDescription->numChannels;
            } else if(ptRasterDescription->interleavingStyle == interleavingStyle_pixel) {
              nColorsInThisBand = 1; /* Really means number of color bands in this band rather than different colors */
              nFramesToWrite = 1;
            }
          }

          /* Raster data from core is 32 bit aligned and padded to 32 bits */
          nBytesPerLineSrc = (g_pstCurrentPage->nRasterWidthData >> 3);
          nOutputBytesPerLine = nBytesPerLineSrc;
          nRIPDepth = g_pstCurrentPage->uRIPDepth;
          nOutputDepth = g_pstCurrentPage->uOutputDepth;

          nOutputPixelsPerLineIncRIPPadding = g_pstCurrentPage->nRasterWidthData / g_pstCurrentPage->uRIPDepth;
          nOutputPixelsPerLineNoPadding = g_pstCurrentPage->nPageWidthPixels;


          if(nRIPDepth != nOutputDepth) {
            if(!((nRIPDepth==1) && (nOutputDepth==8))) {
              HQFAILV(("Bit depth conversion, %d to %d, is not supported. Setting output depth same as RIP.\n", nRIPDepth, nOutputDepth));
              nOutputDepth = nRIPDepth;
              g_pstCurrentPage->uOutputDepth = nRIPDepth;
            }
          }

          /* determine bandheight and bandwidth requested by pms */
          GetBandInfoFromPMS(nOutputPixelsPerLineIncRIPPadding, g_pstCurrentPage->nPageHeightPixels,
            nOutputDepth, &stPMSBandInfo);

/****************pageinfo2 struc initialization*************/

 pageinfo2.paper_width = ceil(((float)( g_pstCurrentPage -> nPageWidthPixels + LEFT_MARGIN+ RIGHT_MARGIN)*INCH_TO_0DOT1MM_FACTOR )/g_pstCurrentJob->uXResolution);
 //pageinfo2.paper_length= ((g_pstCurrentPage -> nPageHeightPixels * 25.4)/g_pstCurrentJob->uXResolution)    ; //2100; 
 
  
 pageinfo2.paper_length = ceil(((float)( g_pstCurrentPage -> nPageHeightPixels + TOP_MARGIN+BOTTOM_MARGIN)*INCH_TO_0DOT1MM_FACTOR )/g_pstCurrentJob->uXResolution);

  pageinfo2.dummy_page =  0;//g_pstCurrentPage->nBlankPage;

  pageinfo2.orientation =  g_pstCurrentPage-> uOrientation ;
  pageinfo2.print_face = g_pstCurrentPage-> bDuplex;

  pageinfo2.frame_width = g_pstCurrentPage-> nPageWidthPixels;// nLinesInFrame;//6814;
//  pageinfo2.frame_width = ptRasterDescription->imageWidth ; //UPDATING VENKAT

  pageinfo2.frame_length = g_pstCurrentPage-> nPageHeightPixels ;// nFramesToWrite; //4760;
  //pageinfo2.band_length = nOutputLinesPerBand ;//2048;
  pageinfo2.band_length = ptRasterDescription->bandHeight; //UPDATING
  
   /***************pageinfo2 struc initialization*************/
  pageinfo2.flag= 126; 
  int checkflag =   GPS_INFO2_BYPASS_DIR |
                  GPS_INFO2_PWIDTH      |
                  GPS_INFO2_PLENGTH     |           
                  GPS_INFO2_FWIDTH     |           
                  GPS_INFO2_FLENGTH    |           
                  GPS_INFO2_BAND       |           
                  GPS_INFO2_PRINT_MODE |		
                  GPS_INFO2_PAGE_SHIFT |	
                  GPS_INFO2_PRINT_FACE |     	
                  GPS_INFO2_DUMMY_PAGE |     	
                  GPS_INFO2_DITHER_MODE |		
                  GPS_INFO2_COLOR_MATCH	|	
                  GPS_INFO2_ORIENT		|
                  GPS_INFO2_GRAY_PRINT	|
                  GPS_INFO2_RGB_COLOR	|	
                  GPS_INFO2_ECO_COLOR	|	
                  GPS_INFO2_FUSER_CTL       ;//126;
  pageinfo2.bypass_dir=GPS_BYPASS_DIR_SEF;
  pageinfo2.page_shift=GPS_PAGE_SHIFT_NONE;
  pageinfo2.print_mode= GPS_PRINT_MODE_NORMAL;
  pageinfo2.dither_mode[0] = GPS_DITHER_AUTO;
  pageinfo2.dither_mode[1] = GPS_DITHER_AUTO;
  pageinfo2.dither_mode[2] = GPS_DITHER_AUTO;
  pageinfo2.dither_mode[3] = GPS_DITHER_AUTO;
  pageinfo2.color_match[0] = GPS_COLOR_MATCH_OFF;
  pageinfo2.color_match[1] = GPS_COLOR_MATCH_OFF;
  pageinfo2.color_match[2] = GPS_COLOR_MATCH_OFF;
  pageinfo2.color_match[3] = GPS_COLOR_MATCH_OFF;
  pageinfo2.gray_print[0]=0;
  pageinfo2.rgb_color_mode =0;
  pageinfo2.eco_color =0;
  pageinfo2.fuser_ctl =0;



   /******Pagemode2 struc initialization ****/


/******Pagemode2 struc initialization ****/
  pagemode2.flag= GPS_PAGEMODEFLAG_FWIDTH |GPS_PAGEMODEFLAG_FLENGTH|GPS_PAGEMODEFLAG_BAND;
  //pagemode2.frame_width =nOutputBytesPerLine;// nLinesInFrame ; //814;
//  pagemode2.frame_width = ptRasterDescription->imageWidth ; //UPDATING VENKAT
     pagemode2.frame_width = g_pstCurrentPage-> nPageWidthPixels;
     pagemode2.frame_length =   g_pstCurrentPage-> nPageHeightPixels ;
 // pagemode2.frame_length= nLinesInFrame;//nFramesToWrite; //   4760;
  //pagemode2.band_length = nOutputLinesPerBand ; //2048;
  pagemode2.band_length = ptRasterDescription->bandHeight; //UPDATING
  for (i=0;i<32;i++)
      pagemode2.res[i]=0;




     /*************pagemode  struc initialization********/



  pagemode.depth = g_pstCurrentJob->uRIPDepth; //1;
    pagemode.rotate =  g_pstCurrentPage-> uOrientation ;
    
   // pageinfo.paper_width = ((g_pstCurrentPage -> nPageWidthPixels * 25.4)/g_pstCurrentJob->uXResolution)   ; //2970;


 pageinfo.paper_width = ceil(((float)( g_pstCurrentPage -> nPageWidthPixels + LEFT_MARGIN+ RIGHT_MARGIN)*INCH_TO_0DOT1MM_FACTOR )/g_pstCurrentJob->uXResolution);
  
 //pageinfo2.paper_length= ((g_pstCurrentPage -> nPageHeightPixels * 25.4)/g_pstCurrentJob->uXResolution)    ; //2100;
  
 pageinfo.paper_length = ceil(((float)( g_pstCurrentPage -> nPageHeightPixels + TOP_MARGIN+BOTTOM_MARGIN)*INCH_TO_0DOT1MM_FACTOR )/g_pstCurrentJob->uXResolution);



/* pageinfo.paper_width =ceil((( g_pstCurrentPage -> nPageWidthPixels + LEFT_MARGIN+RIGHT_MARGIN)*INCH_TO_0DOT1MM_FACTOR )/g_pstCurrentJob->uXResolution);
 pageinfo.paper_length= ceil(((g_pstCurrentPage -> nPageHeightPixels + LEFT_MARGIN + RIGHT_MARGIN)*INCH_TO_0DOT1MM_FACTOR )/g_pstCurrentJob->uXResolution);*/
 //   pageinfo.paper_length= ((g_pstCurrentPage -> nPageHeightPixels * 25.4)/g_pstCurrentJob->uXResolution)    ; //2100;
   pageinfo.resolution_x =g_pstCurrentJob->uXResolution; //600;
   pageinfo.resolution_y =g_pstCurrentJob->uXResolution; //600;
    
   pageinfo.paper_code=142;
   pageinfo.input_tray= g_pstCurrentJob->tCurrentJobMedia.uInputTray;//15;
  // set_paper_type(); 
    pagemode.color_count = 4;
  //pagemode.frame_width= nOutputBytesPerLine;// nLinesInFrame ; //6814;
//  pagemode.frame_width= ptRasterDescription->imageWidth ; //UPDATING VENKAT
     pagemode.frame_width = g_pstCurrentPage-> nPageWidthPixels; 
      pagemode.frame_length =   g_pstCurrentPage-> nPageHeightPixels ;
//  pagemode.frame_length= nLinesInFrame;//nFramesToWrite; //4760;
  //pagemode.band_length= nOutputLinesPerBand ; //2048;
  pagemode.band_length= ptRasterDescription->bandHeight; //UPDATING
  pagemode.skip_level= 0;
  pagemode.face = 1;
  pagemode.upsidedown=0;
//  pagemode.ra_id_k= 1;
//  pagemode.ra_id_c=1;
//  pagemode.ra_id_m=1;
//  pagemode.ra_id_y=1;

  pagemode.ra_id_k=rID[0];
  pagemode.ra_id_c=rID[1];
  pagemode.ra_id_m=rID[2];
  pagemode.ra_id_y=rID[3];

  pagemode.ra_offx_k=0;
  pagemode.ra_offy_k=0;
  pagemode.ra_offx_c=0;
  pagemode.ra_offy_c=0;
  pagemode.ra_offx_m=0;
  pagemode.ra_offy_m=0;
  pagemode.ra_offx_y=0;
  pagemode.ra_offy_y=0;



  
  /*************pageinfo struc********/
//  pageinfo.frame_width =  nOutputBytesPerLine;//nLinesInFrame;//6814;
//  pageinfo.frame_width =  ptRasterDescription->imageWidth ; //UPDATING VENKAT
     pageinfo.frame_width = g_pstCurrentPage-> nPageWidthPixels;
     pageinfo.frame_length =  g_pstCurrentPage-> nPageHeightPixels ;
//pageinfo.frame_length =  nLinesInFrame;//nFramesToWrite;//4760;
  pageinfo.band_length = nOutputLinesPerBand ;//2048;
  pageinfo.band_length = ptRasterDescription->bandHeight ;//UPDATING  
  pageinfo.line_size=0;
  pageinfo.xoffset =100;
  pageinfo.yoffset=100;
  pageinfo.compress=4;
  pageinfo.color =1;
  pageinfo.fci =0;
  pageinfo.limitless_feed =1;
  pageinfo.paper_count=g_pstCurrentJob->uPagesToPrint   ;//1;
  pageinfo.count_off =0;



  
    set_paper_type(); 
//need to check
   pageinfo.line_size=0;
   pageinfo.xoffset =100;
   pageinfo.yoffset=100;
   pageinfo.compress=4;


    if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_CMYK || OIL_ColorantFamily_RGB)
    {
             pageinfo.color =1;
    }
    else if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_Gray )
    {
           pageinfo.color=0;
    }
    else
    {
            pageinfo.color=0;       //need to update
    }

  pageinfo.fci =0;
  pageinfo.limitless_feed =1;
  pageinfo.paper_count=g_pstCurrentJob->uPagesToPrint   ;//1;
  pageinfo.count_off =0;

   /*************************Call to gpsFrameCreateEx******************************/

 printf("checkflag = %d\n",checkflag);
  GPSFrameCreateExretval = GPS_FrameCreateEx(gps_client, PlotSet_plotid, &pagemode2, &pageinfo2, &framecreate_result);
	if(GPSFrameCreateExretval)
	{
		printf("GPS_FrameCreateExval : Success\n");
	}
	else
	{
		printf("GPS_FrameCreateExval : Failed\n");
	}

 /*****************************Call to gpsFrameCreateInfoRes*****************************/


    GPSFrameCreateInfoResretval = GPS_FrameCreateInfoRes(gps_client, PlotSet_plotid, &pagemode, &pageinfo, GPS_PRINT_NORMAL, &pFCR);
	if(GPSFrameCreateInfoResretval>=0)
	{
		printf("GPS_FrameCreateInfoRes : Success\n");
	}
	else
	{
		printf("GPS_FrameCreateInfoRes : Failed\n");
	}

	ptRasterDescription->pageNumber = pFCR.frame_id;
	gps_frameid = ptRasterDescription->pageNumber; // VENKAT - To make use ptRasterDescription->pageNumber in GPS_FramePrint() in oil_interface_oil2pms.c file

	printf("nLinesInFrame = [%d]\n",nLinesInFrame);
	printf("ptRasterDescription->bandHeight = [%d]\n",ptRasterDescription->bandHeight);
#if 0
	GPSMaxBandNum = (nLinesInFrame+ptRasterDescription->bandHeight-1)/ptRasterDescription->bandHeight;
	printf("GPSMaxBandNum = [%d]\n",GPSMaxBandNum);
    printf("nLinesInFrame = %d\n ptRasterDescription->bandHeight = %d\n",nLinesInFrame, ptRasterDescription->bandHeight);
#endif

          /* Comment out the two following lines to use oil's hard-coded bandheight and rip's linewidth */
          nOutputLinesPerBand = stPMSBandInfo.LinesPerBand;
          nOutputBytesPerLine = ceil(((float)ptRasterDescription->imageWidth) /8); //stPMSBandInfo.BytesPerLine;

          if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND ||
              g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_SINGLE ||
              g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_FRAME)
          {
            /* create the band packet structure */
            ptBandPacket = CreateBandPacketForPage(nColorsInThisBand, nColorFamilyOffset, ptRasterDescription->nSeparations, nOutputBytesPerLine, nOutputLinesPerBand, Map);
            /* hook the band packet to the current page structure */
            g_pstCurrentPage->ptBandPacket=(void*)ptBandPacket;
            /* notify PMS that OIL is ready for rendering */
            g_pstCurrentPage->nBlankPage = FALSE;
            SubmitPageToPMS();
            ptCurrentBandPacket = NULL;
          }
        }
#ifdef DIRECTPRINTPCLOUT
        /* start of direct print */
        else
        {
          nLinesPerFrame = ptRasterDescription->imageHeight;
          nLinesToWrite = nLinesPerFrame;
#ifdef USE_PJL
          if(OIL_PjlShouldOutputPage(g_pstCurrentJob->uPagesParsed + 1, g_pstCurrentJob)==TRUE)
#endif
          {
            if((pmsSysInfo.eOutputType == PMS_DIRECT_PRINT))
              PCL5_RASTER_start((uint8 *)"GGEBDSDK", ptRasterDescription, &rasterFile);
            else
              PCL6_RASTER_start((uint8 *)"GGEBDSDK", ptRasterDescription, &rasterFile);
            if ( rasterFile == NULL )
            {
              /* No open file to write raster to */
              return FALSE;
            }
          }
          /* dataWidth (bits per scanline) is a multiple of 32, so divide by 8 to get bytes */
          bytesPerLine = (ptRasterDescription->dataWidth >> 3);

          /* Determine no of rows per line based on raster depth / no of channels
           * If the rip output is band-interleaved, each line (of pixels) will
           * be represented by channelsPerBand rows of sample values.
           */
          channelsPerBand = 1;
          channelBase = 0 ; /* Base index of this channel */
          nFramesToWrite = 1;

          if (ptRasterDescription->numChannels == 1) {
            /* Gray */
            bytesToWritePerLine = ptRasterDescription->imageWidth ;
          } else {
            /* RGB or CMYK */
            switch ( ptRasterDescription->interleavingStyle ) {
            case interleavingStyle_band:
              /* Band interleaved */
              bytesToWritePerLine = ptRasterDescription->imageWidth ;
              channelsPerBand = ptRasterDescription->numChannels;
              break ;
            case interleavingStyle_frame:
              /* Frame interleaved */
              bytesToWritePerLine = ptRasterDescription->imageWidth ;
              nFramesToWrite = ptRasterDescription->numChannels;
              break ;
            case interleavingStyle_pixel:
              /* Pixel interleaved */
              bytesToWritePerLine = ptRasterDescription->imageWidth * ptRasterDescription->numChannels;
              break ;
            default:
              return FALSE ;
            }
          }
          if ( ptRasterDescription->rasterDepth == 1 ) /* Bilevel */
            bytesToWritePerLine = (bytesToWritePerLine + 7) >> 3 ;
          else if ( ptRasterDescription->rasterDepth == 2 ) /* 2-bit */
            bytesToWritePerLine = (bytesToWritePerLine + 3) >> 2 ;
          else if ( ptRasterDescription->rasterDepth == 4 ) /* 4-bit */
            bytesToWritePerLine = (bytesToWritePerLine + 1) >> 1 ;
          else if ( ptRasterDescription->rasterDepth == 16 )
            bytesToWritePerLine *= 2 ;  /* 16bit, each color takes two bytes */
        } /* end of print direct */
#endif
      }
       /* next state will start filling incoming raster */
      stRasterNextState = OIL_Ras_ProcessBands;
      continue;
    case OIL_Ras_ProcessNewSeparation: /* Determine the separation colorant */
      {
        ptColor = ptRasterDescription->pColorants;
        if(strcmp((char*)ptRasterDescription->colorantFamily, "DeviceCMYK")== 0)
        {
          if(!strcmp((char*)ptColor->colorantName, "Cyan"))
            Map[0] = 0;
          else if(!strcmp((char*)ptColor->colorantName, "Magenta"))
            Map[0] = 1;
          else if(!strcmp((char*)ptColor->colorantName, "Yellow"))
            Map[0] = 2;
          else if(!strcmp((char*)ptColor->colorantName, "Black"))
            Map[0] = 3;
          else if(!strcmp((char*)ptColor->colorantName, "Gray"))
            Map[0] = 3;
          else
          {
            HQFAILV(("Colorant %s not supported", (char*)ptColor->colorantName));
          }
        }
        else if(strcmp((char*)ptRasterDescription->colorantFamily, "DeviceRGB")== 0)
        {
          if(!strcmp((char*)ptColor->colorantName, "Red"))
            Map[0] = 0;
          else if(!strcmp((char*)ptColor->colorantName, "Green"))
            Map[0] = 1;
          else if(!strcmp((char*)ptColor->colorantName, "Blue"))
            Map[0] = 2;
          else
          {
            HQFAILV(("Colorant %s not supported", (char*)ptColor->colorantName));
          }
        }

        if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND ||
            g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_SINGLE ||
            g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_FRAME)
        {
          ptCurrentBandPacket = ptBandPacket;
          ptCurrentBandPacket->uBandNumber = iBandNumber+1;

          /* find and set the colour of the separation */
#ifdef PMS_OIL_MERGE_DISABLE
          ptCurrentBandPacket->atColoredBand[0].ePlaneColorant = (PMS_eColourant)(Map[0] + nColorFamilyOffset);
#else
          ptCurrentBandPacket->atColoredBand[0].ePlaneColorant = (OIL_eColorant)(Map[0] + nColorFamilyOffset);
#endif
          /* in case of separations, the band packet will have data only in first plane
          irrespective of the colour. so set the mapping table to first plane */
          Map[0]=0;
        }
      }

      /* next state will start filling incoming raster */
      stRasterNextState = OIL_Ras_ProcessBands;
      continue;
    case OIL_Ras_ProcessBands:
#ifdef DIRECTPRINTPCLOUT
    if((pmsSysInfo.eOutputType == PMS_DIRECT_PRINT)|| (pmsSysInfo.eOutputType == PMS_DIRECT_PRINTXL))
    {
        /* Calculate no of lines of data expected this time, making allowance for last band */
        nLinesThisTime = (nLinesToWrite < ptRasterDescription->bandHeight)
          ? nLinesToWrite : ptRasterDescription->bandHeight;
        nLinesToWrite -= nLinesThisTime;

        if (ptRasterDescription->imageNegate) {
          uint32 *ptr = (uint32 *)pBuffer ;

          i = nLinesThisTime * channelsPerBand * (ptRasterDescription->dataWidth >> 5) ;
          while ( i-- ) {
            *ptr++ ^= 0xffffffffu ;
          }
        }
        /* Write raster data */
        for ( i = 0 ; i < channelsPerBand ; ++i ) {
          int32 topline = ptRasterDescription->imageHeight - nLinesToWrite - nLinesThisTime ;
          int32 linesThisChannel = nLinesThisTime ;
          int32 linesAtOnce = linesThisChannel ;
          printf("\nchannelsPerBand = %d, linesThisChannel = %d, nLinesToWrite = %d",nchannelsPerBand, linesThisChannel, nLinesToWrite);
          do {
#ifdef USE_PJL
            if(OIL_PjlShouldOutputPage(g_pstCurrentJob->uPagesParsed + 1, g_pstCurrentJob)==TRUE)
#endif
            {
              if((pmsSysInfo.eOutputType == PMS_DIRECT_PRINT))
                nErr = PCL5_RASTER_write_data(rasterFile, pBuffer,
                                          topline, linesAtOnce,
                                          bytesToWritePerLine,
                                          channelBase + i) ;
              else
                nErr = PCL6_RASTER_write_data(rasterFile, pBuffer,
                                          topline, linesAtOnce,
                                          bytesToWritePerLine,
                                          channelBase + i) ;
              if ( nErr != RASTER_noErr )
              {
                if((pmsSysInfo.eOutputType == PMS_DIRECT_PRINT))
                  (void)PCL5_RASTER_finish(&rasterFile, nErr) ;
                else
                  (void)PCL6_RASTER_finish(&rasterFile, nErr) ;
                return FALSE ;
              }
            }
            pBuffer += linesAtOnce * bytesPerLine ;
            topline += linesAtOnce ;
            linesThisChannel -= linesAtOnce ;
          } while ( linesThisChannel > 0 ) ;
        }
        if ( nLinesToWrite == 0 ) /* End of frame */
        {
          if ( --nFramesToWrite == 0 )
          {
            iBandNumber = 0;
 #ifdef USE_PJL
            if(OIL_PjlShouldOutputPage(g_pstCurrentJob->uPagesParsed + 1, g_pstCurrentJob)==TRUE)
#endif
            {
              if((pmsSysInfo.eOutputType == PMS_DIRECT_PRINT))
              {
                if ( PCL5_RASTER_finish(&rasterFile, RASTER_noErr) != RASTER_noErr )
                  return FALSE;
              }
              else
              {
                if ( PCL6_RASTER_finish(&rasterFile, RASTER_noErr) != RASTER_noErr )
                  return FALSE;
              }
            }
            stRasterNextState = OIL_Ras_ProcessNewPage; /*Reset*/
          }
          else
          {
            /* Prepare to do next frame */
            nLinesToWrite = nLinesPerFrame;
            ++channelBase ;
          }
        }
        return TRUE;
    }
    else /* end of direct print - start of pms/oil output */
#endif
    {
        /* since requested band height may be different nLinesInSrcBand will keep track of remaining lines in the band*/
        nLinesInSrcBand = ptRasterDescription->bandHeight;

        /* nColorantOffset is the stride value...to jump to next colorant in band interleaved case
        For last band, the stride is the valid data lines and not the band height */
        if(nRemainingLinesToWrite < ptRasterDescription->bandHeight)
        {
          nColorantOffset = nRemainingLinesToWrite * nBytesPerLineSrc;
        }
        else
        {
          nColorantOffset = ptRasterDescription->bandHeight * nBytesPerLineSrc;
        }

        nRIPDepth = g_pstCurrentPage->uRIPDepth;
        nOutputDepth = g_pstCurrentPage->uOutputDepth;

        /* main loop which writes incoming rasters into oil page according to requested bandheight and bandwidth */
        do
        {
          /* Calculate no of lines of data to be written this time, making allowance for requested bandsize & also the last band */
          nLinesThisTime = (nRemainingLinesToWrite < nLinesInSrcBand)
            ? nRemainingLinesToWrite : nLinesInSrcBand;
          nLinesThisTime = (nLinesThisTime < (nOutputLinesPerBand - nLinesAlreadyWritten))
            ? nLinesThisTime : (nOutputLinesPerBand - nLinesAlreadyWritten);

          if(g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND)
          {
	            GPSMaxBandNum = (nLinesInFrame + ptRasterDescription->bandHeight - 1) / ptRasterDescription->bandHeight;
                if(iBandNumber != 0 && iBandNumber < GPSMaxBandNum) 
                {  
                  
                    int PlaneID = GPS_COLOR_K;
                    for(j=0; j < OIL_MAX_PLANES_COUNT; j++)
                    { 
                        switch(Map[j])
                        {
                            case OIL_Cyan:
                                PlaneID = GPS_COLOR_C;
                                break;
                            case OIL_Magenta:
                                PlaneID = GPS_COLOR_M;
                                break;
                            case OIL_Yellow:
                                PlaneID = GPS_COLOR_Y;
                                break;
                            case OIL_Black:
                                PlaneID = GPS_COLOR_K;
                                break;
                            case OIL_InvalidColor:
                            default:
                            PlaneID = -1;
                            break;
                        }

                        /* initialize the band planes */
                        ptBandPacket->atColoredBand[j].uBandHeight = 0;
                        ptBandPacket->atColoredBand[j].cbBandSize = 0;
                        /* allocate memory for the band data only in valid planes, if
                        we're not in band direct mode */
                        if(g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND &&
                        ptBandPacket->atColoredBand[j].ePlaneColorant != 7
/*PMS_INVALID_COLOURANT*/)
                        {
                            /* GPS to Allocate memory for the Bands */

                            GPSFrameGetBandResretval = GPS_FrameGetBandRes(gps_client,1,iBandNumber, PlaneID, &pFGBR); 
                            if(!GPSFrameGetBandResretval)
		                    {
			                    printf("GPS_FrameGetBandRes : Success\n");
                                ptBandPacket->atColoredBand[j].pBandRaster = pFGBR.band_addr;
                                printf("------pfgbr size =%d\n",sizeof(*pFGBR.band_addr));
                                    pDestPtr =  ptBandPacket->atColoredBand[j].pBandRaster;
		                    }
		                    else
		                    {
                                printf("GPS_FrameGetBandRes : Failed\n");
                                ptBandPacket->atColoredBand[j].pBandRaster = NULL;
		                    }

                            if(!ptBandPacket->atColoredBand[j].pBandRaster)
                            {
                                HQASSERTV(g_pstCurrentptBandPacket->atColoredBand[j].pBandRaster!=NULL,
                                ("Failed to allocate %d bytes", (nOutputLinesPerBand * nOutputBytesPerLine)));
                                return NULL;
                            }
                        }
                        else
                        {
                            ptBandPacket->atColoredBand[j].pBandRaster = NULL;
                        }
                   }
              }
          }


          if(g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_SINGLE ||
              g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_FRAME)
          {
            if(iBandNumber >= PMS_BAND_LIMIT)
            {
              HQASSERTV((iBandNumber < PMS_BAND_LIMIT), ("OIL_RasterCallback: Exceeded the limit of maximum PMS bands (%d)", PMS_BAND_LIMIT));
#ifdef PMS_OIL_MERGE_DISABLE
              OIL_PageDone(g_pstCurrentPage->ptPMSpage);
#else
              OIL_PageDone(g_pstCurrentPage);
#endif
              OIL_JobCancel();
              OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)0);
              return FALSE ;
            }
          }
          else if(iBandNumber >= OIL_BAND_LIMIT)
          {
            HQASSERTV((iBandNumber < OIL_BAND_LIMIT), ("OIL_RasterCallback: Exceeded the limit of maximum OIL bands (%d) \n", OIL_BAND_LIMIT));
#ifdef PMS_OIL_MERGE_DISABLE
            if(g_pstCurrentPage->ptPMSpage != NULL)
              OIL_PageDone(g_pstCurrentPage->ptPMSpage);
#else
            if(g_pstCurrentPage != NULL)
              OIL_PageDone(g_pstCurrentPage);
#endif
            else
                DeleteOILPage(g_pstCurrentPage);
            OIL_JobCancel();
            OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)0);
            return FALSE ;
          }
          /* in the push band (on the fly) delivery model, procure an empty buffer */
          if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND ||
              g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_SINGLE ||
              g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_FRAME)
          {
            if(ptCurrentBandPacket == NULL) /* neither partial band nor new separation*/
            {
              ptCurrentBandPacket = ptBandPacket;
              ptCurrentBandPacket->uBandNumber = iBandNumber+1;
            }
          }

          /* loop to copy rasters into correct planes and hook them into g_pstCurrentPage */
          int k;
          for ( k = 0 ; k < nColorsInThisBand ; k++ )
          {

            printf("\nColorsInThisBand = %d\n",nColorsInThisBand);
            	switch(Map[k])
                        {
                            case OIL_Cyan:
                                i = GPS_COLOR_C;
                                break;
                            case OIL_Magenta:
                                i = GPS_COLOR_M;
                                break;
                            case OIL_Yellow:
                                i = GPS_COLOR_Y;
                                break;
                            case OIL_Black:
                                i = GPS_COLOR_K;
                                break;
                            case OIL_InvalidColor:
                            default:
                            	i = -1;
                            break;
                        }

            if(Map[i] == -1)
              continue;
            if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_SINGLE ||
                g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_FRAME)
            {
              /* In push band direct mode, we don't need to do copying,
                 padding or bit depth conversion. (TODO assertions to
                 that effect). Just update band size and band height in
                 the band structure and point to the band memory in the
                 band packet. */
              int nColorantStride = sRasterLayout.colorant_stride;

              if (nColorantStride == 0) {
                /* A colorant stride of zero means use band height * line stride. */
                nColorantStride = nColorantOffset;
              }

              ptCurrentBandPacket->atColoredBand[Map[i]].pBandRaster = (unsigned char *)(pBuffer + nColorantStride * i);
              ptCurrentBandPacket->atColoredBand[Map[i]].cbBandSize = nLinesThisTime * nOutputBytesPerLine;
              ptCurrentBandPacket->atColoredBand[Map[i]].uBandHeight = nLinesThisTime;
              /* TODO: For the moment, we just go around the loop. We
                 will have to do some of the work in this loop if we
                 want to support partial bands OIL_PUSH_BAND_DIRECT
                 mode. */
              continue;
            }

            if(bPartialBand == TRUE)
            {
              /* this band is partially filled with data. determine the end address of the filled data */
              if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND)
              {
                /* taking into account lines already written, set pDestPtr to point to the memory in the band buffer */
                pDestPtr = ptCurrentBandPacket->atColoredBand[Map[i]].pBandRaster + (nLinesAlreadyWritten * nOutputBytesPerLine);
              }
              else
              {
                pDestPtr = g_pstCurrentPage->atPlane[Map[i]].atBand[iBandNumber].pBandRaster + (nLinesAlreadyWritten * nOutputBytesPerLine);
              }
            }
            else /*new band */
            {
                if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND)
                {
                    /* set pDestPtr to point to the allocated memory in the band buffer */
 	              pDestPtr = ptBandPacket->atColoredBand[i].pBandRaster;
                }
                else
                {
                /* allocate and initialize memory for the band */
#ifdef PMS_OIL_MERGE_DISABLE_MEM
                pBandBuffer = (unsigned char *)OIL_malloc(OILMemoryPoolJob, OIL_MemBlock, (nOutputLinesPerBand * nOutputBytesPerLine));
#else
		        pBandBuffer = (unsigned char *)malloc((nOutputLinesPerBand * nOutputBytesPerLine));
#endif
                if(!pBandBuffer)
                {
                  HQASSERTV(pBandBuffer!=NULL, ("OIL_RasterCallback: Failed to allocate %d bytes", (nOutputLinesPerBand * nOutputBytesPerLine)));
                  DeleteOILPage(g_pstCurrentPage);
                  OIL_JobCancel();
                  OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)0);
                  return FALSE ;
                }
                pDestPtr = pBandBuffer;
              }
            }

            /* determine the address in incoming raster buffer from which we have to start copying.
            it is based on number of lines already copied from this colorant from incoming raster buffer */
            pSrcPtr = (unsigned char *)((pBuffer + nColorantOffset * i) + (ptRasterDescription->bandHeight - nLinesInSrcBand) * nBytesPerLineSrc);

            if(nRIPDepth == nOutputDepth)
            {
              if(nOutputBytesPerLine == nBytesPerLineSrc)
              { /* the entire band can be written in 1 go */

				for(line = 0; line < nLinesThisTime; line++)
                {	
                  memcpy(pDestPtr, pSrcPtr, nOutputBytesPerLine);
                  pDestPtr += nOutputBytesPerLine;
                  pSrcPtr += nBytesPerLineSrc;
                }         
                  //memcpy(pDestPtr, pSrcPtr, nLinesThisTime * nOutputBytesPerLine);
                  //printf("-----------pdestptr address= %u-------\n",pDestPtr);
 	              //printf("-----------psrcptr address= %u-------\n",pSrcPtr);
                  printf("iBandnumber = %d and for colorant = %d \n",iBandNumber, i);
		          printf("-----------psrcptr = %s--------------\n",pSrcPtr);
                  printf("pDestPtr = %s \n",pDestPtr);
                  printf("\npSrcPtr = %s \n",pSrcPtr);
                  printf("\nnLinesThisTime = %d\n nOutputBytesPerLine =%d\n",nLinesThisTime,nOutputBytesPerLine);
                  
		          //printf("---size of srcptr =  %d----\n",sizeof(*pSrcPtr));
		          //printf("---size of Destptr =  %d----\n",sizeof(*pTemp));
                  pSrcPtr += (nLinesThisTime * nBytesPerLineSrc); 

              }
              else if(nOutputBytesPerLine < nBytesPerLineSrc)
              { /* write only requested no of bytes per line */
                for(line = 0; line < nLinesThisTime; line++)
                {
                  memcpy(pDestPtr, pSrcPtr, nOutputBytesPerLine);
                  pDestPtr += nOutputBytesPerLine;
                  pSrcPtr += nBytesPerLineSrc;
                }
              }
              else /* (nOutputBytesPerLine > nBytesPerLine) */
              { /* copy nBytesPerLine into nOutputBytesPerLine and zero the remainder */
                for(line = 0; line < nLinesThisTime; line++)
                {
                  memcpy(pDestPtr, pSrcPtr, nBytesPerLineSrc);
                  /* zero the remainder of the line */
                  memset(pDestPtr + (nBytesPerLineSrc*8), 0x00, nOutputBytesPerLine - (nBytesPerLineSrc*8));
                  pDestPtr += nOutputBytesPerLine;
                  pSrcPtr += nBytesPerLineSrc;
                }
              }
            }
            else
            {
                printf("\nIn RIP Conversion \n");
              /* Convert RIP 1bpp to 8bpp, 0x00 or 0xff */
              if((nRIPDepth==1) && (nOutputDepth==8))
              {
                /* Is it like a memcpy, but with bloating 1bpp to 8bpp? */
                if(nOutputBytesPerLine == (nBytesPerLineSrc*8))
                {
  #ifdef USE_64
                    pDestPtr64 = (ras64 *)pDestPtr;
                    p = pSrcPtr;
                    for(x = 0; x < ((nLinesThisTime * nOutputBytesPerLine)&~31); x+=32)
                    {
  #ifdef LOWBYTEFIRST
                      pDestPtr64[0] = l_aConv8to64[p[0]];
                      pDestPtr64[1] = l_aConv8to64[p[1]];
                      pDestPtr64[2] = l_aConv8to64[p[2]];
                      pDestPtr64[3] = l_aConv8to64[p[3]];
  #else
                      pDestPtr64[0] = l_aConv8to64[p[3]];
                      pDestPtr64[1] = l_aConv8to64[p[2]];
                      pDestPtr64[2] = l_aConv8to64[p[1]];
                      pDestPtr64[3] = l_aConv8to64[p[0]];
  #endif
                      pDestPtr64+=4;
                      p+=4;
                    }
  #else /* else 32 */
                    pDestPtr32 = (ras32 *)pDestPtr;
                    p = pSrcPtr;
                    for(x = 0; x < ((nLinesThisTime * nOutputBytesPerLine)&~31); x+=32)
                    {
  #ifdef LOWBYTEFIRST
                      pDestPtr32[0] = l_aConvHigh4to32[p[3]];
                      pDestPtr32[1] = l_aConvLow4to32[p[3]];
                      pDestPtr32[2] = l_aConvHigh4to32[p[2]];
                      pDestPtr32[3] = l_aConvLow4to32[p[2]];
                      pDestPtr32[4] = l_aConvHigh4to32[p[1]];
                      pDestPtr32[5] = l_aConvLow4to32[p[1]];
                      pDestPtr32[6] = l_aConvHigh4to32[p[0]];
                      pDestPtr32[7] = l_aConvLow4to32[p[0]];
  #else
                      pDestPtr32[0] = l_aConvHigh4to32[p[0]];
                      pDestPtr32[1] = l_aConvLow4to32[p[0]];
                      pDestPtr32[2] = l_aConvHigh4to32[p[1]];
                      pDestPtr32[3] = l_aConvLow4to32[p[1]];
                      pDestPtr32[4] = l_aConvHigh4to32[p[2]];
                      pDestPtr32[5] = l_aConvLow4to32[p[2]];
                      pDestPtr32[6] = l_aConvHigh4to32[p[3]];
                      pDestPtr32[7] = l_aConvLow4to32[p[3]];
  #endif
                      pDestPtr32+=8;
                      p+=4;
                    }
  #endif /* 64/32 */
                    pSrcPtr += nLinesThisTime * nBytesPerLineSrc;
                }
                /* Clip the source line */
                else if(nOutputBytesPerLine < (nBytesPerLineSrc*8))
                { /* write only requested no of bytes per line */
                  int nQuick = nOutputBytesPerLine&~31;
                  int nRemainder = nOutputBytesPerLine&31;
  /*                unsigned char uPartialByte;
                  ras64 uPartialPixels;
                  unsigned char *pPartialDst; */
                  for(line = 0; line < nLinesThisTime; line++)
                  {
  #ifdef USE_64
                    pDestPtr64 = (ras64 *)pDestPtr;
                    p = pSrcPtr;
                    for(x = 0; x < nQuick; x+=32)
                    {
  #ifdef LOWBYTEFIRST
                      *pDestPtr64++ = l_aConv8to64[p[3]];
                      *pDestPtr64++ = l_aConv8to64[p[2]];
                      *pDestPtr64++ = l_aConv8to64[p[1]];
                      *pDestPtr64++ = l_aConv8to64[p[0]];
  #else
                      *pDestPtr64++ = l_aConv8to64[p[0]];
                      *pDestPtr64++ = l_aConv8to64[p[1]];
                      *pDestPtr64++ = l_aConv8to64[p[2]];
                      *pDestPtr64++ = l_aConv8to64[p[3]];
  #endif
                      p+=4;
                    }
                    pDestPtr32 = (ras32 *)pDestPtr64;
  #else /* 32 */
                    pDestPtr32 = (ras32 *)pDestPtr;
                    p = pSrcPtr;
                    for(x = 0; x < nQuick; x+=32)
                    {
  #ifdef LOWBYTEFIRST
                      pDestPtr32[0] = l_aConvHigh4to32[p[3]];
                      pDestPtr32[1] = l_aConvLow4to32[p[3]];
                      pDestPtr32[2] = l_aConvHigh4to32[p[2]];
                      pDestPtr32[3] = l_aConvLow4to32[p[2]];
                      pDestPtr32[4] = l_aConvHigh4to32[p[1]];
                      pDestPtr32[5] = l_aConvLow4to32[p[1]];
                      pDestPtr32[6] = l_aConvHigh4to32[p[0]];
                      pDestPtr32[7] = l_aConvLow4to32[p[0]];
  #else
                      pDestPtr32[0] = l_aConvHigh4to32[p[0]];
                      pDestPtr32[1] = l_aConvLow4to32[p[0]];
                      pDestPtr32[2] = l_aConvHigh4to32[p[1]];
                      pDestPtr32[3] = l_aConvLow4to32[p[1]];
                      pDestPtr32[4] = l_aConvHigh4to32[p[2]];
                      pDestPtr32[5] = l_aConvLow4to32[p[2]];
                      pDestPtr32[6] = l_aConvHigh4to32[p[3]];
                      pDestPtr32[7] = l_aConvLow4to32[p[3]];
  #endif
                      pDestPtr32+=8;
                      p+=4;
                    }
  #endif /* 64/32 */

  #ifdef LOWBYTEFIRST
                    a = 3;
  #else
                    a = 0;
  #endif
                    for(x = nRemainder; x > 7; x-=8)
                    {
                      *pDestPtr32++ = l_aConvHigh4to32[p[a]];
                      *pDestPtr32++ = l_aConvLow4to32[p[a]];
  #ifdef LOWBYTEFIRST
                      a--;
                      if(a<0) {
                        a = 3;
                        p+=4;
                      }
  #else
                      a++;
                      if(a>3) {
                        a = 0;
                        p+=4;
                      }
  #endif
                    }
                    if(x > 3) {
                      *pDestPtr32++ = l_aConvHigh4to32[p[a]];
                      x -= 4;
                    }
                    /* The following code to handle less than four
                       remaining bytes doesn't run when output is
                       a multiple of 32 bits as x will always be zero
                       by the time this point is reached. */
                    pDst8 = (unsigned char *)pDestPtr32;
                    switch(x)
                    {
                    default:
                    case 0:
                      break;
                    case 1:
                      pDst8[0] = (*p&0x08)?0xff:0x00;
                    case 2:
                      pDst8[1] = (*p&0x04)?0xff:0x00;
                    case 3:
                      pDst8[2] = (*p&0x02)?0xff:0x00;
                      break;
                    }

                    pDestPtr += nOutputBytesPerLine;
                    pSrcPtr += nBytesPerLineSrc;
                  }
                }
                /* pad the source line */
                else /* (nOutputBytesPerLine > nBytesPerLine) */
                { /* copy nBytesPerLine into nOutputBytesPerLine and zero the remainder */
                  for(line = 0; line < nLinesThisTime; line++)
                  {
  #ifdef USE_64
                    pDestPtr64 = (ras64 *)pDestPtr;
                    p = pSrcPtr;
                    for(x = 0; x < nBytesPerLineSrc; x+=32)
                    {
                      *pDestPtr64++ = l_aConv8to64[p[3]];
                      *pDestPtr64++ = l_aConv8to64[p[2]];
                      *pDestPtr64++ = l_aConv8to64[p[1]];
                      *pDestPtr64++ = l_aConv8to64[p[0]];
                      p+=4;
                    }
  #else /* 32 */
                    pDestPtr32 = (ras32 *)pDestPtr;
                    p = pSrcPtr;
                    for(x = 0; x < nBytesPerLineSrc; x+=32)
                    {
  #ifdef LOWBYTEFIRST
                      pDestPtr32[0] = l_aConvHigh4to32[p[3]];
                      pDestPtr32[1] = l_aConvLow4to32[p[3]];
                      pDestPtr32[2] = l_aConvHigh4to32[p[2]];
                      pDestPtr32[3] = l_aConvLow4to32[p[2]];
                      pDestPtr32[4] = l_aConvHigh4to32[p[1]];
                      pDestPtr32[5] = l_aConvLow4to32[p[1]];
                      pDestPtr32[6] = l_aConvHigh4to32[p[0]];
                      pDestPtr32[7] = l_aConvLow4to32[p[0]];
  #else
                      pDestPtr32[0] = l_aConvHigh4to32[p[0]];
                      pDestPtr32[1] = l_aConvLow4to32[p[0]];
                      pDestPtr32[2] = l_aConvHigh4to32[p[1]];
                      pDestPtr32[3] = l_aConvLow4to32[p[1]];
                      pDestPtr32[4] = l_aConvHigh4to32[p[2]];
                      pDestPtr32[5] = l_aConvLow4to32[p[2]];
                      pDestPtr32[6] = l_aConvHigh4to32[p[3]];
                      pDestPtr32[7] = l_aConvLow4to32[p[3]];
  #endif
                      pDestPtr32+=8;
                      p+=4;
                    }
  #endif /* 64/32 */
                    /* zero the remainder of the line */
                    memset(pDestPtr + (nBytesPerLineSrc*8), 0x00, nOutputBytesPerLine - (nBytesPerLineSrc*8));
                    pDestPtr += nOutputBytesPerLine;
                    pSrcPtr += nBytesPerLineSrc;
                  }
                }
              }
              else
              {
                HQFAILV(("Bit depth conversion, %d to %d, is not supported.", nRIPDepth, nOutputDepth));
              }
            }

            /* if this was a partial band then just update bandsize and height. it is already hooked to g_pstCurrentPage */
            if(bPartialBand == TRUE)
            {
              /* update band size and band height in the band structure */
              if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND ||
                  g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_SINGLE ||
                  g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_FRAME)
              {
                ptCurrentBandPacket->atColoredBand[Map[i]].cbBandSize = (nLinesThisTime + nLinesAlreadyWritten) * nOutputBytesPerLine;
                ptCurrentBandPacket->atColoredBand[Map[i]].uBandHeight = nLinesThisTime + nLinesAlreadyWritten;
              }
              else
              {
                g_pstCurrentPage->atPlane[Map[i]].atBand[iBandNumber].cbBandSize = (nLinesThisTime + nLinesAlreadyWritten) * nOutputBytesPerLine;
                g_pstCurrentPage->atPlane[Map[i]].atBand[iBandNumber].uBandHeight = nLinesThisTime + nLinesAlreadyWritten;
              }
            }
            else
            {
              /* update band size and band height in the band structure */
              if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND ||
                  g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_SINGLE ||
                  g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_FRAME)
              {
                ptCurrentBandPacket->atColoredBand[Map[i]].cbBandSize = nLinesThisTime * nOutputBytesPerLine;
                ptCurrentBandPacket->atColoredBand[Map[i]].uBandHeight = nLinesThisTime;
              }
              else
              {/* if it is a new band, we need to hook it to g_pstCurrentPage */
                g_pstCurrentPage->atPlane[Map[i]].atBand[iBandNumber].pBandRaster = pBandBuffer;
                g_pstCurrentPage->atPlane[Map[i]].atBand[iBandNumber].cbBandSize = nLinesThisTime * nOutputBytesPerLine;
                g_pstCurrentPage->atPlane[Map[i]].atBand[iBandNumber].uBandHeight = nLinesThisTime;
                g_pstCurrentPage->atPlane[Map[i]].atBand[iBandNumber].uBandNumber = iBandNumber+1;
                g_pstCurrentPage->atPlane[Map[i]].uBandTotal = iBandNumber+1;
              }
            }
          } /* for loop end */


          /* decrement the number of lines we just wrote */
          nRemainingLinesToWrite -= nLinesThisTime;
          nLinesInSrcBand -= nLinesThisTime;

          /* if the band is not fully filled and if still there is data to come, mark this band as Partial */
          if(((nLinesThisTime + nLinesAlreadyWritten) < nOutputLinesPerBand) && (nRemainingLinesToWrite > 0))
          {
            bPartialBand = TRUE;
            nLinesAlreadyWritten = nLinesThisTime + nLinesAlreadyWritten;
            break;
          }
          else
          { /* band is fully filled or no more data to come */
            bPartialBand = FALSE;
            iBandNumber++;
            nLinesAlreadyWritten = 0;
            if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND ||
                g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_SINGLE ||
                g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_FRAME)
            {
              /* submit the band to pms here */
              if(!SubmitBandToPMS(ptCurrentBandPacket))
              {
#ifdef PMS_OIL_MERGE_DISABLE
                  OIL_PageDone(g_pstCurrentPage->ptPMSpage);
#else
                  OIL_PageDone(g_pstCurrentPage);
#endif
                  return FALSE;
              }
              ptCurrentBandPacket = NULL;
            }
          }
          /* exit if all the valid data from incoming band is copied */
        }while((nLinesInSrcBand != 0)&&(nRemainingLinesToWrite != 0));

        if (nRemainingLinesToWrite == 0) /* all data for this page/sep copied */
        {
          /* this was the last band. proceed to next state*/
          stRasterNextState = OIL_Ras_ProcessEndOfBands;
          continue;
        }
        else
        {
          /* request next band from rip */
          OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)1);
          return TRUE;
        }
      } /* end of pms/oil output */
    case OIL_Ras_ProcessEndOfBands:
      if ( nRemainingLinesToWrite == 0 )
      {
        ptColor = ptRasterDescription->pColorants;
        for(i=0; ptColor != NULL ; i++)
        {
          g_pstCurrentPage->atPlane[Map[ptColor->nChannel]].bBlankPlane = FALSE;
          ptColor = ptColor->pNext;
        }

        if ( --nFramesToWrite == 0 ) /* end of page */
        {
          g_pstCurrentPage->nBlankPage = (g_pstCurrentPage->nColorants)?FALSE:TRUE;

          g_pstCurrentPage->bPageComplete = TRUE;
          stRasterNextState = OIL_Ras_ProcessNewPage; /*Reset*/

          g_pstCurrentPage->nRasterWidthData = nOutputBytesPerLine << 3;
          g_pstCurrentPage->pstJob = g_pstCurrentJob;

          OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)1);
          return TRUE;
        }
        else /* separation/frame complete */
        {
          /* to start new separation with first band */
          iBandNumber = 0;
          nRemainingLinesToWrite = nLinesInFrame;
          if(ptRasterDescription->nSeparations > 1)
          {
            /* set next state to determine colorant of next separation */
            stRasterNextState = OIL_Ras_ProcessNewSeparation;
          }
          else
          {
            stRasterNextState = OIL_Ras_ProcessBands;
            Map[0] = Map[ptRasterDescription->numChannels - nFramesToWrite];
          }
          OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)1);
          return TRUE;
        }
      }
      break;
    default:
      OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)1);
      return TRUE;
    } /* switch end */
  } /* while (1) */

  OIL_ProbeLog(SW_TRACE_OIL_RASTERCALLBACK, SW_TRACETYPE_EXIT, (intptr_t)1);

  return TRUE;
}

/* this function provides the Red Book feature for a blank page at the end of a duplex job */
/* with an odd number of pages as the core does not currently provide this */
/* note that not all printers require this feature */
void DummyPageforDuplex(void *pJobContext)
{
  /*check for duplex */
  if(l_PageDuplex)
  {
    /* check page count */
    if(g_pstCurrentJob->uPagesToPrint & 0x1)
    {
      struct rasterDescription RasterDescription;
      /* check for delivery type */
      if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND ||
          g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_SINGLE ||
          g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND_DIRECT_FRAME)
      {
        /* create blank page */
        g_pstCurrentPage = CreateOILBlankPage(g_pstCurrentPage, &RasterDescription);
        if(!g_pstCurrentPage)
        return;
        BlankBandsToPMS(pJobContext, &RasterDescription);
        SubmitBandToPMS(NULL);
      }
      else
      {
        SubmitPageToPMS();
      }
      g_pstCurrentPage = NULL;
    }
  }
}


/* This function creates the bands that are sent to the system for  a blank page */
/* CMYK and gray create only a single plane; RGB has to create all three planes */
/* as the data has to be set to ones as opposed to zeroes for CMYK */
void BlankBandsToPMS(void *pJobContext, struct rasterDescription *ptRasterDescription)
{
RASTER_REQUIREMENTS  RasterRequirements;

RASTER_DESTINATION  RasterDestination;
PMS_TyBandPacket *ptBandPacket;
int i, j, k, nBands, nBandSize, nBandHeight;
int nColorsInThisBand;
int nColorFamilyOffset;
RasterColorant *ptColor;
short Map[OIL_MAX_PLANES_COUNT];
PMS_TyBandInfo stPMSBandInfo;

 /* set up raster data for callback functions to be called */
  RasterRequirements.page_width = ptRasterDescription->imageWidth;
  RasterRequirements.page_height = ptRasterDescription->imageHeight;
  RasterRequirements.bit_depth = ptRasterDescription->rasterDepth;
  RasterRequirements.components = ptRasterDescription->numChannels;
  RasterRequirements.components_per_band = ptRasterDescription->numChannels;
  RasterRequirements.line_bytes = ptRasterDescription->dataWidth;
  RasterRequirements.have_framebuffer = 0;
  nColorsInThisBand = ptRasterDescription->numChannels;
  ptColor = ptRasterDescription->pColorants;
    /* determine bandheight and bandwidth requested by pms */
  GetBandInfoFromPMS(g_pstCurrentPage->nPageWidthPixels, g_pstCurrentPage->nPageHeightPixels,
          RasterRequirements.bit_depth, &stPMSBandInfo);
  RasterRequirements.band_height = stPMSBandInfo.LinesPerBand;
   /* call raster definition functions as not called for blank pages */
  for(i=0; i< OIL_MAX_PLANES_COUNT; i++)
    Map[i] = OIL_InvalidColor;
  if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_CMYK){
    Map[0] = 3; /* just set up for the black/gray channel */
    nColorFamilyOffset = CMYK_OFFSET;
    g_pstCurrentPage->nColorants = 1;
  }
  else if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_RGB){
    Map[0] = 0;
    Map[1] = 1;
    Map[2] = 2;
    nColorFamilyOffset = RGB_OFFSET;
    g_pstCurrentPage->nColorants = 3;
  }
  else if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_Gray){
    Map[0] = 3;
    nColorFamilyOffset = GRAY_OFFSET;
    g_pstCurrentPage->nColorants = 1;
  }
  else
    /**********************  TODO something ***********************************/
    return;
  nBands = (g_pstCurrentPage->nPageHeightPixels + stPMSBandInfo.LinesPerBand - 1)/stPMSBandInfo.LinesPerBand;
  ptBandPacket = CreateBandPacket(nColorsInThisBand, nColorFamilyOffset, ptRasterDescription->nSeparations,
                                                        stPMSBandInfo.BytesPerLine, stPMSBandInfo.LinesPerBand, Map);
  g_pstCurrentPage->ptBandPacket=(void*)ptBandPacket;
  /* notify PMS that OIL is ready for rendering */
  g_pstCurrentPage->nBlankPage = FALSE;  /* need to do this to avoid creating another oil blank page */
  SubmitPageToPMS();
  /* Now simulate the raster callback sequence for the relevant configurationa and mode */
  /* send blank band data to buffer for all bands*/
  for(j= 0; j < ptRasterDescription->nSeparations; j++)
  {
    OIL_RasterRequirements(pJobContext, &RasterRequirements, 1);
    for(i= 0; i < nBands; i++)
    {
      /* get the buffer pointer */
      RasterDestination.bandnum = i;
      RasterDestination.handled = 0;
      OIL_RasterDestination(pJobContext, &RasterDestination, j);
      /* calculate band size */
      if((stPMSBandInfo.LinesPerBand * (i + 1)) > (int)RasterRequirements.page_height)
        nBandHeight =RasterRequirements.page_height - (stPMSBandInfo.LinesPerBand * i);
      else
        nBandHeight = stPMSBandInfo.LinesPerBand;
      nBandSize = stPMSBandInfo.BytesPerLine * nBandHeight;
        /* set buffer data to zero */
      for(k =0; k < OIL_MAX_PLANES_COUNT; k++)
      {
        if(Map[k] != OIL_InvalidColor)
        {
          if (g_ConfigurableFeatures.eBandDeliveryType == OIL_PUSH_BAND)
          {
            if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_RGB)
              memset(ptBandPacket->atColoredBand[Map[k]].pBandRaster, 255, nBandSize);
            else
              memset(ptBandPacket->atColoredBand[Map[k]].pBandRaster, 0, nBandSize);
          }
          else
          {
            if(g_pstCurrentPage->eColorantFamily == OIL_ColorantFamily_RGB)
            {
              /* This works as we have 3 colorants for RGB (k = 0,1,2) */
              ptBandPacket->atColoredBand[Map[k]].pBandRaster = RasterDestination.memory_base + (nBandSize * k);
              memset(ptBandPacket->atColoredBand[Map[k]].pBandRaster, 255, nBandSize);
            }
            else
            {
              /* This works as we only have 1 colorant for CMYK and Gray */
              memset(RasterDestination.memory_base, 0, nBandSize);
              ptBandPacket->atColoredBand[Map[k]].pBandRaster = RasterDestination.memory_base;
            }
          }
          ptBandPacket->atColoredBand[Map[k]].cbBandSize = nBandSize;
          ptBandPacket->atColoredBand[Map[k]].uBandHeight = nBandHeight;
        }
      }
      /* check in each band */
      ptBandPacket->uBandNumber = i + 1;
      SubmitBandToPMS(ptBandPacket);
    }
  }
}
