//   ----------------------------------------------
//   | IMGutilRes Image Utility + Resampling
//   |            Lettura e scrittura delle        
//   |            immagini                         
//   |                                           
//   |            Ha bisogno di:
//   |            - C:\FVisual\IMGTool2003\amalloc.cpp                               
//   |            - C:\FVisual\IMGTool2003\ImgResample.cpp                               
//   |            - C:\FVisual\IMGTool2003\resample_function.cpp                               
//   |            - C:\EHTOOL\cpucontrol.c                               
//   |                                           
//   |             8 Maggio 1999
//   |             18 Febbraio 2004
//   |             by Ferrà Art & Technology 
//   ----------------------------------------------

#include "\ehtool\include\ehsw_i.h"
#include <setjmp.h>
#include "\ehtool\imgutil.h"

static SINT iIMGMode=IMG_SINGLETHREAD;
static CRITICAL_SECTION csJpg;

void IMG_Mode(SINT cmd,SINT iMode)
{
	switch (cmd)
	{
		case WS_OPEN:
			InitializeCriticalSection(&csJpg);
			iIMGMode=iMode;
			break;
		
		case WS_CLOSE:
			DeleteCriticalSection(&csJpg);
			break;
	}
}

#ifndef _CONSOLE
void IMGDisp(SINT PosX,SINT PosY,SINT Hdl) {IMGDispEx(PosX,PosY,0,0,0,0,Hdl);}
void IMGDispEx(SINT PosX,SINT PosY,SINT SizeX,SINT SizeY,SINT OfsX,SINT OfsY,SINT Hdl)
{
 IMGHEADER *Img;
 HDC hDC;
 WORD    a=1;
 HBITMAP BitMap;
 BITMAPINFOHEADER BmpHeaderBackup;
 BITMAPINFOHEADER *BmpHeader;
 BITMAPINFO *BmpInfo;
 BYTE *Sorg;
// SINT TileY=1024;// Dimenzione piastrella verticale
// SINT TileX=1024;// Dimenzione piastrella orizzontale (Da Fare)
 SINT TileY=1024;// Dimenzione piastrella verticale
 SINT TileX=2048;// Dimenzione piastrella orizzontale (Da Fare)
 LONG Lx,Ly;
 BOOL Redim;
 SINT ReadPy=0,WritePy=0; 
 SINT ReadPx=0,WritePx=0; 
 SINT ReadSectY=0,ReadSectX=0; 
 SINT WriteSectY=0,WriteSectX=0; 
 LONG TileSizeY;

 Img=Wmemo_lockEx(Hdl,"IMGDispEx"); if (!Img) return;
 Sorg=(BYTE *) Img;
 Sorg+=Img->Offset;

 // -------------------------------------
 // DATI DELL'ICONE                     !
 // -------------------------------------
 hDC=GetDC(NULL); // DA VEDERE
 BmpInfo=(BITMAPINFO *) &Img->bmiHeader;
 BmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;
 memcpy(&BmpHeaderBackup,BmpHeader,sizeof(BmpHeaderBackup));

 // -------------------------------------
 // CREA ED STAMPA IL BITMAP            !
 // -------------------------------------
 
  Ly=BmpHeader->biHeight;
  Lx=BmpHeader->biWidth;
  
  // Inversamente proporzionale : y:1024=TileY:TileX;
  if (Lx>TileX) {TileX=Lx; TileY=TileY*1024/TileX;}
  
  // Calcolo automatico delle dimensioni orizzontali
  //  SizeY:Ly=x:Lx;
  if ((SizeY>0)&&(SizeX==0)) SizeX=SizeY*Lx/Ly;
  // Calcolo automatico delle dimensioni verticali
  if ((SizeX>0)&&(SizeY==0)) SizeY=SizeX*Ly/Lx;
  if (SizeX) Redim=TRUE; else Redim=FALSE;

  WritePy=0; 
  TileSizeY=Img->linesize*TileY;
  for (ReadPy=0;;ReadPy+=TileY)   // Loop Verticale
  {
	ReadSectY=(Ly-ReadPy); 
	if (ReadSectY>TileY) ReadSectY=TileY; 
	if (ReadSectY<1) break;		  // Fine del file

	if (Redim)
	{
	 if ((ReadPy+TileY)>=Ly) WriteSectY=(SizeY-WritePy); else WriteSectY=SizeY*ReadSectY/Ly;
	}

    WritePx=0;
	for (ReadPx=0;;ReadPx+=TileX) // Loop orizzontale
	{
		// Calcolo la grandezza del settore
		ReadSectX=(Lx-ReadPx); 
		if (ReadSectX>TileX) ReadSectX=TileX; 
		if (ReadSectX<1) break;  // Fine della lettura orizzontale

		if (Redim)
		{
			//   SizeY:Ly=x:ReadSectY
		    if ((ReadPx+TileX)>=Lx) WriteSectX=(SizeX-WritePx); else WriteSectX=SizeX*ReadSectX/Lx;
			//RealTileX=SizeX*ReadSectX/Lx;
			//WriteSectX=(SizeX-WritePx); if (WriteSectX>RealTileX) WriteSectX=RealTileX;
		}

//	 win_infoarg("Py=%d [%d]",Py,Img->linesize*SectY);
		BmpHeader->biHeight=-ReadSectY;
		BmpHeader->biWidth=ReadSectX;

		BitMap=CreateDIBitmap(hDC, //Handle del contesto
							  BmpHeader,
							  CBM_INIT,
							  Sorg,// Dati del'icone
						      BmpInfo,
						      DIB_RGB_COLORS);

		if (BitMap==NULL) {ReleaseDC(NULL,hDC); PRG_end("Grave 1");}
		WBmpDispDirect(BitMap,
			           WritePx+PosX,WritePy+PosY,
					   ReadSectX,ReadSectY,
					   WriteSectX,WriteSectY,
					   Redim);
		DeleteObject(BitMap);
		if (Redim) WritePy+=WriteSectY; else WritePy+=TileY;
	}
	Sorg+=TileSizeY;
  }
  ReleaseDC(NULL,hDC); 
  memcpy(BmpHeader,&BmpHeaderBackup,sizeof(BmpHeaderBackup));
  Wmemo_unlockEx(Hdl,"IMGDispEx");
}
#endif

//  --------------------------------------------------------------------------------------
//  | FORMATO JPEG                              
//  --------------------------------------------------------------------------------------

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void ima_jpeg_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  HWND hWnd;
  struct ima_error_mgr  * myerr = (struct ima_error_mgr * ) cinfo->err;
  CHAR buffer[JMSG_LENGTH_MAX];
  //PRG_end("jpeg error");
  /* Create the message */
  myerr->pub.format_message (cinfo, buffer);
  hWnd=GetFocus();
  win_infoarg("Ferra Image Manager:\n%s",buffer);
  SetFocus(hWnd);
  /* Send it to stderr, adding a newline */
  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
  //return;
}

void CreateGrayColourMap(BYTE *Palette,int nGrays)
{
  int a;
  RGBQUAD *pRGB=(RGBQUAD *) Palette;
  //byte g[256];
//  for (i=0; i<n; i++) 
	  //g[i] = (byte)(256*i/n); 
	  //for (i=0;i<3;i++) {memcpy(Zone,g,256); Zone+=256;}
  for (a=0;a<nGrays;a++)
  {
     byte col=(byte)(256*a/nGrays); 
	 pRGB[a].rgbBlue=col;
	 pRGB[a].rgbGreen=col;
	 pRGB[a].rgbRed=col;
     pRGB[a].rgbReserved=0;
  }
}

void SetPalette(BYTE *Palette,SINT nColors, BYTE *Red, BYTE *Green, BYTE *Blue)
{
 RGBQUAD *pRGB=(RGBQUAD *) Palette;
 int a;

 for (a=0;a<nColors;a++)
 {
   pRGB[a].rgbBlue=Blue[a];
   pRGB[a].rgbGreen=Green[a];
   pRGB[a].rgbRed=Red[a];
   pRGB[a].rgbReserved=0;
 }
}

void JPGSyncEnter(void)
{
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);
}

void JPGSyncLeave(void)
{
  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
}

