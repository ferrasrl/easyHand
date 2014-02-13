//
// usb.c
//
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/usb.h"

#pragma message("--> Includo libusb.lib <-------- per USB -----------")
#pragma comment(lib, "/easyhand/ehtoolx/libusb-win32-bin-1.2.6.0/lib/msvc/libusb.lib")

// Enables this example to work with a device running the
// libusb-win32 PIC Benchmark Firmware.
#define BENCHMARK_DEVICE

//////////////////////////////////////////////////////////////////////////////
// TEST SETUP (User configurable)

// Issues a Set configuration request
#define TEST_SET_CONFIGURATION

// Issues a claim interface request
#define TEST_CLAIM_INTERFACE

// Use the libusb-win32 async transfer functions. see
// transfer_bulk_async() below.
#define TEST_ASYNC

// Attempts one bulk read.
#define TEST_BULK_READ

// Attempts one bulk write.
// #define TEST_BULK_WRITE

//////////////////////////////////////////////////////////////////////////////
// DEVICE SETUP (User configurable)

// Device vendor and product id.
#define MY_VID 0x0666
#define MY_PID 0x0001

// Device configuration and interface id.
#define MY_CONFIG 1
#define MY_INTF 0

// Device endpoint(s)
#define EP_IN 0x81
#define EP_OUT 0x01

// Device of bytes to transfer.
#define BUF_SIZE 64

#define OUT_ENDPOINT 0x01
#define IN_ENDPOINT 0x82
#define DEV_TIMEOUT 5000


//////////////////////////////////////////////////////////////////////////////
usb_dev_handle *open_dev(void);

static int transfer_bulk_async(usb_dev_handle *dev,
                               int ep,
                               char *bytes,
                               int size,
                               int timeout);

usb_dev_handle *open_dev(void)
{
    struct usb_bus *bus;
    struct usb_device *dev;

    for (bus = usb_get_busses(); bus; bus = bus->next)
    {
        for (dev = bus->devices; dev; dev = dev->next)
        {
            if (dev->descriptor.idVendor == MY_VID
                    && dev->descriptor.idProduct == MY_PID)
            {
                return usb_open(dev);
            }
        }
    }
    return NULL;
}


//
// usbDevicePrint
//
S_USB_DEV_INFO * usbDevInfoCreate(struct usb_device * dev)
{
	usb_dev_handle *udev;
	CHAR szDesc[1024];
	CHAR szBuffer[256];
	INT	ret;
	INT verbose=1;
	S_USB_DEV_INFO * psDi;

	udev = usb_open(dev); if (!udev) return NULL;
	if (udev) {
		
		psDi=ehAllocZero(sizeof(S_USB_DEV_INFO));
		memcpy(&psDi->sDecriptor,&dev->descriptor,sizeof(dev->descriptor));

		sprintf(szDesc,"%d|%d|%d",dev->descriptor.idVendor,dev->descriptor.idProduct,dev->descriptor.bcdDevice);
		psDi->pszCode=strDup(szDesc);

		//
		// Costruttore
		//
		if (dev->descriptor.iManufacturer) {
			ret = usb_get_string_simple(udev, dev->descriptor.iManufacturer, szBuffer, sizeof(szBuffer));
			if (ret > 0)
				snprintf(szDesc, sizeof(szDesc), "%s", szBuffer);
			else
				snprintf(szDesc, sizeof(szDesc), "%04X",dev->descriptor.idVendor);
		} else
			strcpy(szDesc,"unknow");
		psDi->pszManufacturer=strDup(szDesc);

		//
		// Prodotto
		//
		if (dev->descriptor.iProduct) {
			ret = usb_get_string_simple(udev, dev->descriptor.iProduct, szBuffer, sizeof(szBuffer));
			if (ret > 0)
				sprintf(szDesc, "%s", szBuffer);
			else
				sprintf(szDesc, "%04X", dev->descriptor.idProduct);
		} else
			sprintf(szDesc, "%04X", dev->descriptor.idProduct);

		psDi->pszProduct=strDup(szDesc);


		//
		// Serial
		//
		if (dev->descriptor.iSerialNumber) {
			ret = usb_get_string_simple(udev, dev->descriptor.iSerialNumber, szBuffer, sizeof(szBuffer));
			if (ret > 0)
				sprintf(szDesc, "%s", szBuffer);
			else
				sprintf(szDesc, "%04X", dev->descriptor.iSerialNumber);
		} 
		psDi->pszSerial=strDup(szDesc);

		
	} 
	

	return psDi;
}

