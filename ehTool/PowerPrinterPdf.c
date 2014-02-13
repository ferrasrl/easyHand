#ifdef EH_PDF
	#pragma message("--> Includo /easyhand/ehtoolx/libharu-2.1.0/libhpdf.lib <-------------------")
	#pragma comment(lib, "/easyhand/ehtoolx/libharu-2.1.0/libhpdf.lib")
#endif


#include "/easyhand/inc/easyhand.h"
#include "/easyhand/Ehtoolx/libharu-2.1.0/include/hpdf.h" // aggiunto per i pdf
#include <setjmp.h>



//******************
// inseriti per PDF
//
static void PDFCreator(SINT nPage,RECT *lpRect);
void pdf_error_handler(HPDF_STATUS   error_no,
                       HPDF_STATUS   detail_no,
                       void          *user_data);
/*typedef void (__stdcall *HPDF_Error_Handler) (HPDF_STATUS  error_no,
                                              HPDF_STATUS  detail_no,
                                              void        *user_data);
*/
SINT _LPdfTextAlign(SINT iAllinea);

jmp_buf env;
//******************




//static void LRFillPage(BOOL fPrinter,HDC hDC,SINT nPage,RECT *lpRect)
static void PDFCreator(SINT nPage,RECT *lpRect)
{
	SINT a,c,d,z;
	BYTE *ptr;
	EH_AR arFonts,arNameFonts;

	CHAR *pFileName="c:\\prova.pdf";

	CHAR szfont[200];
	CHAR *pszfont_name=NULL;
	SINT idxFont=0;

	HPDF_Doc  pdf;
	HPDF_Page *psPage; // Array
	HPDF_Font font;
	HPDF_Image image;

	//LREFileLoad();

	//psPage=ehAlloc(sizeof(HPDF_Page)*lpLRInit->iPageCount);
	psPage=ehAlloc(sizeof(HPDF_Page));

	pdf = HPDF_New (pdf_error_handler, NULL);
	//pdf = HPDF_New (error_handler, NULL);
	if (!pdf) {
	  printf ("error: cannot create PdfDoc object\n");
	  //return ;
	}

	if (setjmp(env)) {
	  HPDF_Free (pdf);
	  //return ;
	}

	//font = HPDF_GetFont (pdf, "Helvetica", NULL);

	arFonts= ARNew();
	arNameFonts= ARNew();

	d=0;
	z=0;

	for (c=0;c<lpLRInit->iPageCount;c++)
	{
		psPage[c] = HPDF_AddPage (pdf);//,HPDF_PAGE_SIZE_A4,HPDF_PAGE_PORTRAIT);


		HPDF_Page_SetHeight (psPage[c], (HPDF_REAL) lpLRInit->sPage.cy);
		HPDF_Page_SetWidth (psPage[c], (HPDF_REAL) lpLRInit->sPage.cx+50);


		for (a=arPageIndex[c];a<iLRE_Elements;a++)
		//for (a=iLRE_Elements-1;a>arPageIndex[c];a--)
		{

			if (arElement[a]->iPage==(c+1)) {
				
				ptr=(BYTE *) arElement[a]; // Puntatore al Tag
				ptr+=sizeof(LRETAG);
				switch (arElement[a]->iType)
				{

					case LRE_CHAR: 
						{
							LRT_CHAREX *Lex;
							CHAR *pszString=NULL;			
							SINT  yChar,xChar;
							SINT x,y;


							pszString=ptr+sizeof(LRT_CHAR);
							Lex = (LRT_CHAREX *) ptr;

							xChar=(HPDF_UINT) Lex->xChar;
							yChar=(HPDF_UINT) Lex->yChar;
							x=(HPDF_UINT) Lex->Point.x;
							y=(HPDF_UINT) ((lpLRInit->sPage.cy) - (Lex->Point.y) );

							// carico il font (se già caricato una volta non lo carico +)
							idxFont = ARSearchIdx(arFonts,Lex->szFontName,FALSE);
							if (idxFont<0) {
								ARAdd(&arFonts,Lex->szFontName);
								sprintf(szfont,"c:\\fonts\\%s.ttf",Lex->szFontName);
								pszfont_name = HPDF_LoadTTFontFromFile (pdf, szfont, HPDF_FALSE);
								ARAdd(&arNameFonts,pszfont_name);	
							}
							else pszfont_name=arNameFonts[idxFont];
							font = HPDF_GetFont (pdf, pszfont_name, NULL);

							HPDF_Page_SetFontAndSize (psPage[c], font, Lex->yChar-10);

							if (y>1) {

								HPDF_Page_BeginText (psPage[c]);
								HPDF_Page_TextOut (psPage[c],(HPDF_REAL) x, (HPDF_REAL) y, pszString);
								HPDF_Page_EndText (psPage[c]);

							}
						}

					break;

					case LRE_CHAREX: 
						{

							LRT_CHAREX *Lex;
							CHAR *pszString=NULL;
							SINT  yChar,xChar;
							SINT x,y;

							LONG lColor;
							HPDF_REAL xx;
							float r,g,b;


							pszString=ptr+sizeof(LRT_CHAREX);
							Lex = (LRT_CHAREX *) ptr;

							xChar=(HPDF_UINT) Lex->xChar;
							yChar=(HPDF_UINT) Lex->yChar;
							x=(HPDF_UINT) Lex->Point.x;
							y=(HPDF_UINT) ((lpLRInit->sPage.cy) - (Lex->Point.y));


							// carico il font (se già caricato una volta non lo carico +)
							idxFont = ARSearchIdx(arFonts,Lex->szFontName,FALSE);
							if (idxFont<0) {
								ARAdd(&arFonts,Lex->szFontName);
								sprintf(szfont,"c:\\WINDOWS\\Fonts\\%s.ttf",Lex->szFontName);
								pszfont_name = HPDF_LoadTTFontFromFile (pdf, szfont, HPDF_FALSE);
								ARAdd(&arNameFonts,pszfont_name);	
							}
							else pszfont_name=arNameFonts[idxFont];
							font = HPDF_GetFont (pdf, pszfont_name, NULL);

							HPDF_Page_SetFontAndSize (psPage[c], font, Lex->yChar-10);

							xx = HPDF_Page_TextWidth (psPage[c], pszString);

							if (x>lpLRInit->sPage.cx) x=(HPDF_UINT) (lpLRInit->sPage.cx  - xx - 100);
							if (Lex->Point.y<yChar) y-=100;

							// calcolo colore in RGB
							lColor = Lex->lColore;
							b=(float) ((lColor>>16)&0xFF)/255; 
							g=(float) ((lColor>>8)&0xFF)/255; 
							r=(float) (lColor&0xFF)/255;


							if (y>1) {

								HPDF_Page_SetRGBFill (psPage[c],
													  r,
													  g,
													  b);


								HPDF_Page_BeginText (psPage[c]);
								HPDF_Page_TextOut (psPage[c],(HPDF_REAL) x, (HPDF_REAL) y, pszString);
								HPDF_Page_EndText (psPage[c]);

							}
						}

					break;

					case LRE_CHARBOX: 
					{

						LRT_CHARBOX *Lex;
						CHAR *pszString=NULL;
						SINT  yChar,xChar;
						RECT rPos;
						HPDF_UINT uChar;

						LONG lColor;
						float r,g,b;
						HPDF_REAL xx;


						pszString=ptr+sizeof(LRT_CHARBOX);
						Lex = (LRT_CHARBOX *) ptr;

						memcpy(&rPos,&Lex->rArea,sizeof(RECT));
						rPos.top=(lpLRInit->sPage.cy-Lex->rArea.top )+70;//Lex->rArea.bottom;
						rPos.bottom=(lpLRInit->sPage.cy-Lex->rArea.bottom )+70;

						xChar=(HPDF_UINT) Lex->xChar;
						yChar=(HPDF_UINT) Lex->yChar;

						// calcolo colore in RGB
						lColor = Lex->lColore;
						b=(float) ((lColor>>16)&0xFF)/255; 
						g=(float) ((lColor>>8)&0xFF)/255; 
						r=(float) (lColor&0xFF)/255; 

						// carico il font (se già carica//to una volta non lo carico +)
						idxFont = ARSearchIdx(arFonts,Lex->szFontName,FALSE);
						if (idxFont<0) {
							ARAdd(&arFonts,Lex->szFontName);
							sprintf(szfont,"c:\\fonts\\%s.ttf",Lex->szFontName);
							pszfont_name = HPDF_LoadTTFontFromFile (pdf, szfont, HPDF_FALSE);
							ARAdd(&arNameFonts,pszfont_name);	
						}
						else pszfont_name=arNameFonts[idxFont];
						font = HPDF_GetFont (pdf, pszfont_name, NULL);

						HPDF_Page_SetFontAndSize (psPage[c], font, Lex->yChar-10);

						HPDF_Page_BeginText (psPage[c]);

						HPDF_Page_SetRGBFill (psPage[c],
											  r,
											  g,
											  b);

						// misura dello spazio occupato dal testo
						xx = HPDF_Page_TextWidth (psPage[c], pszString);
						
						// aggiungo spazio a seconda dell' allineamento
						if (Lex->xAllinea==LR_LEFT)  rPos.right+= (LONG) xx;
						if (Lex->xAllinea==LR_RIGHT) rPos.left-= (LONG) xx;
						if (Lex->xAllinea==LR_CENTER) {
							rPos.left-= (LONG) xx;
							rPos.right+= (LONG) xx;
						}
						// da implementare per LR_JUSTIFY

						HPDF_Page_TextRect (psPage[c],
											(HPDF_REAL) rPos.left,
											(HPDF_REAL) rPos.bottom,
											(HPDF_REAL) rPos.right,
											(HPDF_REAL) rPos.top,
											pszString,
											_LPdfTextAlign(Lex->xAllinea),
											&uChar);

						if (!uChar) printf("Pro");
						HPDF_Page_EndText (psPage[c]);

					}

					break;

					case LRE_BOX:
					case LRE_BOXP:
					case LRE_BOXR:
					case LRE_BOXPR:
					{

						LRT_BOXR *Lex;
						SINT x,y,iwidth,iheight;
						LONG lColor;
						float r,g,b;


						Lex = (LRT_BOXR *) ptr;

						x=Lex->rRect.left;
						//y=lpLRInit->sPage.cy-Lex->rRect.top;
						if (Lex->rRect.bottom>lpLRInit->sPage.cy) y=10;
						else y=(lpLRInit->sPage.cy-Lex->rRect.bottom);

						iwidth = Lex->rRect.right - Lex->rRect.left;

						if (Lex->rRect.bottom>lpLRInit->sPage.cy) iheight = Lex->rRect.bottom - Lex->rRect.top - 270;
						else iheight = Lex->rRect.bottom - Lex->rRect.top;


						// calcolo colore in RGB
						lColor = Lex->lColore;
						b=(float) ((lColor>>16)&0xFF)/255; 
						g=(float) ((lColor>>8)&0xFF)/255; 
						r=(float) (lColor&0xFF)/255; 


						//HPDF_Page_SetLineWidth (psPage[c], 0);

						HPDF_Page_SetRGBFill (psPage[c],
											  r,
											  g,
											  b);

						HPDF_Page_Rectangle(psPage[c],
											(HPDF_REAL) x,
											(HPDF_REAL) y,
											(HPDF_REAL) iwidth,
											(HPDF_REAL) iheight);

						if (arElement[a]->iType==LRE_BOX) HPDF_Page_Stroke (psPage[c]); 
						else HPDF_Page_Fill (psPage[c]);

						//HPDF_Page_ClosePathFillStroke (psPage[c]);
						//HPDF_Page_Stroke (psPage[c]);
						//HPDF_Page_Fill (psPage[c]);

					}

					break;

					case LRE_LINE:
						//LMultiLine(fPrinter,hDC,arElement[a]->iType,(LRT_LINE *) ptr,lpRect);
/*
						HPDF_Page_LineTo  (HPDF_Page  page,
										   HPDF_REAL  x,
										   HPDF_REAL  y);*/

					break;

					case LRE_HBITMAP:
					case LRE_HBITMAPD:
						//LIMGDisplay(fPrinter,hDC,arElement[a]->iType,(LRT_BOX *) ptr,lpRect);
					break;

					case LRE_IMAGE:
						//LImage(fPrinter,hDC,arElement[a]->iType,(LRT_IMAGE *) ptr,lpRect);

						//image = HPDF_LoadPngImageFromFile2 (pdf,"c:\\sprite\\account.png");
						//HPDF_Page_DrawImage (psPage[c], image, 1000, 1000, HPDF_Image_GetWidth (image), HPDF_Image_GetHeight (image));
					break;

				}
			}
		}
	}

	ARDestroy(arFonts);
	ARDestroy(arNameFonts);

	// save the document to a file 
	HPDF_SaveToFile (pdf, pFileName);
	ShellExecute(NULL,"open",pFileName,NULL,NULL,SW_NORMAL);

	ehFree(psPage);

	/* clean up */
	HPDF_Free (pdf);

}