// ---------------------------------------------------
// JPGReadFile
// LEGGE UN FILE IN FORMATO JPEG
//
// imageFileName Nome del file da leggere
// *HdlImage	 Puntatore all'handle che conterrà l'immagine
// TrueColor     TRUE=True Color/FALSE=256 colori
// *fStop        Puntatore ad un BOOL che determina lo stop del processo 
//               serve per applicazione Multi/Thread
//               NULL se non si usa
// Ritorna: 0 Errore
//          1 Tutto OK

BOOL JPGReadFile(CHAR *imageFileName,SINT *HdlImage,BOOL TrueColor,BOOL *fStop,SINT *lpiErr)
{
  /* Questa struttura contiene i parametri per la decompressione JPEG ed i puntatori al
   * working space (che sera' allocato come necessita dalla JPEG library).
   */
  IMGHEADER ImgHead;
  BYTE *GBuffer,*CB;
//  int register a;
  struct jpeg_decompress_struct cinfo;
  struct ima_error_mgr jerr;

  /* More stuff */
  FILE * infile;		/* Il File sorgente */
  JSAMPARRAY buffer;	/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  LONG MemoSize;
  /*
     In questo esempio dobbiamo aprire il file di input prima di ogni cosa ,
	 facendo cosi' setjmp() error recovery assume che il file sia aperto.
	 IMPORTANTE: usere l'opzione "b" per aprire il file
    
  */
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);

  *HdlImage=-1;

  // Apertura del file
  if ((infile=fopen(imageFileName,"rb")) == NULL) 
  {	 // win_infoarg("JPGReader:[%s] %d?",imageFileName,GetLastError());
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	  if (lpiErr) *lpiErr=-1;
	  return 0;
  }
  
//  --------------------------------------------------------------------------------------
//  Fase 1: alloco ed inizializzo l'oggetto JPEG per la gestione degli errori
//  Settiamo la normale JPEG error routines, che avverra in uscita con errore.
//  --------------------------------------------------------------------------------------
  //memset(&cinfo,0,sizeof(cinfo));
  cinfo.err = jpeg_std_error(&jerr.pub);
  jpeg_create_decompress(&cinfo);

  cinfo.err = jpeg_std_error(&jerr.pub);
  //
  jerr.pub.error_exit = ima_jpeg_error_exit; // Dichiarata in testa

 /* Initialize JPEG parameters.
   * Much of this may be overridden later.
   * In particular, we don't yet know the input file's color space,
   * but we need to provide some value for jpeg_set_defaults() to work.
   */

  //cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
  //jpeg_set_defaults(&cinfo);
  cinfo.dct_method=JDCT_IFAST;
  //dispx("         [%d]",cinfo.scale_num);
  // Stabilisco il salto di ritorno su errore nel mio contesto.
  // Se avviene un errore JPEG lo segnala
  // Noi dobbiamo pulire JPEG object,chiudere l'input file, e ritornare

  // Possiamo ora inizializzare l'oggetto JPEG per la decompressione
  //jpeg_create_decompress(&cinfo);
  if (setjmp(jerr.setjmp_buffer))  
	{jpeg_destroy_decompress(&cinfo); 
     fclose(infile);
	 if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	  if (lpiErr) *lpiErr=-2;
	 return 0;
	}

//  --------------------------------------------------------------------------------------
//  Fase 2 : specifico lo stream del file contenente il JPEG
//  --------------------------------------------------------------------------------------
  jpeg_stdio_src(&cinfo, infile);

//  --------------------------------------------------------------------------------------
//  Fase 3 : Leggo l'header del JPEG
//  --------------------------------------------------------------------------------------
  jpeg_read_header(&cinfo, TRUE);
  if (fStop) {if (*fStop) goto STOP1;}

 //  --------------------------------------------------------------------------------------
//  Fase 4 : Setto i parametri per la decompressione
//  --------------------------------------------------------------------------------------

  if (!TrueColor)
  {
   if (cinfo.jpeg_color_space!=JCS_GRAYSCALE) 
   {
	cinfo.quantize_colors = TRUE;
	cinfo.desired_number_of_colors = 256;
   }
  }

  //cinfo.scale_num=1;
  //cinfo.scale_denom=2;

//  --------------------------------------------------------------------------------------
//  Fase 5 : Avvio il decompressore
//  --------------------------------------------------------------------------------------
  jpeg_start_decompress(&cinfo);

  // Potremmo avere bisogno di fare il setup di un nostro "own" in questo punto prima di leggere i dati.
  // Dopo il "jpeg_start_decompress()"  abbiamo la corretta dimensione dell'immagine disponibile.
  // e la colormap di uscita se desideriamo la quantizzazione del colore
  // delle dimensioni
  //Create(cinfo.image_width, cinfo.image_height, 8*cinfo.output_components);
/*
  switch (cinfo.jpeg_color_space)
  {
     case JCS_UNKNOWN: lpsJcs="Sconosciuto"; break;
	 case JCS_GRAYSCALE: lpsJcs="Monocromatico"; break;
	 case JCS_RGB: lpsJcs="RGB"; break;
	 case JCS_YCbCr: lpsJcs="YUV"; break;
	 case JCS_CMYK: lpsJcs="CMYN"; break;
	 case JCS_YCCK: lpsJcs="YCCK"; break;
  }
  win_infoarg("Create [%s] (%d,%d,%d)",lpsJcs,cinfo.image_width, cinfo.image_height,cinfo.output_components);
  */
  // Tira fuori il formato

//  --------------------------------------------------------------------------------------
//  FASE 6: Riempo la struttura IMGHEADER 
//  --------------------------------------------------------------------------------------
  // Specifico i dati dell'header
  //memset(&ImgHead,0,sizeof(ImgHead));
  ZeroFill(ImgHead);
  ImgHead.Type=IMG_JPEG;
  strcpy(ImgHead.FileName,imageFileName);
  ImgHead.lSize=file_len(imageFileName);
  ImgHead.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
  ImgHead.bmiHeader.biWidth=cinfo.image_width;
  ImgHead.bmiHeader.biHeight=cinfo.image_height; // forse * -1
  ImgHead.bmiHeader.biPlanes=1;
  ImgHead.bmiHeader.biBitCount=8*cinfo.output_components;// bit colore
//  ImgHead.bmiHeader.biCompression=BI_RGB;// Non compresso
  ImgHead.bmiHeader.biCompression=BI_RGB;// Non compresso
  ImgHead.bmiHeader.biSizeImage=0;
  ImgHead.bmiHeader.biClrUsed=0;// Colori usati 0=Massimo
  ImgHead.bmiHeader.biClrImportant=0;// 0=Tutti i colori importanti
  
  // JSAMPLEs per riga dell'output buffer
  row_stride = cinfo.output_width * cinfo.output_components;
  ImgHead.linesize=row_stride;

  // Allineo a 32bit
  ImgHead.linesize=((ImgHead.linesize+3)>>2)<<2;

  MemoSize=sizeof(IMGHEADER)+(cinfo.image_height*ImgHead.linesize);
  // C'e' la pallette
  if (cinfo.output_components==1) MemoSize+=sizeof(RGBQUAD)*256;
  
  if (fStop) {if (*fStop) goto STOP2;}

  //win_infoarg("%d",cinfo.output_components);
  *HdlImage=memo_chiedi(RAM_AUTO,MemoSize,file_name(imageFileName));
  if (*HdlImage<0) PRG_end("No memory");

  GBuffer=Wmemo_lockEx(*HdlImage,"JPGReadFile"); 
  ImgHead.Offset=sizeof(ImgHead);
  // Carico la palettes
  if (ImgHead.bmiHeader.biBitCount==8)
  {
    if (cinfo.jpeg_color_space==JCS_GRAYSCALE) 
      CreateGrayColourMap(GBuffer+sizeof(ImgHead),256);
      else
	  SetPalette(GBuffer+sizeof(ImgHead),cinfo.actual_number_of_colors, cinfo.colormap[0], cinfo.colormap[1], cinfo.colormap[2]);
	  
	ImgHead.Offset+=(256*sizeof(RGBQUAD));
  }

  memcpy(GBuffer,&ImgHead,sizeof(ImgHead));
  GBuffer+=ImgHead.Offset;

  // Costruisco una riga/array di appoggio che che verra liberata quando l'immagine verra' fatta
  buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