//
// usbDevInfoDestroy()
//
S_USB_DEV_INFO * usbDevInfoDestroy(S_USB_DEV_INFO * psInfo) {

	if (!psInfo) return NULL;
	ehFreePtrs(4,&psInfo->pszCode,&psInfo->pszManufacturer,&psInfo->pszProduct,&psInfo->pszSerial);
	ehFree(psInfo);
	return NULL;
}


//
// usbDevicePrint
//
int usbDevicePrint(struct usb_device *dev, int level)
{
	usb_dev_handle *udev;
	CHAR szDesc[1024];
	CHAR szBuffer[256];
	INT	ret;
	INT verbose=1;

	udev = usb_open(dev);
	if (udev) {
		if (dev->descriptor.iManufacturer) {
			ret = usb_get_string_simple(udev, dev->descriptor.iManufacturer, szBuffer, sizeof(szBuffer));
			if (ret > 0)
				snprintf(szDesc, sizeof(szDesc), "%s - ", szBuffer);
			else
				snprintf(szDesc, sizeof(szDesc), "%04X - ",
				dev->descriptor.idVendor);
		} else
			snprintf(szDesc, sizeof(szDesc), "%04X - ",
			dev->descriptor.idVendor);

		if (dev->descriptor.iProduct) {
			ret = usb_get_string_simple(udev, dev->descriptor.iProduct, szBuffer, sizeof(szBuffer));
			if (ret > 0)
				strAppend(szDesc, "%s", szBuffer);
			else
				strAppend(szDesc, "%04X", dev->descriptor.idProduct);
		} else
			strAppend(szDesc, "%04X", dev->descriptor.idProduct);

	} else
		snprintf(szDesc, sizeof(szDesc), "%04X - %04X",
		dev->descriptor.idVendor, dev->descriptor.idProduct);

	printf("%.*sDev #%d: %s\n", level * 2, " ", dev->devnum, szDesc);

	if (udev && verbose) {
		if (dev->descriptor.iSerialNumber) {
			ret = usb_get_string_simple(udev, dev->descriptor.iSerialNumber, szBuffer, sizeof(szBuffer));
			if (ret > 0)
				printf("%.*s - Serial Number: %s\n", level * 2,
				" ", szBuffer);
		}
	}
	return 0;
}
//
// usbStart()
//
void usbStart(void) {

	// Inizializzo 
    usb_init(); 
    usb_find_busses(); 
    usb_find_devices(); 

}

//
//  usbExit();
//
void usbExit(void) {

}


//
// usbDeviceSearch()
//
usb_dev_handle * usbDeviceSearch(INT idVendor,INT idProduct) {

	struct usb_bus * bus;
	struct usb_device * dev = NULL;
	usb_dev_handle * handle=NULL;

	for (bus = usb_get_busses(); bus; bus = bus->next) {
//		if (verbose) printf("Found bus %s\n", bus->dirname);
		for (dev = bus->devices; dev; dev = dev->next) {
			//if (verbose)
			//	printf("Found device with idVendor 0x%x and idProduct 0x%x\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
			if ((dev->descriptor.idProduct == idProduct) && (dev->descriptor.idVendor == idVendor)){
	//			printf("Device found -> open\n");
				handle = usb_open(dev);
				if (!handle) {
//					printf("invalid handle: %s\n", usb_strerror());
					return NULL;
				} else break;
			}
		}
	}

	return handle;
}


//
// usbWrite() - Scrive dati sulla Usb
//
INT usbWrite(usb_dev_handle * hDev,BYTE * pbData,INT iSize)
{
	int res;

	// Claim
	if (usb_claim_interface(hDev, 0) < 0) {
		printf("error on usb_claim_interface: %s\n", usb_strerror());
		return -1;
	}

/*
#ifdef SET_ALTINTERFACE_ONCE
	if (first) {
		first = false;
#endif
		if (usb_set_altinterface(handle, ALTINTERFACE) < 0){
			printf("usb_set_altinterface failed: %s\n", usb_strerror());
		}
#ifdef SET_ALTINTERFACE_ONCE
	}
#endif
*/

	res = usb_bulk_write(hDev, OUT_ENDPOINT, pbData, iSize, DEV_TIMEOUT);
	if (res < 0){
		printf("usb_bulk_write failed: %s\n", usb_strerror());
		return -2;
	}

	usb_release_interface(hDev, 0);
	return res;
}


