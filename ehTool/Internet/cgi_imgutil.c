//   ีอออออออออออออออออออออออออออออออออออออออออออธ
//   ณ CGI_IMGutil  Image Utility                ณ
//   ณ          Lettura e scrittura delle        ณ
//   ณ          immagini                         ณ
//   ณ                                           ณ
//   ณ             8 Maggio                      ณ
//   ณ             by Ferr… Art & Tecnology 1999 ณ
//   ิอออออออออออออออออออออออออออออออออออออออออออพ

#include "\ehtool\include\ehsw_i.h"
#include <setjmp.h>
#include "\ehtool\imgutil.h"

//  --------------------------------------------------------------------------------------
//  | FORMATO JPEG                              
//  --------------------------------------------------------------------------------------
/*
struct ima_error_mgr {
  struct jpeg_error_mgr pub;	
  jmp_buf setjmp_buffer;	
};
*/
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
  //win_infoarg("Ferra Image Manager:\n%s",buffer);
  SetFocus(hWnd);
  /* Send it to stderr, adding a newline */
  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
  //return;
}

static void CreateGrayColourMap(BYTE *Palette,int nGrays)
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

static void SetPalette(BYTE *Palette,SINT nColors, BYTE *Red, BYTE *Green, BYTE *Blue)
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

// ---------------------------------------------------
// JPGReadFile
// LEGGE UN FILE IN FORMATO JPEG
//
// imageFileName Nome del file da leggere
// *HdlImage	 Puntatore all'handle che conterrเ l'immagine
// TrueColor     TRUE=True Color/FALSE=256 colori
// *fStop        Puntatore ad un BOOL che determina lo stop del processo 
//               serve per applicazione Multi/Thread
//               NULL se non si usa
// Ritorna: 0 Errore
//          1 Tutto OK

BOOL JPGReadFile(CHAR *imageFileName,SINT *HdlImage,BOOL TrueColor,BOOL *fStop,SINT *iErr,BOOL h)
{
  /* Questa struttura contiene i parametri per la decompressione JPEG ed i puntatori al
   * working space (che sera' allocato come necessita dalla JPEG library).
   */
  IMGHEADER ImgHead;
  BYTE *GBuffer,*CB;
//  int register a;
  struct jpeg_decompress_struct cinfo;
//  CHAR *lpsJcs;

  /* Usiamo una nostra gestione privata di JPEG error handler. */
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

  // Apertura del file
  if ((infile=fopen(imageFileName,"rb")) == NULL) 
  {	 // win_infoarg("JPGReader:[%s] %d?",imageFileName,GetLastError());
	  return 0;
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

  if (setjmp(jerr.setjmp_buffer))  {jpeg_destroy_decompress(&cinfo); fclose(infile);return 0;}
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
 // memset(&ImgHead,0,sizeof(ImgHead));
  ZeroFill(ImgHead);
  ImgHead.Type=IMG_JPEG;
  strcpy(ImgHead.FileName,imageFileName);
  ImgHead.lFileSize=file_len(imageFileName);
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

  GBuffer=Wmemo_lock(*HdlImage); 
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
		 Wmemo_unlock(*HdlImage);	
		 memo_libera(*HdlImage,"Stop");
		 *HdlImage=-1;
	   }
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
  
  Wmemo_unlock(*HdlImage);

//  --------------------------------------------------------------------------------------
//  E siamo fatti ....!!!
  return 1;
}



// ---------------------------------------------------
// JPGSaveFile
// SCRIVE UN FILE IN FORMATO JPEG
//
// imageFileName Nome del file da scrivere
// HdlImage	     Hdl che contiene l'immagine in formato IMGHEADER
// iQuality      Fattore di qualitเ dell'immagine
BOOL JPGSaveFile(CHAR *imageFileName,SINT HdlImage,SINT iQuality)
{
  FILE * outfile;		/* Il File sorgente */
  BOOL fRet;
  // Apertura del file
  if ((outfile=fopen(imageFileName,"wb")) == NULL) 
  {	 
	  return 0;
  }

  fRet=JPGPutStream(outfile,HdlImage,iQuality);
  fclose(outfile);
  return fRet;
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

  if (setjmp(jerr.setjmp_buffer))  {jpeg_destroy_compress(&cinfo); Wmemo_unlock(HdlImage); return 0;}
  
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

  // Setto un parametro non di "default"; la qualitเ
  // Here we just illustrate the use of quality (quantization table) scaling:
  jpeg_set_quality(&cinfo, iQuality, TRUE /* limit to baseline-JPEG values */);

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
    // L'array qui ่ di un solo elemento, ma puoi passare pi๙ di un elemento alla volta
    // se pensi sia pi๙ conveniente
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
  
  Wmemo_unlock(HdlImage);

//  --------------------------------------------------------------------------------------
//  E siamo fatti ....!!!
  return 1;
}



