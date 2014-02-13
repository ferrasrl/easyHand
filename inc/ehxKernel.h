//
//  ehKernel.h
//  easyhand
//
//  Created by Giorgio Tassistro on 08/08/11.
//  Copyright 2011 Ferrà srl. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#ifndef EH_XKERNEL
#define EH_XKERNEL

@interface UIColor(Extras)
+ (UIColor *)colorWithRGBA:(NSUInteger) rgba;
+ (UIColor *)colorWithRGB:(NSUInteger) rgb;
@end

@interface UITabBarController(Extras) 
-(void)showTab:(BOOL) bShow;
@end

BYTE *      uiGetUrl(CHAR *pszUrl,SIZE_T * piLength);
CGSize      ehGetSizeScreen(void);
EH_TIME *   timeNSDateToEht(EH_TIME *psEht,NSDate * poDate);
NSDate *    timeEhtToNSDate(NSDate * poDate,EH_TIME *psEht);
UTF8 *      strLocal(UTF8 *pszString);

//
// ehRoundRect
//
@interface ehRoundRect : UIView {
    UIColor     *strokeColor;
    UIColor     *rectColor;
    CGFloat     strokeWidth;
    CGFloat     cornerRadius;
}
@property (nonatomic, retain) UIColor *strokeColor;
@property (nonatomic, retain) UIColor *rectColor;
@property CGFloat strokeWidth;
@property CGFloat cornerRadius;

- (id)initWithFrame:(CGRect)frame cornerRadius:(CGFloat)fRadius color:(UIColor *)colBack;
@end


//
// ehRoundLabel --------------------------------------------------
//
@interface ehRoundLabel : ehRoundRect {
    UILabel     * uiLabel;
}
@property (nonatomic, assign) UILabel * uiLabel;
-(id)initWithText:(CHAR *)pszText cornerRadius:(CGFloat)fRadius color:(UIColor *)colBack;
-(void)setOrigin:(CGPoint)sPoint;
@end


//
//  MBProgressHUD.h
//  Version 0.4
//  Created by Matej Bukovinski on 2.4.09.
//

// This code is distributed under the terms and conditions of the MIT license. 

// Copyright (c) 2011 Matej Bukovinski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//∫
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.



@protocol MBProgressHUDDelegate;

/////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {
    /** Progress is shown using an UIActivityIndicatorView. This is the default. */
    MBProgressHUDModeIndeterminate,
    /** Progress is shown using a MBRoundProgressView. */
	MBProgressHUDModeDeterminate,
	/** Shows a custom view */
	MBProgressHUDModeCustomView
} MBProgressHUDMode;

typedef enum {
    /** Opacity animation */
    MBProgressHUDAnimationFade,
    /** Opacity + scale animation */
    MBProgressHUDAnimationZoom
} MBProgressHUDAnimation;

/////////////////////////////////////////////////////////////////////////////////////////////

/** 
 * Displays a simple HUD window containing a progress indicator and two optional labels for short messages.
 *
 * This is a simple drop-in class for displaying a progress HUD view similar to Apples private UIProgressHUD class.
 * The MBProgressHUD window spans over the entire space given to it by the initWithFrame constructor and catches all
 * user input on this region, thereby preventing the user operations on components below the view. The HUD itself is
 * drawn centered as a rounded semi-transparent view witch resizes depending on the user specified content.
 *
 * This view supports three modes of operation:
 * - MBProgressHUDModeIndeterminate - shows a UIActivityIndicatorView
 * - MBProgressHUDModeDeterminate - shows a custom round progress indicator (MBRoundProgressView)
 * - MBProgressHUDModeCustomView - shows an arbitrary, user specified view (@see customView)
 *
 * All three modes can have optional labels assigned:
 * - If the labelText property is set and non-empty then a label containing the provided content is placed below the
 *   indicator view.
 * - If also the detailsLabelText property is set then another label is placed below the first label.
 */