//
// usbRead() - Scrive dati sulla Usb
//
INT usbRead(usb_dev_handle * hDev,BYTE * pbBuffer,INT iSizeBuffer)
{
	int res;

	// Claim
	if (usb_claim_interface(hDev, 0) < 0) {
		printf("error on usb_claim_interface: %s" CRLF, usb_strerror());
		return -1;
	}

/*
#ifdef SET_ALTINTERFACE_ONCE
	if (first) {
		first = false;
#endif
		if (usb_set_altinterface(handle, ALTINTERFACE) < 0){
			printf("usb_set_altinterface failed: %s\n", usb_strerror());
		}
#ifdef SET_ALTINTERFACE_ONCE
	}
#endif
*/
	*pbBuffer=0;
	res=usb_bulk_read(hDev, IN_ENDPOINT, pbBuffer, iSizeBuffer, DEV_TIMEOUT);
	if (res < 0){
		printf("usb_bulk_read failed: %s"  CRLF, usb_strerror());
		return -2;
	}
	pbBuffer[res]=0;
	usb_release_interface(hDev, 0);
	return res;

}


//
// usbShowDevices()
//
int usbShowDevices(void)
{
    usb_dev_handle * dev = NULL; /* the device handle */
  //  char tmp[BUF_SIZE];
//  int ret;
    void * async_read_context = NULL;
    void * async_write_context = NULL;
//	libusb_device **list;
//	libusb_device *found = NULL;
	struct usb_bus * psBus;
//	ssize_t cnt;

//	usb_set_debug(255);


//	psBus=usb_get_busses();


	// Giro sui busses
	for (psBus=usb_get_busses();psBus;psBus=psBus->next) {

		if (psBus->root_dev&&1==0) {
			usbDevicePrint(psBus->root_dev, 0);
		}
			else {
				struct usb_device *dev;
				for (dev = psBus->devices; dev; dev = dev->next)
					usbDevicePrint(dev, 0);
			}

//		printf("Bus: %s" CRLF,psBus->dirname);
	}
	return 0;
}


	
	// discover devices
	//cnt = usb_get_device_list(NULL, &list);
	/*
	ssize_t i = 0;
int err = 0;
if (cnt < 0)
    error();

for (i = 0; i < cnt; i++) {
    libusb_device *device = list[i];
    if (is_interesting(device)) {
        found = device;
        break;
    }
}

if (found) {
    libusb_device_handle *handle;

    err = libusb_open(found, &handle);
    if (err)
        error();
    // etc
}

*/

//libusb_free_device_list(list, 1);
/*

    if (!(dev = open_dev()))
    {
        printf("error opening device: \n%s\n", usb_strerror());
        return 0;
    }
    else
    {
        printf("success: device %04X:%04X opened\n", MY_VID, MY_PID);
    }

#ifdef TEST_SET_CONFIGURATION
    if (usb_set_configuration(dev, MY_CONFIG) < 0)
    {
        printf("error setting config #%d: %s\n", MY_CONFIG, usb_strerror());
        usb_close(dev);
        return 0;
    }
    else
    {
        printf("success: set configuration #%d\n", MY_CONFIG);
    }
#endif

#ifdef TEST_CLAIM_INTERFACE
    if (usb_claim_interface(dev, 0) < 0)
    {
        printf("error claiming interface #%d:\n%s\n", MY_INTF, usb_strerror());
        usb_close(dev);
        return 0;
    }
    else
    {
        printf("success: claim_interface #%d\n", MY_INTF);
    }
#endif

#ifdef TEST_BULK_WRITE

#ifdef BENCHMARK_DEVICE
    ret = usb_control_msg(dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                          14, // set/get test 
                          2,  // test type    
                          MY_INTF,  // interface id 
                          tmp, 1, 1000);
#endif

#ifdef TEST_ASYNC
    // Running an async write test
    ret = transfer_bulk_async(dev, EP_OUT, tmp, sizeof(tmp), 5000);
#else
    // Running a sync write test
    ret = usb_bulk_write(dev, EP_OUT, tmp, sizeof(tmp), 5000);
#endif
    if (ret < 0)
    {
        printf("error writing:\n%s\n", usb_strerror());
    }
    else
    {
        printf("success: bulk write %d bytes\n", ret);
    }
#endif

#ifdef TEST_BULK_READ

#ifdef BENCHMARK_DEVICE
    ret = usb_control_msg(dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                          14, // set/get test 
                          1,  // test type    
                          MY_INTF,  // interface id 
                          tmp, 1, 1000);
#endif

#ifdef TEST_ASYNC
    // Running an async read test
    ret = transfer_bulk_async(dev, EP_IN, tmp, sizeof(tmp), 5000);
#else
    // Running a sync read test
    ret = usb_bulk_read(dev, EP_IN, tmp, sizeof(tmp), 5000);
#endif
    if (ret < 0)
    {
        printf("error reading:\n%s\n", usb_strerror());
    }
    else
    {
        printf("success: bulk read %d bytes\n", ret);
    }
#endif

#ifdef TEST_CLAIM_INTERFACE
    usb_release_interface(dev, 0);
#endif

    if (dev)
    {
        usb_close(dev);
    }
    printf("Done.\n");

    return 0;
}
*/




