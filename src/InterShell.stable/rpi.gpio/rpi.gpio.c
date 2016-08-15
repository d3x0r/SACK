#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE

#include <stdhdrs.h>
#include "../intershell_registry.h"
#include "../intershell_export.h"



//
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013
 
 
// Access from ARM Running Linux
 
#define BCM2708_PERI_BASE		  0x20000000
#define GPIO_BASE					 (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
 
#ifdef __LINUX__ 
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif 
#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

struct gpio_button
{
	int pin;
	PMENU_BUTTON button;
	LOGICAL on;
};

static struct grio_local
{
	int  mem_fd;
	void *gpio_map;

	// I/O access
	volatile unsigned *gpio;
	int n;
} gpio_local;

int output_pins[] = { 5,6,12,13,16,19,20,21,26 };

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio_local.gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio_local.gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio_local.gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
 
#define GPIO_SET *(gpio_local.gpio+7)  // sets	bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio_local.gpio+10) // clears bits which are 1 ignores bits which are 0
 
#define GET_GPIO(g) (*(gpio_local.gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH
 
#define GPIO_PULL *(gpio_local.gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio_local.gpio+38) // Pull up/pull down clock
 
 
int not_main(int argc, char **argv)
{
  int g,rep;
 
  // Set up gpi pointer for direct register access
 
  // Switch GPIO 7..11 to output mode
 
 /************************************************************************\
  * You are about to change the GPIO settings of your computer.			 *
  * Mess this up and it will stop working!										 *
  * It might be a good idea to 'sync' before running this program		  *
  * so at least you still have your code changes written to the SD-card! *
 \************************************************************************/
 
  // Set GPIO pins 7-11 to output
  for (g=0; g<=4; g++)
  {
	 INP_GPIO(output_pins[g]); // must use INP_GPIO before we can use OUT_GPIO
	 OUT_GPIO(output_pins[g]);
  }
 
  for (rep=0; rep<10; rep++)
  {
	  for (g=7; g<=11; g++)
	  {
		 GPIO_SET = 1<<output_pins[g];
		 //sleep(1);
	  }
	  for (g=7; g<=11; g++)
	  {
		 GPIO_CLR = 1<<output_pins[g];
		 //sleep(1);
	  }
  }
 
  return 0;
 
} // main
 
 
//
// Set up a memory regions to access GPIO
//
PRELOAD( setup_io )
{
#ifdef __LINUX__
	/* open /dev/mem */
	if ((gpio_local.mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		printf("can't open /dev/mem \n");
		exit(-1);
	}
 
	/* mmap GPIO */
	gpio_local.gpio_map = mmap(
		NULL,				 //Any adddress in our space will do
		BLOCK_SIZE,		 //Map length
		PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
		MAP_SHARED,		 //Shared with other processes
		gpio_local.mem_fd,			  //File to map
		GPIO_BASE			//Offset to GPIO peripheral
	);
 
	close(gpio_local.mem_fd); //No need to keep mem_fd open after mmap
 
	if (gpio_local.gpio_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)gpio_local.gpio_map);//errno also set!
		exit(-1);
	}
 
	// Always use volatile pointer!
	gpio_local.gpio = (volatile unsigned *)gpio_local.gpio_map;

	{
		int g;
		// Set available GPIO pins 0-3 to output
		for (g=0; g<=4; g++)
		{
			INP_GPIO(output_pins[g]); // must use INP_GPIO before we can use OUT_GPIO
			OUT_GPIO(output_pins[g]);
		}
	}
#endif
} // setup_io

static uintptr_t CPROC SetGPIOPin( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, pin );
	struct gpio_button *button = (struct gpio_button *)psv;
	button->pin = pin;
	return psv;
}

static void OnLoadControl( "rpi.gpio" )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, "pin=%i", SetGPIOPin );
}

static void OnSaveControl( WIDE("rpi.gpio") )( FILE *file, uintptr_t psv )
{
	struct gpio_button *button = (struct gpio_button *)psv;
	fprintf( file, "pin=%d\n", button->pin );
}

static void OnKeyPressEvent(  WIDE("rpi.gpio") )( uintptr_t psv )
{
	struct gpio_button *button = (struct gpio_button *)psv;
	button->on = !button->on;
	if( button->on )
	{
		GPIO_SET = 1 << output_pins[button->pin];
		InterShell_SetButtonHighlight( button->button, TRUE );
	}
	else
	{
		GPIO_CLR = 1 << output_pins[button->pin];
		InterShell_SetButtonHighlight( button->button, FALSE );
	}
}

static uintptr_t OnCreateMenuButton( WIDE("rpi.gpio") )( PMENU_BUTTON button )
{
	struct gpio_button *my_button = New( struct gpio_button );
	my_button->pin = gpio_local.n++;
	my_button->button = button;
	my_button->on = FALSE;
	return (uintptr_t)my_button;
}