@interface MBProgressHUD : UIView {
	
	MBProgressHUDMode mode;
    MBProgressHUDAnimation animationType;
	
	SEL methodForExecution;
	id targetForExecution;
	id objectForExecution;
	BOOL useAnimation;
	
    float yOffset;
    float xOffset;
	
	float width;
	float height;
	
	float margin;
	
	BOOL dimBackground;
	
	BOOL taskInProgress;
	float graceTime;
	float minShowTime;
	NSTimer *graceTimer;
	NSTimer *minShowTimer;
	NSDate *showStarted;
	
	UIView *indicator;
	UILabel *label;
	UILabel *detailsLabel;
	
	float progress;
	
	id<MBProgressHUDDelegate> delegate;
	NSString *labelText;
	NSString *detailsLabelText;
	float opacity;
	UIFont *labelFont;
	UIFont *detailsLabelFont;
	
    BOOL isFinished;
	BOOL removeFromSuperViewOnHide;
	
	UIView *customView;
	
	CGAffineTransform rotationTransform;
}

/**
 * Creates a new HUD, adds it to provided view and shows it. The counterpart to this method is hideHUDForView:animated:.
 * 
 * @param view The view that the HUD will be added to
 * @param animated If set to YES the HUD will disappear using the current animationType. If set to NO the HUD will not use
 * animations while disappearing.
 * @return A reference to the created HUD.
 *
 * @see hideHUDForView:animated:
 */
+ (MBProgressHUD *)showHUDAddedTo:(UIView *)view animated:(BOOL)animated;

/**
 * Finds a HUD sibview and hides it. The counterpart to this method is showHUDAddedTo:animated:.
 *
 * @param view The view that is going to be searched for a HUD subview.
 * @param animated If set to YES the HUD will disappear using the current animationType. If set to NO the HUD will not use
 * animations while disappearing.
 * @return YES if a HUD was found and removed, NO otherwise. 
 *
 * @see hideHUDForView:animated:
 */
+ (BOOL)hideHUDForView:(UIView *)view animated:(BOOL)animated;

/** 
 * A convenience constructor that initializes the HUD with the window's bounds. Calls the designated constructor with
 * window.bounds as the parameter.
 *
 * @param window The window instance that will provide the bounds for the HUD. Should probably be the same instance as
 * the HUD's superview (i.e., the window that the HUD will be added to).
 */
- (id)initWithWindow:(UIWindow *)window;

/**
 * A convenience constructor that initializes the HUD with the view's bounds. Calls the designated constructor with
 * view.bounds as the parameter
 * 
 * @param view The view instance that will provide the bounds for the HUD. Should probably be the same instance as
 * the HUD's superview (i.e., the view that the HUD will be added to).
 */
- (id)initWithView:(UIView *)view;

/**
 * The UIView (i.g., a UIIMageView) to be shown when the HUD is in MBProgressHUDModeCustomView.
 * For best results use a 37 by 37 pixel view (so the bounds match the build in indicator bounds). 
 */
@property (retain) UIView *customView;

/** 
 * MBProgressHUD operation mode. Switches between indeterminate (MBProgressHUDModeIndeterminate) and determinate
 * progress (MBProgressHUDModeDeterminate). The default is MBProgressHUDModeIndeterminate.
 *
 * @see MBProgressHUDMode
 */
@property (assign) MBProgressHUDMode mode;

/**
 * The animation type that should be used when the HUD is shown and hidden. 
 *
 * @see MBProgressHUDAnimation
 */
@property (assign) MBProgressHUDAnimation animationType;

/** 
 * The HUD delegate object. If set the delegate will receive hudWasHidden callbacks when the HUD was hidden. The
 * delegate should conform to the MBProgressHUDDelegate protocol and implement the hudWasHidden method. The delegate
 * object will not be retained.
 */
@property (assign) id<MBProgressHUDDelegate> delegate;