/*
* Read/Write using async transfer functions.
*
* NOTE: This function waits for the transfer to complete essentially making
* it a sync transfer function so it only serves as an example of how one might
* implement async transfers into thier own code.
*/
/*
static int transfer_bulk_async(usb_dev_handle *dev,
                               int ep,
                               char *bytes,
                               int size,
                               int timeout)
{
    // Each async transfer requires it's own context. A transfer
    // context can be re-used.  When no longer needed they must be
    // freed with usb_free_async().
    //
    void* async_context = NULL;
    int ret;

    // Setup the async transfer.  This only needs to be done once
    // for multiple submit/reaps. (more below)
    //
    ret = usb_bulk_setup_async(dev, &async_context, ep);
    if (ret < 0)
    {
        printf("error usb_bulk_setup_async:\n%s\n", usb_strerror());
        goto Done;
    }

    // Submit this transfer.  This function returns immediately and the
    // transfer is on it's way to the device.
    //
    ret = usb_submit_async(async_context, bytes, size);
    if (ret < 0)
    {
        printf("error usb_submit_async:\n%s\n", usb_strerror());
        usb_free_async(&async_context);
        goto Done;
    }

    // Wait for the transfer to complete.  If it doesn't complete in the
    // specified time it is cancelled.  see also usb_reap_async_nocancel().
    //
    ret = usb_reap_async(async_context, timeout);

    // Free the context.
    usb_free_async(&async_context);

Done:
    return ret;
}

*/

