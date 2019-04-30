#include <linux/uinput.h>
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define VENDOR_ID 0x9A7A
#define PRODUCT_ID 0xBA17



char *d2b(int x){
	int a, b, i;
	char *p;
	i = 0;
	p = (char*)malloc(8);

	if(p == NULL){
		exit(EXIT_FAILURE);
	}

	for(a = 7; a >= 0; a--){
		b = x >> a;
		if(b&1){
			*(p+i) = 1+'0';
		}
		else{
			*(p+i) = 0+'0';
		}
		i++;
	}
	return p;
}

int button(char * p){
	int a;
	if(p[3] == '1'){
		printf("\nButton is pressed!");
		a = 1;
	}
	else{
		printf("\nButton is not pressed!");
		a = 0;
	}
	return a;
}

int xaxis(char * p){
	int a;
	if(p[4] == '0' && p[5] == '1'){
		printf("\nTraveling left!");
		a = 1;
	}
	else if(p[4] == '1' && p[5] == '0'){
		printf("\nNot moving on x-axis!");
		a = 2;
	}
	else if(p[4] == '1' && p[5] == '1'){
		printf("\nTraveling right!");
		a = 3;
	}
	return a;
}

int yaxis(char * p){
	int a;
	if(p[6] == '0' && p[7] == '1'){
		printf("\nTraveling down!\n");
		a = 1;
	}
	else if(p[6] == '1' && p[7] == '0'){
		printf("\nNot moving on y-axis!\n");
		a = 2;
	}
	else if(p[6] == '1' && p[7] == '1'){
		printf("\nTraveling up!\n");
		a = 3;
	}
	return a;
}