/** 
 * An optional short message to be displayed below the activity indicator. The HUD is automatically resized to fit
 * the entire text. If the text is too long it will get clipped by displaying "..." at the end. If left unchanged or
 * set to @"", then no message is displayed.
 */
@property (copy) NSString *labelText;

/** 
 * An optional details message displayed below the labelText message. This message is displayed only if the labelText
 * property is also set and is different from an empty string (@"").
 */
@property (copy) NSString *detailsLabelText;

/** 
 * The opacity of the HUD window. Defaults to 0.9 (90% opacity). 
 */
@property (assign) float opacity;

/** 
 * The x-axis offset of the HUD relative to the centre of the superview. 
 */
@property (assign) float xOffset;

/** 
 * The y-ayis offset of the HUD relative to the centre of the superview. 
 */
@property (assign) float yOffset;

/**
 * The amounth of space between the HUD edge and the HUD elements (labels, indicators or custom views).
 *
 * Defaults to 20.0
 */
@property (assign) float margin;

/** 
 * Cover the HUD background view with a radial gradient. 
 */
@property (assign) BOOL dimBackground;

/*
 * Grace period is the time (in seconds) that the invoked method may be run without 
 * showing the HUD. If the task finishes befor the grace time runs out, the HUD will
 * not be shown at all. 
 * This may be used to prevent HUD display for very short tasks.
 * Defaults to 0 (no grace time).
 * Grace time functionality is only supported when the task status is known!
 * @see taskInProgress
 */
@property (assign) float graceTime;


/**
 * The minimum time (in seconds) that the HUD is shown. 
 * This avoids the problem of the HUD being shown and than instantly hidden.
 * Defaults to 0 (no minimum show time).
 */
@property (assign) float minShowTime;

/**
 * Indicates that the executed operation is in progress. Needed for correct graceTime operation.
 * If you don't set a graceTime (different than 0.0) this does nothing.
 * This property is automatically set when using showWhileExecuting:onTarget:withObject:animated:.
 * When threading is done outside of the HUD (i.e., when the show: and hide: methods are used directly),
 * you need to set this property when your task starts and completes in order to have normal graceTime 
 * functunality.
 */
@property (assign) BOOL taskInProgress;

/**
 * Removes the HUD from it's parent view when hidden. 
 * Defaults to NO. 
 */
@property (assign) BOOL removeFromSuperViewOnHide;

/** 
 * Font to be used for the main label. Set this property if the default is not adequate. 
 */
@property (retain) UIFont* labelFont;

/** 
 * Font to be used for the details label. Set this property if the default is not adequate. 
 */
@property (retain) UIFont* detailsLabelFont;

/** 
 * The progress of the progress indicator, from 0.0 to 1.0. Defaults to 0.0. 
 */
@property (assign) float progress;


/** 
 * Display the HUD. You need to make sure that the main thread completes its run loop soon after this method call so
 * the user interface can be updated. Call this method when your task is already set-up to be executed in a new thread
 * (e.g., when using something like NSOperation or calling an asynchronous call like NSUrlRequest).
 *
 * If you need to perform a blocking thask on the main thread, you can try spining the run loop imeidiately after calling this 
 * method by using:
 *
 * [[NSRunLoop currentRunLoop] runUntilDate:[NSDate distantPast]];
 *
 * @param animated If set to YES the HUD will disappear using the current animationType. If set to NO the HUD will not use
 * animations while disappearing.
 */
- (void)show:(BOOL)animated;

/** 
 * Hide the HUD. This still calls the hudWasHidden delegate. This is the counterpart of the hide: method. Use it to
 * hide the HUD when your task completes.
 *
 * @param animated If set to YES the HUD will disappear using the current animationType. If set to NO the HUD will not use
 * animations while disappearing.
 */
- (void)hide:(BOOL)animated;

