all: usb_test v4l2_capture
v4l2_capture : v4l2_capture.c
	arm-linux-gcc v4l2_capture.c -o v4l2_capture
	chmod +x v4l2_capture
usb_test : usb_test.c
	arm-linux-gcc usb_test.c -o usb_test
	chmod +x usb_test
clean:
	rm usb_test  v4l2_capture