//  --------------------------------------------------------------------------------------
//  Fase 7  : Loop sulle linee che rimangono da leggere
//  Usiamo per il loop una variabile di stato delle librerie cinfo.output_scanline
//  come contatore, cosi fancendo non .....
//  --------------------------------------------------------------------------------------

  CB=GBuffer;
  while (cinfo.output_scanline < cinfo.output_height) {
	 
	 jpeg_read_scanlines(&cinfo, buffer, 1);
     //if (cinfo.output_scanline <10) win_infoarg("%d",cinfo.output_scanline );
	 // Memorizzo le linee in un buffer
	 //memo_scrivivar(*HdlImage,PtDest,buffer,row_stride);
	 //memset(GBuffer,120,row_stride);

	 memcpy(GBuffer,buffer[0],ImgHead.linesize);
	 if (fStop) {if (*fStop) break;}

	 //PtDest+=row_stride;
     // memset(GBuffer,0,3*10);
	 GBuffer+=ImgHead.linesize;
  }

//  --------------------------------------------------------------------------------------
//  Fase 8
//  Fine della decompressione rilascio le memorie usate
//  --------------------------------------------------------------------------------------

STOP2:
  if (fStop) 
	{if (*fStop) jpeg_abort_decompress(&cinfo);}
	else
    jpeg_finish_decompress(&cinfo);

  // Ignoriamo un valore di fine sospessione possibile dello standard IO data source.

  /* Step 8: Release JPEG decompression object */
  /* This is an important step since it will release a good deal of memory. */

//  --------------------------------------------------------------------------------------
//  Fase 9
//  Rilascio il JPEG decompression Object
//  --------------------------------------------------------------------------------------

STOP1:

   jpeg_destroy_decompress(&cinfo);
   fclose(infile);

   // Richiesto stop
   if (fStop) 
   {
	 if (*fStop)
	 {
	   if (*HdlImage>0)
	   {
		 Wmemo_unlockEx(*HdlImage,"JPGReadFile");	
		 memo_libera(*HdlImage,"Stop");
		 *HdlImage=-1;
	   }
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
      return FALSE;
	 }
   }
   
//  --------------------------------------------------------------------------------------
//  Fase 10
//  Inverto i colori per quello stronzo di "windows" che li vuole in formato BGR
//  --------------------------------------------------------------------------------------

  if (cinfo.output_components==3)
  {
   SINT kx,ky;
   BYTE *ap;
   for (ky=0;ky<(SINT) cinfo.image_height;ky++)
   {
	   ap=CB;
	   for (kx=0;kx<(ImgHead.linesize-2);kx+=3)
		{
		 BYTE k;
		 k=*ap; *ap=*(ap+2); *(ap+2)=k;
		 ap+=3;
		}
       CB+=ImgHead.linesize;
   }
  }

  Wmemo_unlockEx(*HdlImage,"JPGReadFile2");

//  --------------------------------------------------------------------------------------
//  E siamo fatti ....!!!
  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
  return 1;
}

// ---------------------------------------------------
// JPGSaveFile
// SCRIVE UN FILE IN FORMATO JPEG
//
// imageFileName Nome del file da scrivere
// HdlImage	     Hdl che contiene l'immagine in formato IMGHEADER
// iQuality      Fattore di qualità dell'immagine
BOOL JPGSaveFile(CHAR *imageFileName,SINT HdlImage,SINT iQuality)
{
  FILE * outfile;		/* Il File sorgente */
  SINT rt;
  // Apertura del file
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);
  if ((outfile=fopen(imageFileName,"wb")) == NULL) 
  {	 
	  win_infoarg("JPGSaveFile():Errore in creazione file [%s]",imageFileName);
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	  return 0;
  }

  rt=JPGPutStream(outfile,HdlImage,iQuality);
  fclose(outfile);
  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
  return rt;
}

BOOL JPGPutStream(FILE *outfile,SINT HdlImage,SINT iQuality)
{
  IMGHEADER *Img;
  //JSAMPLE *image_buffer;
  BYTE *GBuffer;
  struct jpeg_compress_struct cinfo;

  /* Usiamo una nostra gestione privata di JPEG error handler. */
  struct ima_error_mgr jerr;

  /* More stuff */
  JSAMPARRAY buffer;	/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
//  LONG MemoSize;
  BYTE *lpLine;
  /*
     In questo esempio dobbiamo aprire il file di input prima di ogni cosa ,
	 facendo cosi' setjmp() error recovery assume che il file sia aperto.
	 IMPORTANTE: usere l'opzione "b" per aprire il file
    
  */
  SINT kx;

  Img=Wmemo_lock(HdlImage);
  //BmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;
  GBuffer=(BYTE *) Img;
  GBuffer+=Img->Offset;

//  --------------------------------------------------------------------------------------
//  Fase 1: alloco ed inizializzo l'oggetto JPEG per la gestione degli errori
//  Settiamo la normale JPEG error routines, che avverra in uscita con errore.
//  --------------------------------------------------------------------------------------
  memset(&cinfo,0,sizeof(cinfo));
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = ima_jpeg_error_exit; // Dichiarata in testa

  // Stabilisco il salto di ritorno su errore nel mio contesto.
  // Se avviene un errore JPEG lo segnala
  // Noi dobbiamo pulire JPEG object,chiudere l'input file, e ritornare

  if (setjmp(jerr.setjmp_buffer)) {jpeg_destroy_compress(&cinfo); fclose(outfile); Wmemo_unlockEx(HdlImage,"JpgPutStream"); return 0;}
  
  // Possiamo ora inizializzare l'oggetto JPEG per la compressione
  jpeg_create_compress(&cinfo);

//  --------------------------------------------------------------------------------------
//  Fase 2 : specifico lo streem del file contenente il JPEG
//  --------------------------------------------------------------------------------------
  jpeg_stdio_dest(&cinfo, outfile);

//  --------------------------------------------------------------------------------------
//  Fase 3 : Setto i parametri per la compressione
//  --------------------------------------------------------------------------------------

  // Descrivo la grandezza il tipo dell'immagine
  // 4 campi nella struttura cinfo devono essere riempiti:
  ///
  cinfo.image_width = Img->bmiHeader.biWidth; 
  cinfo.image_height = Img->bmiHeader.biHeight;
  cinfo.input_components = Img->bmiHeader.biBitCount>>3;
  cinfo.in_color_space = JCS_RGB; 	// colorspace of input image 

  // Setto i parametri di default nella libreria
  // (You must set at least cinfo.in_color_space before calling this,
  // since the defaults depend on the source color space.)
  jpeg_set_defaults(&cinfo);

  // Setto un parametro non di "default"; la qualità
  // Here we just illustrate the use of quality (quantization table) scaling:
  jpeg_set_quality(&cinfo, iQuality, TRUE); /* limit to baseline-JPEG values */

  //  --------------------------------------------------------------------------------------
  //  Fase 4 : Parte il compressore
  //  --------------------------------------------------------------------------------------
  // TRUE ensures that we will write a complete interchange-JPEG file.
  // Pass TRUE unless you are very sure of what you're doing.
  jpeg_start_compress(&cinfo, TRUE);

  row_stride = Img->bmiHeader.biWidth*cinfo.input_components;	/* JSAMPLEs per row in image_buffer */

  // Costruisco una riga/array di appoggio che che verra liberata quando l'immagine verra' fatta
  buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
/*
  while (cinfo.next_scanline < cinfo.image_height) {
    // jpeg_write_scanlines espetta un array of puntatori alle "scanlines".
    // L'array qui è di un solo elemento, ma puoi passare più di un elemento alla volta
    // se pensi sia più conveniente
    row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
*/

//  --------------------------------------------------------------------------------------
//  Fase 5  : Loop sulle linee per scriverle
//  --------------------------------------------------------------------------------------
  
  while (cinfo.next_scanline < cinfo.image_height) 
  {
	 // Leggo la linea 
	 lpLine=buffer[0];
	 memcpy(lpLine,GBuffer,row_stride);
	 
	 // Converto BGR con RGB (sempre per lo stronzo di windows)
	 for (kx=0;kx<(row_stride-2);kx+=3)
		{
		 BYTE k;
		 k=*lpLine; *lpLine=*(lpLine+2); *(lpLine+2)=k;
		 lpLine+=3;
		}
	  
	 // SCrivo la linea
	 jpeg_write_scanlines(&cinfo, buffer, 1);
	 GBuffer+=Img->linesize;
  }

//  --------------------------------------------------------------------------------------
//  Fase 6  : Libero le risorse impegnate
//  --------------------------------------------------------------------------------------
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  
  Wmemo_unlockEx(HdlImage,"JPGPutStream2");

//  --------------------------------------------------------------------------------------
//  E siamo fatti ....!!!
  return 1;
}