/** 
 * Hide the HUD after a delay. This still calls the hudWasHidden delegate. This is the counterpart of the hide: method. Use it to
 * hide the HUD when your task completes.
 *
 * @param animated If set to YES the HUD will disappear using the current animationType. If set to NO the HUD will not use
 * animations while disappearing.
 * @param delay Delay in secons until the HUD is hidden.
 */
- (void)hide:(BOOL)animated afterDelay:(NSTimeInterval)delay;

/** 
 * Shows the HUD while a background task is executing in a new thread, then hides the HUD.
 *
 * This method also takes care of NSAutoreleasePools so your method does not have to be concerned with setting up a
 * pool.
 *
 * @param method The method to be executed while the HUD is shown. This method will be executed in a new thread.
 * @param target The object that the target method belongs to.
 * @param object An optional object to be passed to the method.
 * @param animated If set to YES the HUD will disappear using the current animationType. If set to NO the HUD will not use
 * animations while disappearing.
 */
- (void)showWhileExecuting:(SEL)method onTarget:(id)target withObject:(id)object animated:(BOOL)animated;

@end

/////////////////////////////////////////////////////////////////////////////////////////////

@protocol MBProgressHUDDelegate <NSObject>

@optional

/** 
 * Called after the HUD was fully hidden from the screen. 
 */
- (void)hudWasHidden:(MBProgressHUD *)hud;

/**
 * @deprecated use hudWasHidden: instead
 * @see hudWasHidden:
 */
- (void)hudWasHidden __attribute__ ((deprecated)); 

@end

/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * A progress view for showing definite progress by filling up a circle (pie chart).
 */
@interface MBRoundProgressView : UIView {
@private
    float _progress;
}

/**
 * Progress (0.0 to 1.0)
 */
@property (nonatomic, assign) float progress;

@end

/////////////////////////////////////////////////////////////////////////////////////////////


@interface ehUrlGet : NSObject {
    
    CHAR * pszUrlRequest;
    NSURLConnection * nsConn;
    MBProgressHUD * poProgress;
    NSMutableData * receivedData;
	long long expectedLength;
	long long currentLength;
    
    void * funcToNotify;
	SEL methodForExecution;
	id targetForExecution;
	id objectForExecution;
    NSError * lastError;
    
}

//@property(nonatomic,readonly,retain) UINavigationController *navigationController; // If this view controller has been pushed onto a navigation controller, return it.
@property(nonatomic,retain) NSError * lastError; 
@property(nonatomic,retain) NSMutableData * receivedData;
- (id)initWithMethod:(SEL)selMethod target:(id)idTarget url:(CHAR *)pszPageOpen window:(UIWindow *)npsWin;
- (id)initWithFunc:(void(*)(BYTE * pbData,SIZE_T iLength))funcNotify url:(CHAR *)pszPageOpen window:(UIWindow *)npsWin;
- (BYTE *)getString;
- (UIImage *)getImage;
-(void)stop;

@end


// ############################################################################################
//
// ehForm - Costruttore di form ###############################################################
//
// ############################################################################################

//#define FLD_TYPE_MASK 0xff
typedef enum {
	FLD_TEXT,
	FLD_NUMBER,
	FLD_DATE,
	FLD_TEXTAREA,
	FLD_PASSWORD,

	FLD_PHONE,
	FLD_EMAIL,
	FLD_URL,
	FLD_NAME,
	
	FLD_BUTTON,
	FLD_CHECKBOX,
	FLD_RADIO,
	FLD_SELECT,			

	FLD_TITLE,			
    
	FLD_APPEND=0x100,
	FLD_QNOT=0x200		// Da non usare in query con il db (in Get)
    
} EN_FORM_FT; // Field Type

/*
 typedef enum {
 FCLS_UNKNOW,
 FCLS_TEXT,
 FCLS_SELECT,
 FCLS_BUTTON
 } EN_FORM_FC; // Field Class
 */