/*USEFUL INFO

Interrupt Transfers -- One Endpoint -- Stream Pipe

ChompStick:
    Vendor ID:      0x9A7A
    Product ID:     0xBA17
*/
int main(){
//PART 1
//initialize the library by calling the function libusb_init and creating a session
libusb_context *context = NULL;
libusb_device **device;
int init_success = libusb_init(&context);
if (init_success == 0){
	printf("\nLibrary initialized successfully!\n");
}
else{
	printf("\nLibrary not initialized!\n");
}

libusb_set_debug(context, 3);


//Call the function libusb_get_device_list to get a list of connected devices. This creates an array of libusb_device containing all usb devices connected to the system.
ssize_t numDev = libusb_get_device_list(context, &device);
printf("\n%ld devices in the list", numDev);


//Loop through all these devices and check their options
struct libusb_device_descriptor descriptor;
struct libusb_config_descriptor *config;
int desc_success;
int config_success;
//This quadruple for loop gave me the interface number
for (int i = 0; i<numDev; i++){
	printf("\n");
	desc_success = libusb_get_device_descriptor(device[i], &descriptor);
	if(desc_success == 0){
		printf("\nDescriptor successfully created!");
	}
	else{
		printf("\nFailed to create descriptor");
	}
	config_success = libusb_get_config_descriptor(device[i], 0, &config);
	if(config_success == 0){
		printf("\nConfiguration Descriptor successfully created!");
	}
	else{
		printf("\nFailed to create configuration descriptor");
	}
	printf("\nVendorID: %d\nProductID: %d", descriptor.idVendor, descriptor.idProduct);
	printf("\nNumber of interfaces: %d", config->bNumInterfaces);

	const struct libusb_interface *interface;
	const struct libusb_interface_descriptor *interDesc;
	const struct libusb_endpoint_descriptor *epDesc;
	for(int j = 0; j < config->bNumInterfaces; j++){
		interface = &config->interface[j];
		int numAlts = interface->num_altsetting;
		printf("\nNumber of alternate settings: %d", numAlts);
		for(int k = 0; k < numAlts; k++){
			interDesc = &interface->altsetting[k];
			int intNum =  (int)interDesc->bInterfaceNumber;
			printf("\nInterface Number: %d", intNum);
			int epNum = (int)interDesc->bNumEndpoints;
			printf("\nNumber of endpoints: %d", intNum);
			for(int l = 0; l<epNum;l++){
				epDesc = &interDesc->endpoint[l];
				int descType = (int)epDesc->bDescriptorType;
				printf("\nDescriptor type: %d", descType);
				int epAddress = (int)epDesc->bEndpointAddress;
				printf("\nEndpoint Address: %d", epAddress);
			}
		}
	}
	printf("\n");
}

//Discover the one and open the device either by libusb_open or libusb_open_device_with_vid_pid(when you know vendor and product id of the device) to open the device
libusb_device_handle * device_handle = libusb_open_device_with_vid_pid(context, VENDOR_ID, PRODUCT_ID);
if(device_handle == NULL){
	printf("\nDevice not opened");
}
else{
	printf("\nDevice opened successfully!");
}

//Clear the list you got from libusb_get_device_list by using libusb_free_device_list
libusb_free_device_list(device, 1);

//Claim the interface with libusb_claim_interface (requires you to know the interface numbers of device)
int interClaim = libusb_claim_interface(device_handle, 0);
if(interClaim == 0){
	printf("\nInterface claimed successfully!");
}
else{
	printf("\nInterface not claimed");
}

struct uinput_setup u;
int version, rc, fd;
fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
printf("\nDID THIS WORK? PROLLY NAH %d", fd);
ioctl(fd, UI_SET_EVBIT, EV_KEY);
ioctl(fd, UI_SET_KEYBIT, BTN_0);
ioctl(fd, UI_SET_EVBIT, EV_ABS);
ioctl(fd, UI_SET_ABSBIT, ABS_X);
ioctl(fd, UI_SET_ABSBIT, ABS_Y);
memset(&u, 0, sizeof(u));
strcpy(u.name, "js0");
ioctl(fd, UI_DEV_SETUP, &u);
int joystick_test = ioctl(fd, UI_DEV_CREATE);
if(joystick_test == 0){
	printf("\nDevice create successfully!");
}
else{
	printf("\nDevice failed to create!");
}
sleep(1);
//Do desired I/O
unsigned char data[1];
int transferred;
int but;
int xBut;
int yBut;
int leaveLoop =1;
int interReleased = 0;
int dataTransfer = 0;
while(leaveLoop == 1){
	dataTransfer = libusb_interrupt_transfer(device_handle, 129, data, 16, &transferred, 0);
	if(dataTransfer == 0){
		printf("\nData transferred successfully!");
	}
	else{
		printf("\nData failed to transfer");
	}
	printf("\nData length: %d", transferred);
	printf("\nData: %d", data[0]);
	int x = data[0];
	char * p = d2b(x);
	printf("\nBinary representation: ");
	for(int i = 0; i<sizeof(p);i++){
		printf("%c", p[i]);
	}
	but = button(p);
	xBut = xaxis(p);
	yBut = yaxis(p);
	free(p);
	if(but == 0){
		struct input_event ie;
		ie.type = EV_KEY;
		ie.code = BTN_0;
		ie.value = 0;
		ie.time.tv_sec = 0;
		ie.time.tv_usec = 0;
		write(fd, &ie, sizeof(ie));

		struct input_event iev;
		iev.type = EV_SYN;
		iev.code = SYN_REPORT;
		iev.value = 0;
		iev.time.tv_sec = 0;
		iev.time.tv_usec = 0;
		write(fd, &iev, sizeof(iev));


	}
	else if(but == 1){
		struct input_event ie;
		ie.type = EV_KEY;
		ie.code = BTN_0;
		ie.value = 1;
		ie.time.tv_sec = 0;
		ie.time.tv_usec = 0;
		write(fd, &ie, sizeof(ie));

		struct input_event iev;
		iev.type = EV_SYN;
		iev.code = SYN_REPORT;
		iev.value = 0;
		iev.time.tv_sec = 0;
		iev.time.tv_usec = 0;
		write(fd, &iev, sizeof(iev));
	}
	if(xBut == 0 || yBut == 0){
		printf("\nERROR: Invalid button input");
	}
	if(xBut == 1){
		struct input_event iex;
		iex.type = EV_ABS;
		iex.code = ABS_X;
		iex.value = -32767;
		iex.time.tv_sec = 0;
		iex.time.tv_usec = 0;
		write(fd, &iex, sizeof(iex));

		struct input_event iexv;
		iexv.type = EV_ABS;
		iexv.code = BTN_JOYSTICK;
		iexv.value = 0;
		iexv.time.tv_sec = 0;
		iexv.time.tv_usec = 0;
		write(fd, &iexv, sizeof(iexv));

	}
	else if(xBut == 2){
		struct input_event iex;
		iex.type = EV_ABS;
		iex.code = ABS_X;
		iex.value = 0;
		iex.time.tv_sec = 0;
		iex.time.tv_usec = 0;
		write(fd, &iex, sizeof(iex));

		struct input_event iexv;
		iexv.type = EV_ABS;
		iexv.code = BTN_JOYSTICK;
		iexv.value = 0;
		iexv.time.tv_sec = 0;
		iexv.time.tv_usec = 0;
		write(fd, &iexv, sizeof(iexv));
	}
	else if(xBut == 3){
		struct input_event iex;
		iex.type = EV_ABS;
		iex.code = ABS_X;
		iex.value = 32767;
		iex.time.tv_sec = 0;
		iex.time.tv_usec = 0;
		write(fd, &iex, sizeof(iex));

		struct input_event iexv;
		iexv.type = EV_ABS;
		iexv.code = BTN_JOYSTICK;
		iexv.value = 0;
		iexv.time.tv_sec = 0;
		iexv.time.tv_usec = 0;
		write(fd, &iexv, sizeof(iexv));
	}
	if(yBut == 1){
		struct input_event iey;
		iey.type = EV_ABS;
		iey.code = ABS_Y;
		iey.value = 32767;
		iey.time.tv_sec = 0;
		iey.time.tv_usec = 0;
		write(fd, &iey, sizeof(iey));

		struct input_event ieyv;
		ieyv.type = EV_ABS;
		ieyv.code = BTN_JOYSTICK;
		ieyv.value = 0;
		ieyv.time.tv_sec = 0;
		ieyv.time.tv_usec = 0;
		write(fd, &ieyv, sizeof(ieyv));
	}
	else if(yBut == 2){
		struct input_event iey;
		iey.type = EV_ABS;
		iey.code = ABS_Y;
		iey.value = 0;
		iey.time.tv_sec = 0;
		iey.time.tv_usec = 0;
		write(fd, &iey, sizeof(iey));

		struct input_event ieyv;
		ieyv.type = EV_ABS;
		ieyv.code = BTN_JOYSTICK;
		ieyv.value = 0;
		ieyv.time.tv_sec = 0;
		ieyv.time.tv_usec = 0;
		write(fd, &ieyv, sizeof(ieyv));
	}
	else if(yBut == 3){
		struct input_event iey;
		iey.type = EV_ABS;
		iey.code = ABS_Y;
		iey.value = -32767;
		iey.time.tv_sec = 0;
		iey.time.tv_usec = 0;
		write(fd, &iey, sizeof(iey));

		struct input_event ieyv;
		ieyv.type = EV_ABS;
		ieyv.code = BTN_JOYSTICK;
		ieyv.value = 0;
		ieyv.time.tv_sec = 0;
		ieyv.time.tv_usec = 0;
		write(fd, &ieyv, sizeof(ieyv));
	}
	if (dataTransfer != 0){
		//I'm pretty sure the release interface will never work
		interReleased = libusb_release_interface(device_handle, 0);
		leaveLoop = 0;
	}
}
sleep(1);
//Release the device by using libusb_release_interface
//int interReleased = libusb_release_interface(device_handle, 0);
if(interReleased == 0){
	printf("\nDevice released successfully!");
}
else{
	printf("\nDevice failed to release");
}

//Close the device you openedbefore, by using libusb_close
libusb_close(device_handle);
//Close the session by using libusb_exit
libusb_exit(context);

return 0;
}