BOOL JPGReadHeader(CHAR *imageFileName,IMGHEADER *ImgHead)
{
  struct jpeg_decompress_struct cinfo;

  struct ima_error_mgr jerr;

  FILE * infile;		/* Il File sorgente */
  int row_stride;		/* physical row width in output buffer */

  if (!imageFileName) return FALSE;
  if (!*imageFileName) return FALSE;
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);

  if ((infile=fopen(imageFileName,"rb")) == NULL) 
  {	
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	  return FALSE;
  }

//  --------------------------------------------------------------------------------------
//  Fase 1: alloco ed inizializzo l'oggetto JPEG per la gestione degli errori
//  Settiamo la normale JPEG error routines, che avverra in uscita con errore.
//  --------------------------------------------------------------------------------------
  memset(&cinfo,0,sizeof(cinfo));
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = ima_jpeg_error_exit; // Dichiarata in testa

  // Stabilisco il salto di ritorno su errore nel mio contesto.
  // Se avviene un errore JPEG lo segnala
  // Noi dobbiamo pulire JPEG object,chiudere l'input file, e ritornare

  if (setjmp(jerr.setjmp_buffer))  {//jpeg_destroy_decompress(&cinfo); 
									fclose(infile);
									if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
									return FALSE;}
  // Possiamo ora inizializzare l'oggetto JPEG per la decompressione
  jpeg_create_decompress(&cinfo);

//  --------------------------------------------------------------------------------------
//  Fase 2 : specifico lo streem del file contenente il JPEG
//  --------------------------------------------------------------------------------------
  jpeg_stdio_src(&cinfo, infile);

//  --------------------------------------------------------------------------------------
//  Fase 3 : Leggo l'header del JPEG
//  --------------------------------------------------------------------------------------
  jpeg_read_header(&cinfo, TRUE);

//  --------------------------------------------------------------------------------------
//  FASE 6: Riempo la struttura IMGHEADER 
//  --------------------------------------------------------------------------------------
  jpeg_start_decompress(&cinfo);
  memset(ImgHead,0,sizeof(IMGHEADER));
/*
  switch (cinfo.jpeg_color_space)
  {
	case JCS_GRAYSCALE: ImgHead->bmiHeader.biBitCount=8; break;
	case JCS_RGB: ImgHead->bmiHeader.biBitCount=24; break;
	case JCS_CMYK: ImgHead->bmiHeader.biBitCount=32; break;
	default: ImgHead->bmiHeader.biBitCount=0; break;
  }
*/
  // Specifico i dati dell'header
  ImgHead->Type=IMG_JPEG;
  strcpy(ImgHead->FileName,imageFileName);
  ImgHead->lSize=file_len(imageFileName);
  ImgHead->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
  ImgHead->bmiHeader.biWidth=cinfo.image_width;
  ImgHead->bmiHeader.biHeight=cinfo.image_height; // forse * -1
  ImgHead->bmiHeader.biPlanes=1;
  ImgHead->bmiHeader.biBitCount=cinfo.output_components*8;
  ImgHead->bmiHeader.biCompression=BI_RGB;// Non compresso
  ImgHead->bmiHeader.biSizeImage=0;
  ImgHead->bmiHeader.biClrUsed=0;// Colori usati 0=Massimo
  ImgHead->bmiHeader.biClrImportant=0;// 0=Tutti i colori importanti
  row_stride = cinfo.output_width * cinfo.output_components;
  ImgHead->linesize=row_stride;
  ImgHead->linesize=((ImgHead->linesize+3)>>2)<<2;

  jpeg_destroy_decompress(&cinfo);
  fclose(infile);
  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
  return TRUE;
}


void IMGCalc(SINT *SizeX,SINT *SizeY,SINT Hdl)
{
 IMGHEADER *Img;
 WORD    a=1;
 BITMAPINFOHEADER BmpHeaderBackup;
 BITMAPINFOHEADER *BmpHeader;
 BITMAPINFO *BmpInfo;
 BYTE *Sorg;
 LONG Lx,Ly;

 //win_info("=%d",Hdl);
 Img=Wmemo_lockEx(Hdl,"IMGCalc");
 Sorg=(BYTE *) Img;
 Sorg+=Img->Offset;

 BmpInfo=(BITMAPINFO *) &Img->bmiHeader;
 BmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;
 memcpy(&BmpHeaderBackup,BmpHeader,sizeof(BmpHeaderBackup));

 Ly=BmpHeader->biHeight;
 Lx=BmpHeader->biWidth;
 //win_infoarg("%d,%d, (%d,%d)",SizeX,SizeY,Lx,Ly);
 //  SizeY:Ly=x:Lx;
 if ((*SizeY>0)&&(*SizeX==0)) *SizeX=*SizeY*Lx/Ly;
 // Calcolo automatico delle dimensioni verticali
 if ((*SizeX>0)&&(*SizeY==0)) *SizeY=*SizeX*Ly/Lx;
 
 Wmemo_unlockEx(Hdl,"IMGCalc");
 return;
}

// ------------------------------------------------------------------
// IMGRemaker
// Costruisce un immagine partendo da un'altra in altre dimensioni
// Per ora solo true color
// ------------------------------------------------------------------
static SINT LocalIMGRemaker(SINT HdlImage,SINT xNew,SINT yNew);
static SINT LocalIMGRemakerAA(SINT HdlImage,SINT xNew,SINT yNew,SINT iTRS);

SINT IMGRemaker(SINT HdlImage,SINT xNew,SINT yNew,SINT fPhotoQuality,SINT iTRS)
{
  if (fPhotoQuality) return LocalIMGRemakerAA(HdlImage,xNew,yNew,iTRS);
  return LocalIMGRemaker(HdlImage,xNew,yNew);
}