/*

#include <stdio.h>
#include <string.h>
#include "usb.h"

#define snprintf printf

#define ID_PRODUCT 0x0200
#define ID_VENDOR 0x8235

#define CONFIGURATION 1
#define INTERFACE_ 0
#define ALTINTERFACE 0
#define TIMEOUT 5000


// #define SET_ALTINTERFACE_ONCE

int verbose = 0;
unsigned char first = true;

void print_endpoint(struct usb_endpoint_descriptor *endpoint)
{
	printf(" bEndpointAddress: &#37;02xh\n", endpoint->bEndpointAddress);
	printf(" bmAttributes: %02xh\n", endpoint->bmAttributes);
	printf(" wMaxPacketSize: %d\n", endpoint->wMaxPacketSize);
	printf(" bInterval: %d\n", endpoint->bInterval);
	printf(" bRefresh: %d\n", endpoint->bRefresh);
	printf(" bSynchAddress: %d\n", endpoint->bSynchAddress);
}

void print_altsetting(struct usb_interface_descriptor *interface)
{
	int i;

	printf(" bInterfaceNumber: %d\n", interface->bInterfaceNumber);
	printf(" bAlternateSetting: %d\n", interface->bAlternateSetting);
	printf(" bNumEndpoints: %d\n", interface->bNumEndpoints);
	printf(" bInterfaceClass: %d\n", interface->bInterfaceClass);
	printf(" bInterfaceSubClass: %d\n", interface->bInterfaceSubClass);
	printf(" bInterfaceProtocol: %d\n", interface->bInterfaceProtocol);
	printf(" iInterface: %d\n", interface->iInterface);

	for (i = 0; i < interface->bNumEndpoints; i++)
		print_endpoint(&interface->endpoint[i]);
}

void print_interface(struct usb_interface *interface)
{
	int i;

	for (i = 0; i < interface->num_altsetting; i++)
		print_altsetting(&interface->altsetting[i]);
}

void print_configuration(struct usb_config_descriptor *config)
{
	int i;

	printf(" wTotalLength: %d\n", config->wTotalLength);
	printf(" bNumInterfaces: %d\n", config->bNumInterfaces);
	printf(" bConfigurationValue: %d\n", config->bConfigurationValue);
	printf(" iConfiguration: %d\n", config->iConfiguration);
	printf(" bmAttributes: %02xh\n", config->bmAttributes);
	printf(" MaxPower: %d\n", config->MaxPower);

	for (i = 0; i < config->bNumInterfaces; i++)
		print_interface(&config->interface[i]);
}

int print_device(struct usb_device *dev, int level)
{
	usb_dev_handle *udev;
	char description[256];
	char string[256];
	int ret, i;

	udev = usb_open(dev);
	if (udev) {
		if (dev->descriptor.iManufacturer) {
			ret = usb_get_string_simple(udev, dev->descriptor.iManufacturer, string, sizeof(string));
			if (ret > 0)
				snprintf(description, sizeof(description), "%s - ", string);
			else
				snprintf(description, sizeof(description), "%04X - ",
				dev->descriptor.idVendor);
		} else
			snprintf(description, sizeof(description), "%04X - ",
			dev->descriptor.idVendor);

		if (dev->descriptor.iProduct) {
			ret = usb_get_string_simple(udev, dev->descriptor.iProduct, string, sizeof(string));
			if (ret > 0)
				snprintf(description + strlen(description), sizeof(description) -
				strlen(description), "%s", string);
			else
				snprintf(description + strlen(description), sizeof(description) -
				strlen(description), "%04X", dev->descriptor.idProduct);
		} else
			snprintf(description + strlen(description), sizeof(description) -
			strlen(description), "%04X", dev->descriptor.idProduct);

	} else
		snprintf(description, sizeof(description), "%04X - %04X",
		dev->descriptor.idVendor, dev->descriptor.idProduct);

	printf("%.*sDev #%d: %s\n", level * 2, " ", dev->devnum,
		description);

	if (udev && verbose) {
		if (dev->descriptor.iSerialNumber) {
			ret = usb_get_string_simple(udev, dev->descriptor.iSerialNumber, string, sizeof(string));
			if (ret > 0)
				printf("%.*s - Serial Number: %s\n", level * 2,
				" ", string);
		}
	}

	if (udev)
		usb_close(udev);

	if (verbose) {
		if (!dev->config) {
			printf(" Couldn't retrieve descriptors\n");
			return 0;
		}

		for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
			print_configuration(&dev->config[i]);
	} else {
		for (i = 0; i < dev->num_children; i++)
			print_device(dev->children[i], level + 1);
	}

	return 0;
}

int read(struct usb_dev_handle *handle)
{
	if (usb_claim_interface(handle, INTERFACE_) < 0) {
		printf("error on usb_claim_interface: %s\n", usb_strerror());
		return -1;
	}
	printf("usb_claim_interface successful\n");
#ifdef SET_ALTINTERFACE_ONCE
	if (first) {
		first = false;
#endif
		if (usb_set_altinterface(handle, ALTINTERFACE) < 0){
			printf("usb_set_altinterface failed: %s\n", usb_strerror());
		}
#ifdef SET_ALTINTERFACE_ONCE
	}
#endif

	int size = 512, res;
	char *data = (char *) malloc(size*sizeof(char));
	res = usb_bulk_read(handle, IN_ENDPOINT, data, size, TIMEOUT);
	if (res < 0){
		printf("usb_bulk_read failed: %s\n", usb_strerror());
	}
	printf("usb_bulk_read: %d bytes read: ", res);
	for (int i = 0; i < res; ++i) {
		printf("%3x ", data[i]);
	}
	printf("\n");

	usb_release_interface(handle, INTERFACE_);
	free(data);
}

int write(struct usb_dev_handle *handle)
{
	int size = 12;
	char *data = (char *) malloc(size*sizeof(char));
	data[0] = 0x33;
	data[1] = 0x5B;
	data[2] = 0x02;
	data[3] = 0x01;
	data[4] = 0x00;
	data[5] = 0x05;
	data[6] = 0x01;
	data[7] = 0x03;
	data[8] = 0x07;
	data[9] = 0x0F;
	data[10] = 0x7F;
	data[11] = 0x1F;
	// data = {0x33, 0x5B, 0x02, 0x01, 0x00, 0x05, 0x01, 0x03, 0x07, 0x0F, 0x7F, 0x1F};

	if (usb_claim_interface(handle, INTERFACE_) < 0) {
		printf("error on usb_claim_interface: %s\n", usb_strerror());
		return -1;
	}
	printf("usb_claim_interface successful\n");
#ifdef SET_ALTINTERFACE_ONCE
	if (first) {
		first = false;
#endif
		if (usb_set_altinterface(handle, ALTINTERFACE) < 0){
			printf("usb_set_altinterface failed: %s\n", usb_strerror());
		}
#ifdef SET_ALTINTERFACE_ONCE
	}
#endif

	printf("usb_bulk_write: writing %d bytes: ", size);
	for (int i = 0; i < size; ++i) {
		printf("%3x ", data[i]);
	}
	printf("\n");

	int res = usb_bulk_write(handle, OUT_ENDPOINT, data, size, TIMEOUT);
	if (res < 0){
		printf("usb_bulk_write failed: %s\n", usb_strerror());
		return -1;
	}

	printf("usb_bulk_write: %d bytes written\n", res);

	usb_release_interface(handle, INTERFACE_);
	free(data);
}

int readWrite(struct usb_dev_handle *handle)
{

	int size = 512;
	char *data = (char *) malloc(size*sizeof(char));

	printf("type a string...\n");
	scanf("%s", data);	// Get a string

	if (usb_claim_interface(handle, INTERFACE_) < 0) {
		printf("error on usb_claim_interface: %s\n", usb_strerror());
		system("PAUSE");
		return -1;
	}
	printf("usb_claim_interface successful\n");
#ifdef SET_ALTINTERFACE_ONCE
	if (first) {
		first = false;
#endif
		if (usb_set_altinterface(handle, ALTINTERFACE) < 0){
			printf("usb_set_altinterface failed: %s\n", usb_strerror());
		}
#ifdef SET_ALTINTERFACE_ONCE
	}
#endif

	if (usb_bulk_write(handle, OUT_ENDPOINT, data, strlen(data), 3000) < 0){
		printf("usb_bulk_write failed: %s\n", usb_strerror());
		system("PAUSE");
		return -1;
	}

	strcpy(data, "12345678901234567890");
	printf("%s\n", "read data");
	if (usb_bulk_read(handle, IN_ENDPOINT, data, size, 3000) < 0){
		printf("usb_bulk_read failed: %s\n", usb_strerror());
	}
	printf("output %d, %s\n", size, data);
	//	for (int i = 0; i < size; ++i) {
	//	 printf("%4x ", data[i]);
	//	}

	usb_release_interface(handle, INTERFACE_);
	free(data);
}

int readWriteLoop(struct usb_dev_handle *handle)
{
	int NOF_LOOPS = 20;
	int size = 12;
	char *data = (char *) malloc(size*sizeof(char));
	data[0] = 0x33;
	data[1] = 0x5B;
	data[2] = 0x02;
	data[3] = 0x01;
	data[4] = 0x00;
	data[5] = 0x05;
	data[6] = 0x01;
	data[7] = 0x03;
	data[8] = 0x07;
	data[9] = 0x0F;
	data[10] = 0x7F;
	data[11] = 0x1F;
	// data = {0x33, 0x5B, 0x02, 0x01, 0x00, 0x05, 0x01, 0x03, 0x07, 0x0F, 0x7F, 0x1F};

	if (usb_claim_interface(handle, INTERFACE_) < 0) {
		printf("error on usb_claim_interface: %s\n", usb_strerror());
		return -1;
	}
	printf("usb_claim_interface successful\n");
#ifdef SET_ALTINTERFACE_ONCE
	if (first) {
		first = false;
#endif
		if (usb_set_altinterface(handle, ALTINTERFACE) < 0){
			printf("usb_set_altinterface failed: %s\n", usb_strerror());
		}
#ifdef SET_ALTINTERFACE_ONCE
	}
#endif

	printf("usb_bulk_write: writing %d bytes: ", size);
	for (int i = 0; i < size; ++i) {
		printf("%3x ", data[i]);
	}
	printf("\n------------------------\n");

	for (int var = 0; var < NOF_LOOPS; ++var) {

		int res = usb_bulk_write(handle, OUT_ENDPOINT, data, size, TIMEOUT);
		if (res < 0){
			printf("usb_bulk_write failed: %s\n", usb_strerror());
			return -1;
		}

		printf("usb_bulk_write: %d bytes written\n", res);

		int size = 64;
		char *data = (char *) malloc(size*sizeof(char));
		res = usb_bulk_read(handle, IN_ENDPOINT, data, size, TIMEOUT);
		if (res < 0){
			printf("usb_bulk_read failed: %s\n", usb_strerror());
		}
		printf("usb_bulk_read: %d bytes read: ", res);
		for (int i = 0; i < res; ++i) {
			printf("%3x ", data[i]);
		}
		printf("\n");
	}

	usb_release_interface(handle, INTERFACE_);
	free(data);
}

void logDevices()
{
	struct usb_bus *bus;

	printf("log devices...\n");
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		if (bus->root_dev && !verbose)
			print_device(bus->root_dev, 0);
		else {
			struct usb_device *dev;

			for (dev = bus->devices; dev; dev = dev->next)
				print_device(dev, 0);
		}
	}
}
*/