void pdf_error_handler(HPDF_STATUS   error_no,
					   HPDF_STATUS   detail_no,
                       void          *user_data)
{
    printf ("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no, (HPDF_UINT)detail_no);
    longjmp(env, 1);
}

/*
void error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    printf ("ERROR: error_no=%04X, detail_no=%d\n", 
      (unsigned int) error_no, (int) detail_no);
    longjmp (env, 1); 
}
*/


SINT _LPdfTextAlign(SINT iAllinea)
{
	
	switch (iAllinea) {
		default:
		case LR_LEFT :
			return HPDF_TALIGN_LEFT ;
		break;
		case LR_CENTER :
			return HPDF_TALIGN_CENTER ;
		break;
		case LR_RIGHT :
			return HPDF_TALIGN_RIGHT ;
		break;
		case LR_JUSTIFY :
			return HPDF_TALIGN_JUSTIFY ;
		break;

	}
}


/*
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "/easyhand/Ehtoolx/libharu-2.1.0/include/hpdf.h"

jmp_buf env;

#ifdef HPDF_DLL
void  __stdcall
#else
void
#endif
error_handler (HPDF_STATUS   error_no,
               HPDF_STATUS   detail_no,
               void         *user_data)
{
    printf ("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no,
                (HPDF_UINT)detail_no);
    longjmp(env, 1);
}

const char *font_list[] = {
    "Courier",
    "Courier-Bold",
    "Courier-Oblique",
    "Courier-BoldOblique",
    "Helvetica",
    "Helvetica-Bold",
    "Helvetica-Oblique",
    "Helvetica-BoldOblique",
    "Times-Roman",
    "Times-Bold",
    "Times-Italic",
    "Times-BoldItalic",
    "Symbol",
    "ZapfDingbats",
    NULL
};

int main (int argc, char **argv)
{
    const char *page_title = "Font Demo";
    HPDF_Doc  pdf;
    char fname[256];
    HPDF_Page page;
    HPDF_Font def_font;
    HPDF_REAL tw;
    HPDF_REAL height;
    HPDF_REAL width;
    HPDF_UINT i;

    strcpy (fname, argv[0]);
    strcat (fname, ".pdf");

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        printf ("error: cannot create PdfDoc object\n");
        return 1;
    }

    if (setjmp(env)) {
        HPDF_Free (pdf);
        return 1;
    }

    // Add a new page object. 
    page = HPDF_AddPage (pdf);

    height = HPDF_Page_GetHeight (page);
    width = HPDF_Page_GetWidth (page);

    // Print the lines of the page. 
    HPDF_Page_SetLineWidth (page, 1);
    HPDF_Page_Rectangle (page, 50, 50, width - 100, height - 110);
    HPDF_Page_Stroke (page);

    // Print the title of the page (with positioning center). 
    def_font = HPDF_GetFont (pdf, "Helvetica", NULL);
    HPDF_Page_SetFontAndSize (page, def_font, 24);

    tw = HPDF_Page_TextWidth (page, page_title);
    HPDF_Page_BeginText (page);
    HPDF_Page_TextOut (page, (width - tw) / 2, height - 50, page_title);
    HPDF_Page_EndText (page);

    // output subtitle. 
    HPDF_Page_BeginText (page);
    HPDF_Page_SetFontAndSize (page, def_font, 16);
    HPDF_Page_TextOut (page, 60, height - 80, "<Standerd Type1 fonts samples>");
    HPDF_Page_EndText (page);

    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, 60, height - 105);

    i = 0;
    while (font_list[i]) {
        const char* samp_text = "abcdefgABCDEFG12345!#$%&+-@?";
        HPDF_Font font = HPDF_GetFont (pdf, font_list[i], NULL);

        // print a label of text 
        HPDF_Page_SetFontAndSize (page, def_font, 9);
        HPDF_Page_ShowText (page, font_list[i]);
        HPDF_Page_MoveTextPos (page, 0, -18);

        // print a sample text. 
        HPDF_Page_SetFontAndSize (page, font, 20);
        HPDF_Page_ShowText (page, samp_text);
        HPDF_Page_MoveTextPos (page, 0, -20);

        i++;
    }

    HPDF_Page_EndText (page);

    HPDF_SaveToFile (pdf, fname);

    // clean up 
    HPDF_Free (pdf);

    return 0;
}
*/
    