//
// ehFormField --------------------------------------------------
//
@interface ehFormFld : ehRoundRect {
    CGFloat fptLabelWidth;
    void *  psField;
    id      poFormTable;
}
@property (nonatomic, assign) CGFloat fptLabelWidth;
@property (nonatomic, assign) void *  psField;
@property (nonatomic, assign) id poFormTable;
@end

typedef struct {
    
	EN_FORM_FT  enType;	// Tipo di input
    //	EN_FLD_CLS	enClass;	// Interno: Testo,Select,Check
    
	CHAR *		pszName;	// Nome del campo (usato per rintracciarlo e negli eventi)
	CHAR *		pszButton;	// Testo del "button" (usato con i button o a destra del campo)
	//CHAR *		pszPostLabel;	// Testo a destra del campo
    NSString *  nszPostLabel;   // VAlore campo NSString  
	CGFloat		fWidth;		// Larghezza 0=100%
	CGFloat		fHeight;	// Larghezza 0=automatica (altezza riga)
	SINT		iAfterWidth;// Larghezza del testo after (dopo il campo di test0) 0= automatica
	SINT		iAlt;		// Altezza del campo (0=Default)
//	SINT		iRow;		// Riga (Virtuale) della tabella
    
	EH_COLOR	colText;	
	RECT		rcMargin;
    
	UTF8 *		pszValue;       // Valore del campo in UTF8
	UTF8 *		pszValueInit;   // Backup del valore iniziale da usare in cado di "Annullamento di editing"
    
    UTF8 *		pszValueShow;	// Valore del campo Mostrato in UTF8
    NSString *  nszValueShow;   // VAlore campo NSString  

    BOOL        bCleanOnBegin;  // T/F se deve pulire il campo in inizio editing
    
	BOOL		bAppend;        // Accodato al precedente
    //	BOOL		bQueryNot;	// T/F se non Ë un campo da associare ad una query (esempio testuali o descrittivi)
	BOOL		bDisable;       // T/F se disabilitato alla modifica
    
	EH_AR		arParam;        // Parametri aggiuntivi
	EH_AR		arOption;       // Elenco di opzioni per i campi select
    INT         iOptionSelected;
	BOOL		bCheck;         // Se settato per i checkbox
	SINT		iTextRows;      // Numero di righe (Text/Area)
	SINT		iMaxChar;       // Numero massimo di caratteri
	BOOL		bReadOnly;      // T/F se sono il lettura
	
	SINT		iNumberSize;		// Dimensioni del numero intero (1000 = 4)
	SINT		iNumberDecimal;		// Numero di decimali
	BOOL		bNumberThousandSep; // T/F Separatore delle migliaia
    
    INT         iSection;
    INT         iRow;
    
	// Classes
    //	RECT		rcClient;	// Area occupata dall'oggetto
    NSString *  nszName;        // Nome
    NSString *  nszLabel;       // Label
    NSString *  nszPlaceHolder; // PlaceHolder (mostrato con campo vuoto)
    UITableViewCell * cell;     // istanza cella
    id          poInput;        // Oggetto input (UITextField  UITextView)
    ehFormFld * psRoundBack;
    
} S_FORM_FIELD;

typedef struct {
    INT             iFields;    // Numero di campi presenti nella riga
    S_FORM_FIELD *  psStart;    // Puntatore al primo campo
    CGFloat         fHeight;    // Altezza della riga
    UITableViewCell * cell;     // istanza cella
} S_FORM_ROW;

typedef struct {

    S_FORM_FIELD *  psTitle;    // Elemento che contiene il titolo del settore
    S_FORM_ROW *    arsRows;    // Riga che contiene le righe del settore
    INT             iRows;      // Numero di righe del settore
    
    S_FORM_FIELD *  psFirst;    // Puntatore al primo campo
    INT             iFields;    // Campi presenti nella sezione 

} S_FORM_SECIDX;

@interface ehFormTable : UITableViewController<UITextFieldDelegate, UITextViewDelegate, UIPopoverControllerDelegate> {
    