static SINT LocalIMGRemaker(SINT HdlImage,SINT xNew,SINT yNew)
{
	BYTE *lpi;
	IMGHEADER *ImgSorg,*ImgDest;
	SINT iNewSize;
	//SIZE sImage;
	SINT iLineSize;
	SINT HdlNew;
	BYTE *lpSorg,*lpDest;
	BITMAPINFOHEADER *BHSorg;
	BITMAPINFOHEADER *BHDest;
	POINT pSorg;
	SINT yPf,iPf;
	SINT xc;
	register SINT x,y;
   	
	lpi=Wmemo_lockEx(HdlImage,"LocalIMGRemaker");
	//if (lpi==NULL) PRG_end("LocalIMGRemaker(): ");
	ImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+ImgSorg->Offset;
	
	BHSorg=(BITMAPINFOHEADER *) &ImgSorg->bmiHeader;
	//sImage.cy=BHSorg->biHeight;
    // sImage.cx=BHSorg->biWidth;
	
	if (ImgSorg->bmiHeader.biBitCount!=24) 
	{
		//iLineSize=sImage.cx*3; iLineSize=((iLineSize+3)>>2)<<2;
		//iNewSize=(iLineSize*sImage.cy)+ImgSorg->Offset;
		HdlNew=memo_chiedi(RAM_AUTO,sys.memolist[HdlImage].dwSize,"NewImage");
		//memo_copy(
		memo_copyall(HdlImage,HdlNew);
		Wmemo_unlockEx(HdlImage,"LocalIMGRemaker");
		return HdlNew;
		//PRG_end("IMGRemaker: no TRUECOLOR %d",ImgSorg->bmiHeader.biBitCount);
	}

	// Ricalcolo l'occupazione di spazio

	// Quantizzazione a 32 bit 
	iLineSize=xNew*3; iLineSize=((iLineSize+3)>>2)<<2;
//	win_infoarg("%d - %d - %d",xNew,xNew*3,iLineSize);
	iNewSize=(iLineSize*yNew)+ImgSorg->Offset;

	// Alloco la memoria
	HdlNew=memo_chiedi(RAM_AUTO,iNewSize,"NewImage");
	if (HdlNew<0) PRG_end("IMGRemaker:non memory");
	lpDest=Wmemo_lockEx(HdlNew,"LocalIMGRemaker2");
	memset(lpDest,0xFF,iNewSize);

	// Copio l'header
	memcpy(lpDest,lpi,ImgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	ImgDest=(IMGHEADER *) lpDest;
	ImgDest->linesize=iLineSize;
	BHDest=(BITMAPINFOHEADER *) &ImgDest->bmiHeader;
	BHDest->biHeight=yNew;
	BHDest->biWidth=xNew;
	lpDest+=ImgDest->Offset;

	// Effettuo lo streching da Grande a piccolo
//	if ((xNew<BHSorg->biWidth)&&(yNew<BHSorg->biHeight)) 
	{
	 for (y=0;y<yNew;y++)
	 {
		// pSorg.y= Posizione Y del sorgente formula  y:pSorg.y=yNew:BHSorg->biHeight
		pSorg.y=y*BHSorg->biHeight/yNew;
		// yPF Puntantore a Y Reale calcolato su ImgSorg->linesize
		yPf=(pSorg.y*ImgSorg->linesize);
		xc=0;
		for (x=0;x<xNew;x++)
		{
			// Calcolo la posizione del punto X nel sorgente
			// xSorg? : xSorgSize = yDest : yDestSize;
			pSorg.x=x*BHSorg->biWidth/xNew;
			// Calcolo la posizione fisica
			iPf=yPf+(pSorg.x*3);
			// Copia il pixel
			memcpy(lpDest+xc,lpSorg+iPf,3);
			xc+=3;
		}
		lpDest+=ImgDest->linesize;
	 }
	}
	// Effettuo lo streching da Piccolo -> a GRANDE
	/*
	else
	{
	 for (y=0;y<yNew;y++)
	 {
		// pSorg.y= Posizione Y del sorgente formula  y:pSorg.y=yNew:BHSorg->biHeight
		pSorg.y=y*BHSorg->biHeight/yNew;
		// yPF Puntantore a Y Reale calcolato su ImgSorg->linesize
		yPf=(pSorg.y*ImgSorg->linesize);
		xc=0;
		memset(lpDest+xc,0,3);
		lpDest+=ImgDest->linesize;
	 }

	}
	*/
	Wmemo_unlockEx(HdlNew,"LocalIMGRemaker2");
	Wmemo_unlockEx(HdlImage,"LocalIMGRemaker3");
	return HdlNew;
}


// ------------------------------------------------------------------
// IMGRemakerAA
// Costruisce un immagine partendo da un'altra in altre dimensioni
// Effettua antialiasing sulla compressione
// Per ora solo true color
// ------------------------------------------------------------------

static SINT LocalIMGRemakerAA(SINT HdlImage,SINT xNew,SINT yNew,SINT iTRS)
{
	BYTE *lpi;
	IMGHEADER *ImgSorg;
	SIZE sImage;
	SINT HdlNew;
	BYTE *lpSorg;
	BITMAPINFOHEADER *BHSorg;
   	
	lpi=Wmemo_lockEx(HdlImage,"LocalIMGRemakerAA");
	ImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+ImgSorg->Offset;
	
	BHSorg=(BITMAPINFOHEADER *) &ImgSorg->bmiHeader;
	sImage.cy=BHSorg->biHeight;
    sImage.cx=BHSorg->biWidth;

    // Se le dimensioni sono uguali o maggiori usa l'altro metodo
	/*
	if ((xNew>=sImage.cx)&&(yNew>=sImage.cy))
	{
	 	Wmemo_unlockEx(HdlImage,"LocalIMGRemakerAA1");
		return LocalIMGRemaker(HdlImage,xNew,yNew);
	}
	*/

	if (ImgSorg->bmiHeader.biBitCount!=24) 
	{
		HdlNew=memo_chiedi(RAM_AUTO,sys.memolist[HdlImage].dwSize,"NewImage");
		memo_copyall(HdlImage,HdlNew);
		Wmemo_unlockEx(HdlImage,"LocalIMGRemakerAA2");
		return HdlNew;
	}

	// Se le dimensioni sono uguali o maggiori usa l'altro metodo
	if ((xNew==sImage.cx)&&(yNew==sImage.cy))
	{
	 	Wmemo_unlock(HdlImage);
		return LocalIMGRemaker(HdlImage,xNew,yNew);
	}
	else
	{
		HdlNew=IMGResampling(HdlImage,  // Handle dell'immagine
							 NULL, // NULL=Tutta l'immagine rettangolo dell'area interessata
							 xNew, 
							 yNew, 
							 iTRS);
		return HdlNew;
	}
}

// ----------------------------------------------------------------------------
// JPGNewFile
// Crea un nuovo file di nuove dimensioni e qualità differenti 
// partendo da un'altro file JPG
//
// NB: uno dei due Lx/Ly deve essere a 0
//
// Ritorna  0= Tutto OK
//         -1= Errore in lettura file
//         -2= Errore in scrittura file
//         -3= Errore in ridimensionamento
//
SINT JPGNewFile(CHAR *lpFileSource,CHAR *lpFileDest,
				SINT iLx,SINT iLy,
				SINT iQuality,
				BOOL fAntiAlias,
				SINT iTRS)
{
	SINT HdlImage;
	SINT HdlImageNew;
	SINT err;
    IMGHEADER ImgHead;
	
	// Leggo l'header
	if (!JPGReadHeader(lpFileSource,&ImgHead)) return -1;

	// Calcolo automatico delle dimensioni orizzontali
    //  SizeY:Ly=x:Lx;
    if ((iLy>0)&&(iLx==0)) iLx=iLy*ImgHead.bmiHeader.biWidth/ImgHead.bmiHeader.biHeight;
    // Calcolo automatico delle dimensioni verticali
    if ((iLx>0)&&(iLy==0)) iLy=iLx*ImgHead.bmiHeader.biHeight/ImgHead.bmiHeader.biWidth;

	if (!JPGReadFile(lpFileSource,&HdlImage,TRUE,NULL,NULL)) return -1;
	HdlImageNew=IMGRemaker(HdlImage,iLx,iLy,fAntiAlias,iTRS);
				    
	if (HdlImageNew<0)
	{
		//win_infoarg("JPGNewFile:Errore in ridimensionamento %s",lpFileSource);
		memo_libera(HdlImage,"Img1");
		return -3;
	}

	err=JPGSaveFile(lpFileDest,HdlImageNew,iQuality);
	memo_libera(HdlImage,"Img1");
	memo_libera(HdlImageNew,"Img1");
	if (!err) return -2;
	return 0;
}

// ----------------------------------------------------------------------------
// IMGTopdown
// Mette sotto sopra il bitmap (Necessario per alcuni tipi di driver stampanti
//
//
//
void IMGTopdown(SINT HdlImage)
{
	IMGHEADER *ImgSorg;
	BITMAPINFOHEADER *BHSorg;
	BYTE *lpi;
	BYTE *lpj;
	BYTE *lpSorg;
	SIZE sImage;
	SINT x,y;
	SINT HdlMemo;
	BYTE *lpMemo;

	lpi=Wmemo_lock(HdlImage);
	ImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+ImgSorg->Offset;
	
	BHSorg=(BITMAPINFOHEADER *) &ImgSorg->bmiHeader;
	sImage.cy=BHSorg->biHeight;
    sImage.cx=BHSorg->biWidth;

	if (ImgSorg->bmiHeader.biBitCount!=24) {PRG_end("IMGTopdown solo a 24 bit");}

	// -----------------------------------------------
	// Chiedo memoria per il processo
	//
	HdlMemo=memo_chiedi(RAM_AUTO,sImage.cy*3,"Buff");
	lpMemo=Wmemo_lock(HdlMemo);
	
	// -----------------------------------------------
	// Effettuo il mirroring
	//
	
	for (x=0;x<sImage.cx;x++)
	{
		lpj=lpSorg+x*3;
		for (y=0;y<sImage.cy;y++)
		{	
			memcpy(lpMemo+(y*3),lpj,3); lpj+=ImgSorg->linesize;
		}

		lpj-=ImgSorg->linesize;
		for (y=0;y<sImage.cy;y++)
		{	
			memcpy(lpj,lpMemo+(y*3),3); lpj-=ImgSorg->linesize;
		}
	}
	BHSorg->biHeight=-BHSorg->biHeight;
	memo_libera(HdlMemo,"Buf");
	Wmemo_unlockEx(HdlImage,"IMGTopDown");
}


/*
#define MAX_PIXEL_VALUE 255

#define MAX_OUT_DIMENSION ((MAX_OUT_WIDTH > MAX_OUT_HEIGHT) ? \
                           MAX_OUT_WIDTH : MAX_OUT_HEIGHT)

float a;

static float C0(float t) {return -a * t * t * t + a * t * t;}
static float C1(float t) {return -(a + 2.0f) * t * t * t + (2.0f * a + 3.0f) * t * t - a * t;}
static float C2(float t) {return (a + 2.0f) * t * t * t - (a + 3.0f) * t * t + 1.0f;}
static float C3(float t) {return a * t * t * t - 2.0f * a * t * t + a * t;}

void IMGMagnify(void)
{
  int in_width, in_height, out_width, out_height, larger_out_dimension;
  int n, d, j, k, l, m, index;
  //int lcs[MAX_OUT_DIMENSION];
  SINT *lcs;
  //BYTE f[MAX_IN_HEIGHT][MAX_IN_WIDTH];
  //BYTE g[MAX_OUT_HEIGHT][MAX_OUT_WIDTH];
  float x;
  //float c[4][MAX_OUT_DIMENSION];
  float *tvc;
  //float h[MAX_IN_WIDTH];
  float *lph;


  larger_out_dimension = (out_width > out_height) ? out_width : out_height;
  lcs=malloc(larger_out_dimension*sizeof(SINT));
  tvc=malloc(larger_out_dimension*4*sizeof(float));
  lph=malloc(out_width*sizeof(float));
  for (k = 0; k < larger_out_dimension; k++) lcs[k] = (k * d) / n;

  for (k = 0; k < n; k++) {
    x = (float)((k * d) % n) / (float)n;
    tvc[(0+1)*k] = C0(x);
    tvc[(1+1)*[k] = C1(x);
    tvc[(2+1)*k] = C2(x);
    tvc[(3+1)*k] = C3(x);
  }

  for (k = n; k < larger_out_dimension; k++)
    for (l = 0; l < 4; l++)
      tvc[(l+1*k)] = tvc[(l+1)*(k % n)];

  for (k = 0; k < out_height; k++) {
    for (j = 0; j < in_width; j++) {
      h[j] = 0.0f;
      for (l = 0; l < 4; l++) {
        index = lcs[k] + l - 1;
        if ((index >= 0) && (index < in_height))
          h[j] += f[index][j] * c[3 - l][k];
      }
    }
    for (m = 0; m < out_width; m++) {
      x = 0.5f;
      for (l = 0; l < 4; l++) {
        index = lcs[m] + l - 1;
        if ((index >= 0) && (index < in_width))
          x += h[index] * c[3 - l][m];
      }
      if (x <= 0.0f)
        g[k][m] = 0;
      else if (x >= MAX_PIXEL_VALUE)
        g[k][m] = MAX_PIXEL_VALUE;
      else
        g[k][m] = (unsigned char)x;
    }
  }
  free(lcs);
  free(tvc);

}

*/


// ---------------------------------------------------
// JPGReadFileEx
// LEGGE UN FILE IN FORMATO JPEG
//
// imageFileName Nome del file da leggere
// *HdlImage	 Puntatore all'handle che conterrà l'immagine
// TrueColor     TRUE=True Color/FALSE=256 colori
// *fStop        Puntatore ad un BOOL che determina lo stop del processo 
//               serve per applicazione Multi/Thread
//               NULL se non si usa
// Ritorna: 0 Errore
//          1 Tutto OK
BOOL JPGReadFileEx(CHAR *imageFileName,
				   SINT *HdlImage,
				   BOOL TrueColor,
				   BOOL *fStop,
				   SINT iScale,
				   SINT iDct // JDCT_ISLOW (Default), JDCT_IFAST (Veloce ma meno accurato), JDCT_FLOAT
				   ) // Fattore di scala 1/x 
{
  /* Questa struttura contiene i parametri per la decompressione JPEG ed i puntatori al
   * working space (che sera' allocato come necessita dalla JPEG library).
   */
  IMGHEADER ImgHead;
  BYTE *GBuffer,*CB;
//  int register a;
  struct jpeg_decompress_struct cinfo;
  struct ima_error_mgr jerr;

  /* More stuff */
  FILE * infile;		/* Il File sorgente */
  JSAMPARRAY buffer;	/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  LONG MemoSize;
  /*
     In questo esempio dobbiamo aprire il file di input prima di ogni cosa ,
	 facendo cosi' setjmp() error recovery assume che il file sia aperto.
	 IMPORTANTE: usere l'opzione "b" per aprire il file
    
  */
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);
  *HdlImage=-1;

  // Apertura del file
  if ((infile=fopen(imageFileName,"rb")) == NULL) 
  {	 // win_infoarg("JPGReader:[%s] %d?",imageFileName,GetLastError());
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	  return 0;
  }
  
//  --------------------------------------------------------------------------------------
//  Fase 1: alloco ed inizializzo l'oggetto JPEG per la gestione degli errori
//  Settiamo la normale JPEG error routines, che avverra in uscita con errore.
//  --------------------------------------------------------------------------------------
  //memset(&cinfo,0,sizeof(cinfo));
  cinfo.err = jpeg_std_error(&jerr.pub);
  jpeg_create_decompress(&cinfo);

  cinfo.err = jpeg_std_error(&jerr.pub);
  //
  jerr.pub.error_exit = ima_jpeg_error_exit; // Dichiarata in testa

 /* Initialize JPEG parameters.
   * Much of this may be overridden later.
   * In particular, we don't yet know the input file's color space,
   * but we need to provide some value for jpeg_set_defaults() to work.
   */

  //cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
  //jpeg_set_defaults(&cinfo);
  // Stabilisco il salto di ritorno su errore nel mio contesto.
  // Se avviene un errore JPEG lo segnala
  // Noi dobbiamo pulire JPEG object,chiudere l'input file, e ritornare

  // Possiamo ora inizializzare l'oggetto JPEG per la decompressione
  //jpeg_create_decompress(&cinfo);
  if (setjmp(jerr.setjmp_buffer))  
	{jpeg_destroy_decompress(&cinfo); 
     fclose(infile);
	 if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	 return 0;
	}
  cinfo.dct_method=iDct;
//  cinfo.scale_num=1;
//  cinfo.scale_denom=32;

//  --------------------------------------------------------------------------------------
//  Fase 2 : specifico lo stream del file contenente il JPEG
//  --------------------------------------------------------------------------------------
  jpeg_stdio_src(&cinfo, infile);

//  --------------------------------------------------------------------------------------
//  Fase 3 : Leggo l'header del JPEG
//  --------------------------------------------------------------------------------------
  jpeg_read_header(&cinfo, TRUE);
  if (fStop) {if (*fStop) goto STOP1;}

  cinfo.dct_method=iDct;
  cinfo.scale_num=1;
  cinfo.scale_denom=iScale;

 //  --------------------------------------------------------------------------------------
//  Fase 4 : Setto i parametri per la decompressione
//  --------------------------------------------------------------------------------------

  if (!TrueColor)
  {
   if (cinfo.jpeg_color_space!=JCS_GRAYSCALE) 
   {
	cinfo.quantize_colors = TRUE;
	cinfo.desired_number_of_colors = 256;
   }
  }

  //cinfo.scale_num=1;
  //cinfo.scale_denom=2;

//  --------------------------------------------------------------------------------------
//  Fase 5 : Avvio il decompressore
//  --------------------------------------------------------------------------------------
  jpeg_start_decompress(&cinfo);

  // Potremmo avere bisogno di fare il setup di un nostro "own" in questo punto prima di leggere i dati.
  // Dopo il "jpeg_start_decompress()"  abbiamo la corretta dimensione dell'immagine disponibile.
  // e la colormap di uscita se desideriamo la quantizzazione del colore
  // delle dimensioni
  //Create(cinfo.image_width, cinfo.image_height, 8*cinfo.output_components);
/*
  switch (cinfo.jpeg_color_space)
  {
     case JCS_UNKNOWN: lpsJcs="Sconosciuto"; break;
	 case JCS_GRAYSCALE: lpsJcs="Monocromatico"; break;
	 case JCS_RGB: lpsJcs="RGB"; break;
	 case JCS_YCbCr: lpsJcs="YUV"; break;
	 case JCS_CMYK: lpsJcs="CMYN"; break;
	 case JCS_YCCK: lpsJcs="YCCK"; break;
  }
  win_infoarg("Create [%s] (%d,%d,%d)",lpsJcs,cinfo.image_width, cinfo.image_height,cinfo.output_components);
  */
  // Tira fuori il formato

//  --------------------------------------------------------------------------------------
//  FASE 6: Riempo la struttura IMGHEADER 
//  --------------------------------------------------------------------------------------
  // Specifico i dati dell'header
  //memset(&ImgHead,0,sizeof(ImgHead));
  ZeroFill(ImgHead);
  ImgHead.Type=IMG_JPEG;
  strcpy(ImgHead.FileName,imageFileName);
  ImgHead.lSize=file_len(imageFileName);
  ImgHead.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
  ImgHead.bmiHeader.biWidth=cinfo.output_width;
  ImgHead.bmiHeader.biHeight=cinfo.output_height; // forse * -1
  //dispx("- [%d,%d] - [%d,%d]",ImgHead.bmiHeader.biWidth,ImgHead.bmiHeader.biHeight,cinfo.output_width,cinfo.output_height); pausa(100);
  ImgHead.bmiHeader.biPlanes=1;
  ImgHead.bmiHeader.biBitCount=8*cinfo.output_components;// bit colore
//  ImgHead.bmiHeader.biCompression=BI_RGB;// Non compresso
  ImgHead.bmiHeader.biCompression=BI_RGB;// Non compresso
  ImgHead.bmiHeader.biSizeImage=0;
  ImgHead.bmiHeader.biClrUsed=0;// Colori usati 0=Massimo
  ImgHead.bmiHeader.biClrImportant=0;// 0=Tutti i colori importanti
  
  // JSAMPLEs per riga dell'output buffer
  row_stride = cinfo.output_width * cinfo.output_components;
  ImgHead.linesize=row_stride;

  // Allineo a 32bit
  ImgHead.linesize=((ImgHead.linesize+3)>>2)<<2;

  MemoSize=sizeof(IMGHEADER)+(cinfo.image_height*ImgHead.linesize);
  // C'e' la pallette
  if (cinfo.output_components==1) MemoSize+=sizeof(RGBQUAD)*256;
  
  if (fStop) {if (*fStop) goto STOP2;}

  //win_infoarg("%d",cinfo.output_components);
  *HdlImage=memo_chiedi(RAM_AUTO,MemoSize,file_name(imageFileName));
  if (*HdlImage<0) PRG_end("No memory");

  GBuffer=Wmemo_lockEx(*HdlImage,"JPGReadFile"); 
  if (!GBuffer) PRG_end("GBuffer = NULL hdl=%d",HdlImage);
  ImgHead.Offset=sizeof(ImgHead);
  // Carico la palettes
  if (ImgHead.bmiHeader.biBitCount==8)
  {
    if (cinfo.jpeg_color_space==JCS_GRAYSCALE) 
      CreateGrayColourMap(GBuffer+sizeof(ImgHead),256);
      else
	  SetPalette(GBuffer+sizeof(ImgHead),cinfo.actual_number_of_colors, cinfo.colormap[0], cinfo.colormap[1], cinfo.colormap[2]);
	  
	ImgHead.Offset+=(256*sizeof(RGBQUAD));
  }

  memcpy(GBuffer,&ImgHead,sizeof(ImgHead));
  GBuffer+=ImgHead.Offset;

  // Costruisco una riga/array di appoggio che che verra liberata quando l'immagine verra' fatta
  buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

//  --------------------------------------------------------------------------------------
//  Fase 7  : Loop sulle linee che rimangono da leggere
//  Usiamo per il loop una variabile di stato delle librerie cinfo.output_scanline
//  come contatore, cosi fancendo non .....
//  --------------------------------------------------------------------------------------

  CB=GBuffer;
  while (cinfo.output_scanline < cinfo.output_height) {
	 
	 jpeg_read_scanlines(&cinfo, buffer, 1);
     //if (cinfo.output_scanline <10) win_infoarg("%d",cinfo.output_scanline );
	 // Memorizzo le linee in un buffer
	 //memo_scrivivar(*HdlImage,PtDest,buffer,row_stride);
	 //memset(GBuffer,120,row_stride);

	 memcpy(GBuffer,buffer[0],ImgHead.linesize);
	 if (fStop) {if (*fStop) break;}

	 //PtDest+=row_stride;
     // memset(GBuffer,0,3*10);
	 GBuffer+=ImgHead.linesize;
  }

//  --------------------------------------------------------------------------------------
//  Fase 8
//  Fine della decompressione rilascio le memorie usate
//  --------------------------------------------------------------------------------------

STOP2:
  if (fStop) 
	{if (*fStop) jpeg_abort_decompress(&cinfo);}
	else
    jpeg_finish_decompress(&cinfo);

  // Ignoriamo un valore di fine sospessione possibile dello standard IO data source.

  /* Step 8: Release JPEG decompression object */
  /* This is an important step since it will release a good deal of memory. */

//  --------------------------------------------------------------------------------------
//  Fase 9
//  Rilascio il JPEG decompression Object
//  --------------------------------------------------------------------------------------

STOP1:

   jpeg_destroy_decompress(&cinfo);
   fclose(infile);

   // Richiesto stop
   if (fStop) 
   {
	 if (*fStop)
	 {
	   if (*HdlImage>0)
	   {
		 Wmemo_unlockEx(*HdlImage,"JPGReadFile");	
		 memo_libera(*HdlImage,"Stop");
		 *HdlImage=-1;
	   }
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
      return FALSE;
	 }
   }
   
//  --------------------------------------------------------------------------------------
//  Fase 10
//  Inverto i colori per quello stronzo di "windows" che li vuole in formato BGR
//  --------------------------------------------------------------------------------------

  if (cinfo.output_components==3)
  {
   SINT kx,ky;
   BYTE *ap;
   for (ky=0;ky<(SINT) cinfo.image_height;ky++)
   {
	   ap=CB;
	   for (kx=0;kx<(ImgHead.linesize-2);kx+=3)
		{
		 BYTE k;
		 k=*ap; *ap=*(ap+2); *(ap+2)=k;
		 ap+=3;
		}
       CB+=ImgHead.linesize;
   }
  }

  Wmemo_unlockEx(*HdlImage,"JPGReadFile2");

//  --------------------------------------------------------------------------------------
//  E siamo fatti ....!!!
  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
  return 1;
}