BOOL JPGReadHeader(CHAR *imageFileName,IMGHEADER *ImgHead,BOOL h)
{
  struct jpeg_decompress_struct cinfo;

  struct ima_error_mgr jerr;

  FILE * infile;		/* Il File sorgente */
  int row_stride;		/* physical row width in output buffer */

  if (!imageFileName) return FALSE;
  if (!*imageFileName) return FALSE;
  
  if ((infile=fopen(imageFileName,"rb")) == NULL) 
  {	
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
  // Specifico i dati dell'header
  memset(ImgHead,0,sizeof(IMGHEADER));
  ImgHead->Type=IMG_JPEG;
  strcpy(ImgHead->FileName,imageFileName);
  ImgHead->lFileSize=file_len(imageFileName);
  ImgHead->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
  ImgHead->bmiHeader.biWidth=cinfo.image_width;
  ImgHead->bmiHeader.biHeight=cinfo.image_height; // forse * -1
  ImgHead->bmiHeader.biPlanes=1;
  ImgHead->bmiHeader.biBitCount=8*cinfo.output_components;// bit colore
  ImgHead->bmiHeader.biCompression=BI_RGB;// Non compresso
  ImgHead->bmiHeader.biSizeImage=0;
  ImgHead->bmiHeader.biClrUsed=0;// Colori usati 0=Massimo
  ImgHead->bmiHeader.biClrImportant=0;// 0=Tutti i colori importanti
  row_stride = cinfo.output_width * cinfo.output_components;
  ImgHead->linesize=row_stride;
  ImgHead->linesize=((ImgHead->linesize+3)>>2)<<2;

  jpeg_destroy_decompress(&cinfo);
  fclose(infile);
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
 Img=Wmemo_lock(Hdl);
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
 
 Wmemo_unlock(Hdl);

 return;
}

// ------------------------------------------------------------------
// IMGRemaker
// Costruisce un immagine partendo da un'altra in altre dimensioni
// Per ora solo true color
// ------------------------------------------------------------------
static SINT LocalIMGRemaker(SINT HdlImage,SINT xNew,SINT yNew);
static SINT LocalIMGRemakerAA(SINT HdlImage,SINT xNew,SINT yNew,SINT iResampling);

SINT IMGRemaker(SINT HdlImage,SINT xNew,SINT yNew,SINT fPhotoQuality,SINT iResampling)
{
  if (fPhotoQuality) return LocalIMGRemakerAA(HdlImage,xNew,yNew,iResampling);
  return LocalIMGRemaker(HdlImage,xNew,yNew);
}

static SINT LocalIMGRemaker(SINT HdlImage,SINT xNew,SINT yNew)
{
	BYTE *lpi;
	IMGHEADER *ImgSorg,*ImgDest;
	SINT iNewSize;
	SIZE sImage;
	SINT iLineSize;
	SINT HdlNew;
	BYTE *lpSorg,*lpDest;
	BITMAPINFOHEADER *BHSorg;
	BITMAPINFOHEADER *BHDest;
	POINT pSorg;
	SINT yPf,iPf;
	SINT xc;
	SINT x,y;
   	
	lpi=Wmemo_lock(HdlImage);
	ImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+ImgSorg->Offset;
		
	BHSorg=(BITMAPINFOHEADER *) &ImgSorg->bmiHeader;
	sImage.cy=BHSorg->biHeight;
    sImage.cx=BHSorg->biWidth;
	
	if (ImgSorg->bmiHeader.biBitCount!=24) 
	{
		//iLineSize=sImage.cx*3; iLineSize=((iLineSize+3)>>2)<<2;
		//iNewSize=(iLineSize*sImage.cy)+ImgSorg->Offset;
		HdlNew=memo_chiedi(RAM_AUTO,sys.memolist[HdlImage].dwSize,"NewImage");
		//memo_copy(
		memo_copyall(HdlImage,HdlNew);
		//Wmemo_unlock(HdlNew);
		Wmemo_unlock(HdlImage);
		return HdlNew;

		//PRG_end("IMGRemaker: no TRUECOLOR %d",ImgSorg->bmiHeader.biBitCount);
	}

	// Ricalcolo l'occupazione di spazio

	// Quantizzazione a 32 bit
	iLineSize=xNew*3; iLineSize=((iLineSize+3)>>2)<<2;
	iNewSize=(iLineSize*yNew)+ImgSorg->Offset;

	// Alloco la memoria
	HdlNew=memo_chiedi(RAM_AUTO,iNewSize,"NewImage");
	if (HdlNew<0) PRG_end("IMGRemaker: non memory");
	lpDest=Wmemo_lock(HdlNew);
	memset(lpDest,0,iNewSize);

	// Copio l'header
	memcpy(lpDest,lpi,ImgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	ImgDest=(IMGHEADER *) lpDest;
	ImgDest->linesize=iLineSize;
	BHDest=(BITMAPINFOHEADER *) &ImgDest->bmiHeader;
	BHDest->biHeight=yNew;
	BHDest->biWidth=xNew;
	lpDest+=ImgDest->Offset;

	// Effettuo lo streching
	for (y=0;y<yNew;y++)
	{
		pSorg.y=y*sImage.cy/yNew;
		yPf=(pSorg.y*ImgSorg->linesize);
		xc=0;
		for (x=0;x<xNew;x++)
		{
			// Calcolo la posizione del punto X nel sorgente
			// xSorg? : xSorgSize = yDest : yDestSize;
			pSorg.x=x*sImage.cx/xNew;
			// Calcolo la posizione fisica
			iPf=yPf+(pSorg.x*3);
			// Copia il pixel
			memcpy(lpDest+xc,lpSorg+iPf,3);
			xc+=3;
		
		}
		lpDest+=ImgDest->linesize;
	}

	Wmemo_unlock(HdlNew);
	Wmemo_unlock(HdlImage);
	return HdlNew;
}


// ------------------------------------------------------------------
// IMGRemakerAA
// Costruisce un immagine partendo da un'altra in altre dimensioni
// Effettua antialiasing sulla compressione
// Per ora solo true color
// ------------------------------------------------------------------

static SINT LocalIMGRemakerAA(SINT HdlImage,SINT xNew,SINT yNew,SINT iResampling)
{
	BYTE *lpi;
	IMGHEADER *ImgSorg,*ImgDest;
	SINT iNewSize;
	SIZE sImage;
	SINT iLineSize;
	SINT HdlNew;
	BYTE *lpSorg,*lpDest;
	BITMAPINFOHEADER *BHSorg;
	BITMAPINFOHEADER *BHDest;
	POINT pSorg;
	POINT pSorg2;
	SINT yPf;
	SINT xc;
	SINT x,y;
	SINT rgb[3];
	BYTE *lpj;
	SINT yk,xk;
	SINT iCount;
	SINT a;
   	
	lpi=Wmemo_lock(HdlImage);
	ImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+ImgSorg->Offset;
		
	BHSorg=(BITMAPINFOHEADER *) &ImgSorg->bmiHeader;
	sImage.cy=BHSorg->biHeight;
    sImage.cx=BHSorg->biWidth;

 	if (ImgSorg->bmiHeader.biBitCount!=24) 
	{
		HdlNew=memo_chiedi(RAM_AUTO,sys.memolist[HdlImage].dwSize,"NewImage");
		memo_copyall(HdlImage,HdlNew);
		Wmemo_unlock(HdlImage);
		return HdlNew;
	}

	// Se le dimensioni sono uguali o maggiori usa l'altro metodo
	if ((xNew>=sImage.cx)&&(yNew>=sImage.cy))
	{
	 	Wmemo_unlock(HdlImage);
		//return LocalIMGRemaker(HdlImage,xNew,yNew);

		HdlNew=IMGResampling(HdlImage,  // Handle dell'immagine
					  NULL, // NULL=Tutta l'immagine rettangolo dell'area interessata
					  xNew, 
					  yNew, 
				      iResampling);
		return HdlNew;
	}

  /*
	if ((xNew>=sImage.cx)&&(yNew>=sImage.cy))
	{
	 	Wmemo_unlock(HdlImage);
		return LocalIMGRemaker(HdlImage,xNew,yNew);
	
	}
*/	
	// Ricalcolo l'occupazione di spazio

	// Quantizzazione a 32 bit
	iLineSize=xNew*3; iLineSize=((iLineSize+3)>>2)<<2;
	iNewSize=(iLineSize*yNew)+ImgSorg->Offset;

	// Alloco la memoria
	HdlNew=memo_chiedi(RAM_AUTO,iNewSize,"NewImage");
	if (HdlNew<0) PRG_end("IMGRemaker: non memory");
	lpDest=Wmemo_lock(HdlNew);
	memset(lpDest,0,iNewSize);

	// Copio l'header
	memcpy(lpDest,lpi,ImgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	ImgDest=(IMGHEADER *) lpDest;
	ImgDest->linesize=iLineSize;
	BHDest=(BITMAPINFOHEADER *) &ImgDest->bmiHeader;
	BHDest->biHeight=yNew;
	BHDest->biWidth=xNew;
	lpDest+=ImgDest->Offset;

	// -----------------------------------------------
	// Effettuo lo streching con antialiasing
	//
	
	for (y=0;y<yNew;y++)
	{
		pSorg.y=y*sImage.cy/yNew;
		pSorg2.y=(y+1)*sImage.cy/yNew;
		if (pSorg.y>sImage.cy) pSorg.y=sImage.cy;
//		yPf=(pSorg.y*ImgSorg->linesize); // Posizione fisica di inizio linea
		xc=0;
		for (x=0;x<xNew;x++)
		{
			// Calcolo la posizione del punto X nel sorgente
			// xSorg? : xSorgSize = yDest : yDestSize;
			pSorg.x=x*sImage.cx/xNew;
			pSorg2.x=(x+1)*sImage.cx/xNew;
			if (pSorg.x>sImage.cx) pSorg.x=sImage.cx;
		    yPf=(pSorg.y*ImgSorg->linesize); // Posizione fisica di inizio linea

			// A) Somma di tutti i colori del blocco
			ZeroFill(rgb);
			iCount=0;
			for (yk=pSorg.y;yk<pSorg2.y;yk++)
			{
				lpj=lpSorg+(yPf+(pSorg.x*3));
				for (xk=pSorg.x;xk<pSorg2.x;xk++)
				{
				  rgb[0]+=*lpj; lpj++;
				  rgb[1]+=*lpj; lpj++;
				  rgb[2]+=*lpj; lpj++;
				  iCount++;
				}
				yPf+=ImgSorg->linesize; // Incremento di una linea
			}
			// B) Calcolo della media
			if (iCount<0) iCount=0;
			for (a=0;a<3;a++,xc++) {*(lpDest+xc)=(BYTE) (rgb[a]/iCount);}
			// Copia il pixel
			//memcpy(lpDest+xc,lpSorg+iPf,3);
			//xc+=3;
		}
		lpDest+=ImgDest->linesize;
	}

	Wmemo_unlock(HdlNew);
	Wmemo_unlock(HdlImage);
	return HdlNew;
}

// ----------------------------------------------------------------------------
// JPGNewFile
// Crea un nuovo file di nuove dimensioni e qualitเ differenti 
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
				SINT iResampling)
{
	SINT HdlImage;
	SINT HdlImageNew;
	SINT err;
    IMGHEADER ImgHead;
	
	// Leggo l'header
	if (!JPGReadHeader(lpFileSource,&ImgHead,FALSE)) return -1;

	// Calcolo automatico delle dimensioni orizzontali
    //  SizeY:Ly=x:Lx;
    if ((iLy>0)&&(iLx==0)) iLx=iLy*ImgHead.bmiHeader.biWidth/ImgHead.bmiHeader.biHeight;
    // Calcolo automatico delle dimensioni verticali
    if ((iLx>0)&&(iLy==0)) iLy=iLx*ImgHead.bmiHeader.biHeight/ImgHead.bmiHeader.biWidth;

	if (!JPGReadFile(lpFileSource,&HdlImage,TRUE,NULL,NULL,FALSE)) return -1;
	HdlImageNew=IMGRemaker(HdlImage,iLx,iLy,fAntiAlias,iResampling);
				    
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


