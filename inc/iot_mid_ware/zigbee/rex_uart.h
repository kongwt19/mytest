#ifndef __REX_UART_H__
#define __REX_UART_H__


typedef enum _usb_port_sta
{
	USB_ADD = 0,
	USB_REMOVE
}USB_PORT_STA_E;


typedef void(*USB_PORT_CHECK)(USB_PORT_STA_E sta);


void reg_usb_port_callback(USB_PORT_CHECK cb);
char *read_uart_dev(void);
int usb_dev_query(void);
int init_rex();


#endif