void IMGGetSize(SINT hImage,SIZE *lps)
{
  IMGHEADER *Img;
  BITMAPINFOHEADER *BmpHeader;

  Img=Wmemo_lockEx(hImage,"IMGGetSize"); if (Img==NULL) PRG_end("IMGGetSize");
  BmpHeader=(BITMAPINFOHEADER *)  &Img->bmiHeader;
  lps->cy=BmpHeader->biHeight;
  lps->cx=BmpHeader->biWidth;
  Wmemo_unlock(hImage);
}
/*
static void BmpMaskDispRedim(SINT x,SINT y,
							 SINT iLxOriginal,SINT iLyOriginal,
							 SINT iLxReal,SINT iLyReal,
							 HBITMAP BitMap,
							 HBITMAP MaskBit,
							 HDC hDC)
{
	// Creo un area di memoria compatibile al DC
	// Seleziono l'oggetto BITMAP nella memoria nuova

	// -----------------------------------------------------
	// Trasferisco il bitmap dalla Copia --> Al DCwindows  !
	// Versione senza maschera                             !
	// -----------------------------------------------------

	if (MaskBit==NULL)
	 {
		HDC hdcMemory;
		hdcMemory = CreateCompatibleDC(hDC);
		if (hdcMemory==NULL) PRG_end("BMD:4");

		SelectObject(hdcMemory, BitMap);
		*/
		/*
		BitBlt(hDC,  // --- DESTINAZIONE ---
			   x, y, // Coordinate X-Y
			   lx, // Larghezza
			   ly, //Altezza
			   hdcMemory, // --- SORGENTE ---
			   0, 0, //Cordinate x-y
			   SRCCOPY);
			   */
		/*
    if (StretchDIBits(hDC, 
					  x,y
					  TileX,ReadSectY,
					  0,0,
					  TileX,ReadSectY,
					  Sorg,
					  BmpInfo,
					  DIB_RGB_COLORS, 
					  SRCCOPY) == GDI_ERROR) 
*/
/*
		DeleteDC(hdcMemory);
	 }
	 else
	// -----------------------------------------------------
	// Trasferisco il bitmap dalla Copia --> Al DCwindows  !
	// Versione con la MASKERA                             !
	// -----------------------------------------------------
	 {
		HDC hdcMemory;
		HDC hdcMem2;
		RECT Dim_xy;	

		// Creo i due DC
		hdcMemory = CreateCompatibleDC(hDC); if (hdcMemory==NULL) PRG_end("BMD:1");
		hdcMem2 = CreateCompatibleDC(hDC); if (hdcMem2==NULL) PRG_end("BMD:2");
		SelectObject(hdcMem2, MaskBit);
		SetTextColor(hDC,ColorPal[15]);
		SetBkColor(hDC,ColorPal[0]);

	    // ----------------------
	    // Rovescio la maskera  !
	    // ----------------------
		Dim_xy.left=0; Dim_xy.top=0;
		Dim_xy.right=lx; Dim_xy.bottom=ly;

		// ----------------------
		// Buco con la maskera  !
		// ----------------------
		BitBlt(hDC,  // --- DESTINAZIONE ---
			   x, y, // Coordinate X-Y
			   lx, // Larghezza
			   ly, //Altezza
			   hdcMem2, // --- SORGENTE ---
			   0, 0, //Cordinate x-y
			   SRCAND); //Questo ma invertito

		// RiRovescio la Maskera
		//InvertRect(hdcMem2, &Dim_xy);
	    // -----------------------------------
	    // Buco l'icone Pulisco l'icona      !
	    // -----------------------------------
	    SelectObject(hdcMem2, BitMap);
	    SelectObject(hdcMemory, MaskBit);

	    BitBlt(hdcMem2,  // --- DESTINAZIONE ---
			   0, 0, // Coordinate X-Y
			   lx, // Larghezza
			   ly, //Altezza
			   hdcMemory, // --- SORGENTE ---
			   0, 0, //Cordinate x-y
			   SRCAND);
		// ----------------------
		// Stampo l'icona       !
		// ----------------------

		BitBlt(hDC,  // --- DESTINAZIONE ---
			   x, y, // Coordinate X-Y
			   lx, // Larghezza
			   ly, //Altezza
			   hdcMem2, // --- SORGENTE ---
			   0, 0, //Cordinate x-y
			   SRCPAINT);

		DeleteDC(hdcMem2);
	    DeleteDC(hdcMemory);
	 }
}
*/