/*
int main(int argc, char *argv[])
{
	struct usb_bus *bus;
	struct usb_device *dev;
	struct usb_dev_handle *handle;

	bool run = true;

	if (argc > 1 && !strcmp(argv[1], "-v"))
		verbose = 1;

	usb_set_debug(255);

	printf("initialize libraray, find busses and devices\n");
	usb_init();

	usb_find_busses();
	usb_find_devices();

	if (verbose)
		logDevices();

	int size = 512;
	char *data = (char *) malloc(size*sizeof(char));

	printf("Search for device with idVendor 0x%x and idProduct 0x%x\n", ID_VENDOR, ID_PRODUCT);
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		if (verbose)
			printf("Found bus %s\n", bus->dirname);
		for (dev = bus->devices; dev; dev = dev->next) {
			if (verbose)
				printf("Found device with idVendor 0x%x and idProduct 0x%x\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
			if ((dev->descriptor.idProduct == ID_PRODUCT) && (dev->descriptor.idVendor == ID_VENDOR)){
				printf("Device found -> open\n");
				handle = usb_open(dev);
				if (!handle) {
					printf("invalid handle: %s\n", usb_strerror());
					system("PAUSE");
					return -1;
				}
				printf("Set configuration\n");
				if (usb_set_configuration(handle, CONFIGURATION) < 0) {
					printf("error on usb_set_configuration: %s\n", usb_strerror());
					system("PAUSE");
					return -1;
				}

				printf("Set altinterface (must failed)\n");
#ifdef SET_ALTINTERFACE_ONCE
				if (first) {
					first = false;
#endif
					if (usb_set_altinterface(handle, ALTINTERFACE) < 0){
						printf("usb_set_altinterface failed: %s\n", usb_strerror());
					}
#ifdef SET_ALTINTERFACE_ONCE
				}
#endif

				printf("w=write, r=read, x=exit, t=write+read, u=write+read(2), l=r/w loop, z=reset and open\n");

				while (run) {
					scanf("%s", data);

					switch (data[0]) {
case 'w':	// write
	write(handle);
	break;
case 'r':	// read
	read(handle);
	break;
case 'x':	// exit
	run = false;
	break;
case 't':	// write + read
	if (write(handle)) {
		read(handle);
	}
	break;
case 'u':	// write + read
	readWrite(handle);
	break;
case 'l':	// loop
	readWriteLoop(handle);
	break;
case 's':	// reset first flag (set_altinterface())
	first = true;
	break;
case 'z':	// reset and open
	usb_reset(handle);
	handle = usb_open(dev);
	break;
default:
	break;
					}
				}
				printf("\ndone\n");
			}
		}
	}
	free(data);
	system("PAUSE");

	return 1;
}

*/