    BOOL            bEditing;       // T/F se il form è in editing
    BOOL            biPad;          // T/F se è iPad
    NSString *      nszFormTitle;   // Titolo della finestra
    CGFloat         fptLabelWidth;  // dipensioni della label a sinistra
    CGFloat         fptInputLeftMargin;  // dipensioni della label a sinistra
    _DMI            dmiField;
    S_FORM_FIELD *  arsField;
    
    INT             iColumn;        // numero di colonne
    INT             iSecIdx;
    S_FORM_SECIDX * arsSecIdx;
    UIColor *       colBack;
    
    // Pickers
    UIDatePicker *  pickerDate;
    UIPopoverController * pickerPopover;

    UIBarButtonItem * leftButton;
    UIBarButtonItem * rightButton;

    NSString *      nszTitleBackup;
    id              leftButtonBackup;
    id              rightButtonBackup;
    id              titleViewBackup;
    
    id              inputFocus;
    CGFloat         fcxTablePadding;   // Il padding orizzontale delle celle della tabella (adesso è fisso, ma dovrei leggero da IOS)

    CGFloat         fcyInputMargin;    // Margin verticale dei campi all'interno della cella, comprese le label
    CGFloat         fcxInputMargin;    // Margin orizzontale (veid sopra)
    
}

@property (nonatomic, retain) UIDatePicker *pickerDate;
@property (nonatomic, retain) UIPopoverController *pickerPopover;
@property (nonatomic, retain) UIColor * colBack;
@property (nonatomic, readonly) BOOL biPad;

- (void) setTitleForm:(CHAR *)pszTitle;
- (void) setLabelWidth:(float)ptWidth;
- (void) addSection:(CHAR *)pszName; 
- (void) addFieldName:(CHAR *)pszName type:(EH_DATATYPE)enType value:(UTF8 *)pszValue label:(CHAR *)pszLabel;
- (void) addFieldName:(CHAR *)pszName type:(EH_DATATYPE)enType value:(UTF8 *)pszValue label:(CHAR *)pszLabel style:(CHAR *)pszStyle;
- (void) addFieldName:(CHAR *)pszName type:(EH_DATATYPE)enType value:(UTF8 *)pszValue label:(CHAR *)pszLabel placeHolder:(CHAR *)pszPlace style:(CHAR *)pszStyle;
- (void) addTo:(CHAR *)pszName postLabel:(CHAR *)pszLabel;
- (void) endForm;

- (void) setColumn:(INT)nCol;
- (void) setFieldValue:(S_FORM_FIELD *)psField value:(UTF8 *)pszValue;

- (void) pickerDateHide;
- (void) pickerDateStop;

- (BOOL) inputFocus:(id)poInput;
- (void) inputBlur:(id)poInput;

- (void) doneButton;
- (void) backButton;

- (EH_ARF) optionToArray:(S_FORM_FIELD *)psField idx:(int)idx;
- (EH_ARF) getSelectValue:(S_FORM_FIELD *)psField;
- (void) setOptionsTo:(CHAR *)name array:(EH_AR)ar;

- (void) showDateChoose:(S_FORM_FIELD *)psField;
- (void) styleParser:(S_FORM_FIELD *)psFld str:(CHAR *)pszParam;
- (UTF8 *)getValue:(CHAR *)pszName;

@end


//
// ehFormSelect -------------------------------------------------
//
@interface ehFormSelect : UITableViewController <UIActionSheetDelegate> {
    
    ehFormTable * ehForm;
    S_FORM_FIELD * psField;
    BOOL bPopOver;
    UIPopoverController * pickerPopover;
    
}
@property (assign, readwrite) S_FORM_FIELD * psField;
@property (assign, readwrite) ehFormTable * ehForm;
@property (assign, readwrite) BOOL bPopOver;
@property (assign, readwrite) UIPopoverController * pickerPopover;

-(id)initWithField:(S_FORM_FIELD *)ps popOver:(BOOL)bOver;
@end



#endif