SINT ico_dispEx(SINT x1,SINT y1,SINT iLx,SINT iLy,CHAR *lpIcone)
{
	struct  ICONE *ptico;
	struct  ICO_HEAD *head;
	WORD    pt,a=1;
	SINT    hdl;
	CHAR    *sorg,*mask;
	HBITMAP BitMap,MaskBit;
	BOOL	fLock=FALSE;
	BYTE *  IconeBmpPtr=NULL;

	BITMAPINFOHEADER *BmpHeader;
	BITMAPINFO *BmpInfo;
	HDC    hDC;
	LONG dx,dy;

    if (lpIcone==NULL) return -1;
	strupr(lpIcone); if (ico_cerca(&pt,lpIcone)) return -1;
	ptico=sys.icone+pt;
	hdl=ptico->hdl;

	// Controlla il tipo di memoria
	if ((sys.memolist[hdl].iTipo==RAM_HEAP)||(sys.memolist[hdl].iTipo==RAM_GLOBALHEAP))
		 {sorg=memo_heap(hdl); }
			else
		 {sorg=GlobalLock(sys.memolist[hdl].hGlobal);
		  if (sorg==NULL) PRG_end("Errore memo in ICODISP");
		  fLock=TRUE;
		 }
    
	sorg+=ptico->offset;

	// -------------------------------------
	// DATI DELL'ICONE                     !
	// -------------------------------------
	head=(struct ICO_HEAD *) sorg;
	mask=sorg+head->ofs_mask;
	sorg+=head->ofs_icone;
/*
	IconeBmpHdl=memo_chiedi(RAM_HEAP,sizeof(BITMAPINFOHEADER)+(256*sizeof(RGBQUAD)),"BMP->ICONE");
	if (IconeBmpHdl<0) win_errgrave("ICONE");
	IconeBmpPtr=memo_heap(IconeBmpHdl);
	*/
	IconeBmpPtr=GlobalAlloc(GPTR,sizeof(BITMAPINFOHEADER)+(256*sizeof(RGBQUAD)));

	BmpInfo=(BITMAPINFO *) IconeBmpPtr;
	BmpHeader=(BITMAPINFOHEADER *) IconeBmpPtr;

	// -------------------------------------
	// CARICA BMPHEADER                    !
	// -------------------------------------
	BmpHeader->biSize=sizeof(BITMAPINFOHEADER); // la larghezza della struttura
	BmpHeader->biWidth=head->dimx;
	BmpHeader->biHeight=-head->dimy; // forse * -1
	BmpHeader->biPlanes=1;
	BmpHeader->biBitCount=head->bit;// bit colore
	BmpHeader->biCompression=BI_RGB;// Non compresso
	BmpHeader->biClrUsed=0;// Colori usati 0=Massimo
	BmpHeader->biClrImportant=0;// 0=Tutti i colori importanti

	hDC=UsaDC(APRI,0);

	// -------------------------------------
	// CREA IL BITMAP                      !
	// -------------------------------------
	BitMap=CreateDIBitmap(hDC, //Handle del contesto
						  BmpHeader,
						  CBM_INIT,
						  sorg,// Dati del'icone
						  BmpInfo,
						  DIB_RGB_COLORS);
	if (BitMap==NULL) PRG_end("ico_disp:NULL1");
	
	if (head->ofs_mask!=-1)
		 {MaskBit=CreateBitmap(head->dimx,head->dimy, 1, 1,mask);
	      if (MaskBit==NULL) PRG_end("ico_disp:NULL2");
		  //Flag=TRUE;
		 }
		 else
		 {//Flag=FALSE;
		  MaskBit=NULL;
		 }

//	BmpMaskDisp(x1,y1,head->dimx,head->dimy,BitMap,MaskBit,hDC);
	/*
	if (iLx!=head->dimx||iLy!=head->dimy) 
		BmpMaskDispRedim(x1,y1,head->dimx,head->dimy,iLx,iLy,BitMap,MaskBit,hDC);
		else
		BmpMaskDisp(x1,y1,iLx,iLy,BitMap,MaskBit,hDC);
		*/
	DeleteObject(BitMap);
	if (MaskBit!=NULL) {DeleteObject(MaskBit);}
	if (fLock) GlobalUnlock(sys.memolist[hdl].hGlobal);
	UsaDC(CHIUDI,hDC);
	GlobalFree(IconeBmpPtr);

	return a;
}


