//   +-------------------------------------------+
//    usb.h
//
//                by Ferrà Art & Technology 2013
//   +-------------------------------------------+

#include "/Easyhand/ehtoolx/libusb-win32-bin-1.2.6.0/include/lusb0_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

//#ifdef EH_MAIN
//#endif

void				usbStart(void);
void				usbExit(void);
int					usbShowDevices(void);
usb_dev_handle *	usbDeviceSearch(INT idVendor,INT idProduct);
INT					usbWrite(usb_dev_handle * hDev,BYTE * pbData,INT iSizeData);
INT					usbRead(usb_dev_handle * hDev,BYTE * pbBuffer,INT iSizeBuffer);

typedef struct {
	
	struct usb_device_descriptor sDecriptor;
	CHAR * pszCode; // Codificazione per trovarla

	CHAR * pszManufacturer;
	CHAR * pszProduct;
	CHAR * pszSerial;

} S_USB_DEV_INFO;

S_USB_DEV_INFO * usbDevInfoCreate(struct usb_device * dev);
S_USB_DEV_INFO * usbDevInfoDestroy(S_USB_DEV_INFO * psInfo);

#ifdef __cplusplus
}
#endif
