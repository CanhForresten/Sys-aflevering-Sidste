
# Exercise: Linux Device Driver 

*With this exercise you will explore how kernel modules work, what a linux character driver consists of, what file operations look like from a kernel perspective, how major/minor numbers a used to map device nodes with drivers and devices, how gpios are accessed inside the kernel and information transferred to/from user space and access and finally how blocking I/O and interrupts are implemented inside a kernel module*

---

## Focus

* Kernel Module and run-time installation of these
* Linux character driver boiler plate
* Major / minor numbers
* File operations
* GPIO access
* Blocking I/O
* Interrupt handling

---

## Preparation

* Your RPI and VSCode must be set up for remote development.
* You must have studied the relevant lecture material on **Linux Device Drivers**.
* You must download the Linux source tree to your Raspberry Pi and prepare it for building kernel modules as described in the following. **Updating and downloading Linux takes time, so do it at home!!**

### Download and prepare Linux source tree

Log into your Raspberry Pi and update it to the most recent Linux kernel (This will take a while, so do it at home!):

```bash
sudo apt update
sudo apt upgrade
sudo rpi-update
```

When it finishes, reboot your RPI

```bash
sudo reboot
```

Check the current Linux kernel version
```bash
uname -r
```
This will output something like:
```bash
6.12.56-v8-16k+
```

Go to your home folder and clone the Linux source tree without all Git history (~2 GB)

```bash
cd ~
git clone --depth=1 https://github.com/raspberrypi/linux
```

Source tree will be stored in *~/linux*

Install build dependencies:
```bash
sudo apt install bc bison flex libssl-dev make
```

Configure Linux kernel build system for Raspberry 5

```bash
cd linux
KERNEL=kernel_2712
make bcm2712_defconfig
```

(For Raspberry Pi 4 use `KERNEL=kernel8` and `make bcm2711_defconfig`)

Should you want to build the whole Linux kernel (**Not needed for our usage**), then you must type the following (takes +20 min)
```bash
make -j6 Image.gz modules dtbs
```

Otherwise you can just build the required header files (takes 1-2 minutes)
```bash
make modules_prepare
```

If we do not build the whole kernel, we'll have to download a special file (Module.symvers) that contains version information for modules available in the kernel. Download it.
```bash
wget https://raw.githubusercontent.com/raspberrypi/firmware/refs/heads/master/extra/Module_2712.symvers
```
And then
```bash
mv Module_2712.symvers Module.symvers
```

(For Raspberry Pi 4, `wget` [Module8.symvers](https://github.com/raspberrypi/firmware/raw/refs/heads/master/extra/Module8.symvers) and rename it to `Module.symvers`)

Now you are ready to build Linux kernel modules!

This guide was inspired by [this](https://www.raspberrypi.com/documentation/computers/linux_kernel.html)

---

## Activity 1: Your first kernel module

In the slides we saw an example of a very basic *hello world* kernel module:

```c
// FILE: hello.c - Hello world kernel module
#include <linux/module.h>

static int __init hello_init(void)
{
    printk(KERN_INFO "Hello, world\n");
    return 0;
}
static void __exit hello_exit(void)
{
    printk(KERN_INFO "Goodbye, cruel world\n");
}
module_init(hello_init);
module_exit(hello_exit);
MODULE_AUTHOR("Peter HM <phm@ece.au.dk>");
MODULE_LICENSE("GPL");
```

We also saw a Makefile:
```make
# FILE: Makefile (WITH CAPITAL M!!)
obj-m := hello.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

KERNELDIR ?= ~/linux

all default: modules
install: modules_install

modules modules_install help clean:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) $@
```

Before `$(MAKE)` there MUST be a TAB!

### Task 1: Build your first kernel module

Working with kernel modules and device drivers, it's a good idea to organize your projects with one **C** source file and one Makefile in a separate folder per project. E.g. (~/ is an alias for /home/au123456/)
```bash
~/linux
~/my_exercise_repo_folder/exercise12/hello/Makefile
~/my_exercise_repo_folder/exercise12/hello/hello.c
~/my_exercise_repo_folder/exercise12/led-driver/Makefile
~/my_exercise_repo_folder/exercise12/led-driver/led-driver.c
```

So, on your Raspberry Pi, create a new folder with a **C** source file, *hello.c*, containing the **C** code shown before and create a makefile, *Makefile* (capital 'M'), with the content the Makefile shown above.

In the folder containing *hello.c* and *Makefile*, build the kernel module
```bash
make
```
This should output something like this
```bash
make -C ~/linux M=/home/au123456/sw3sys/exercise12/hello modules
```
Plus a lot more. Observe the generated files, there should be a file, *hello.ko*, which is a kernel object (ko).
Let's check for which kernel version it is built
```bash
modinfo ./hello.ko
```
*(With `modinfo` you have to type `./` because it otherwise searches for modules in the standard folder for modules, which is */usr/lib/modules*)*

Is it the same kernel version as the for the running system (`uname -r`)?

If they are the same, you can now insert the module into the Linux kernel:
```bash
sudo insmod hello.ko
```
Let's check if it has been loaded:
```bash
lsmod
```
You should be able to find it in the list together with its size and number of refering modules (kernel modules can refer to other kernel modules).

**NOTE!** Kernel modules lives inside the kernel, which means that they live in **kernel space** (priviliged access) as opposed to **user space** (unpriviliged access), where applications (programs with a main function) live. User space has access to kernel space through systems call ONLY. That is, a user space application can request data from the kernel, which it finds and returns. A Driver cannot on its own send data to user space, it must be requested by someone to do so. A consequence is that instead of using `printf()` in kernel space to print something to the terminal in user space, we use `printk()`, with **k** as kernel, to "print" messages. These messages end up in the kernel logging system and not at the terminal directly!

To read the content of the kernel log do:
```bash
dmesg
```

This will dump all sorts of information, including all that happened during start-up of the system. At the tail of the log you should see something like:
```bash
[ 7330.901546] Hello, world
```

This is a timestamp followed by the text from `hello_init()` and indicates that the function has been called

Now, remove the module and check if the text from `hello_exit()` is printed to the kernel log (type commands one at a time to see what happens)
```bash
sudo rmmod hello.ko
dmesg
```
A text similar to `[ 7343.247017] Goodbye, cruel world` should appear. You can also check if the module has been unloaded by invoking `lsmod | grep hello` (use grep to search for lines with *hello*).

That's it! You have built, inserted and removed your first kernel module!!!

## Activity 2: Add file operations

So far, our kernel module can only write something to the kernel log, but has not actual functionality. You cannot use it for anything from user space.

We know the saying: *"In Linux, everything is files!"*

What it actually means, is that files are used as representation for data channels, just as we saw it with message passing between processes using named pipes. Remember:
```bash
mkfifo myfifo
cat myfifo &
echo 123 > myfifo
123
```
`myfifo` is a pipe, represented by a file. When `cat` is forked at reads from it, nothing happens until `echo` writes `123` to it. Both `cat` and `echo` can access files (read/write), so the pipe must have an interface that looks like a file!

We utilize the same functionality with device drivers. The interface towards user space is the *file operations* interface and the interface towards hardware is specific to the given platform. This means e.g. that a gpio driver implements the *file operations* interface, and adapts the logic towards the actual hardware platform. 

The user space applications benefit from this, since the unified interface makes your code able to run on multiple hardware platforms. **Applications only know about a file, e.g. /dev/mygpio12, not the underlying hardware**

We also know that Linux supports the Posix file operations:
* [int open(const char *path, int oflag, ... );](https://linux.die.net/man/3/open)
* [int close(int fildes);](https://linux.die.net/man/3/close)
* [ssize_t read(int fildes, void *buf, size_t nbyte);](https://linux.die.net/man/3/read)
* [ssize_t write(int fildes, const void *buf, size_t nbyte);](https://linux.die.net/man/3/write)
* [int ioctl(int fildes, int request, ... /* arg */);](https://linux.die.net/man/3/ioctl)

This is the *file operations* interface as seen from user space. Creating a driver, we must complete the kernel space implementation of this interface. 

A *button* driver should implement `open()`, `read()` and `close()`. To **open** the button device, **read** the status of the button and finally to **close** the button device.

An *led* driver should implement `open()`, `write()` and `close()`. To **open** the led device, **write** the led state and finally to **close** the led device.

### Task 1: Build a basic driver with file operations

Create a new folder and copy the *Makefile* to it. Create a new file *button.c*
Update your Makefile to build *button.o* instead of *hello.o*

Open *button.c* in an editor and add the following template code:
```c
// button.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>

#define DRIVER_NAME "mygpio" // Must start with "my"
#define DEVICE_NAME "but0"     // Name of specific device

static dev_t dev_num;
static struct cdev gpio_cdev;
static struct class *gpio_class;

static ssize_t gpio_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    char kbuf[16]; // Buffer allocated in kernel
    int value;

    // Set value
    value = 23;
    
    // Format string
    int n = snprintf(kbuf, sizeof(kbuf), "%d\n", value);

    // Copy string to user space
    if (copy_to_user(buf, kbuf, n))
        return -EFAULT;

    return n; // Return length of string
}

static ssize_t gpio_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    char kbuf[16];
    int value;

    // Return error if data written is bigger than kernel buffer
    if (len >= sizeof(kbuf-1))
        return -EINVAL;

    // Copy data from user space to kernel space
    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    // Append a zero-termination to ascii buffer (C-string)
    kbuf[len] = '\0';

    // Convert string to int value (aut base)
    if (kstrtoint(kbuf, 0, &value))
        return -EINVAL;

    // Use value for something...
    printk(KERN_INFO "Recieved %d from user_space\n", value);

    // Return that written length = requested write length => Everything ok
    return len;
}

// Define which file functions to use for file operations
static struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .read  = gpio_read,
    .write = gpio_write,
};

/* Button init
   This is boiler-plate code for a character driver.
   It allocates an available Major number and 
   reserves minor numbers 0-255.
   Creates a class and one device for minor number 0
*/
static int gpio_init(void)
{
    int ret;

    // Request an available Major number and Minor numbers [0..255]
    ret = alloc_chrdev_region(&dev_num, 0, 255, DRIVER_NAME);
    if (ret)
        return ret;

    // Initialize and add Character Driver that uses fileoperations in 
    cdev_init(&gpio_cdev, &gpio_fops);
    ret = cdev_add(&gpio_cdev, dev_num, 255);
    if (ret)
        goto unregister_region;

    // Create a class in /sys/class (needed for devices)
    gpio_class = class_create(DRIVER_NAME);
    if (IS_ERR(gpio_class)) {
        ret = PTR_ERR(gpio_class);
        goto del_cdev;
    }

    // Create a device: 
    // Major = the one allocated with `alloc_chrdev_region`
    // Minor = 0
    // Name = <DRIVER_NAME>-<DEVICE_NAME> eg /dev/mygpio-but0
    device_create(gpio_class, NULL, MKDEV(MAJOR(dev_num), MINOR(0)), NULL, "%s-%s", DRIVER_NAME, DEVICE_NAME);

    printk(KERN_INFO "%s: loaded (major=%d)\n", DRIVER_NAME, MAJOR(dev_num));
    return 0;

    device_destroy(gpio_class, dev_num);
    class_destroy(gpio_class);
del_cdev:
    cdev_del(&gpio_cdev);
unregister_region:
    unregister_chrdev_region(dev_num, 255);
    return ret;
}

static void gpio_exit(void)
{
    // Reverse-order cleanup!!!
    device_destroy(gpio_class, dev_num);
    device_destroy(gpio_class, dev_num+1);
    class_destroy(gpio_class);
    cdev_del(&gpio_cdev);
    unregister_chrdev_region(dev_num, 255);
    printk(KERN_INFO "%s: unloaded\n", DRIVER_NAME);
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter HM <phm@ece.au.dk>");
MODULE_DESCRIPTION("Minimal gpio char driver");
```

Note carefully, that the driver implements a `gpio_read()` and `gpio_write()` function, and that the function prototypes resemble the *Posix* ones. They implement the interface given in [`linux/fs.h`](https://elixir.bootlin.com/linux/v6.17.7/source/include/linux/fs.h#L2156). You MUST comply to this interface, you cannot make your own, as this is the only one supported by Linux.

To keep things simple we have not created `open` and `close` functions in the driver, instead we are relying on the default implementations given in the kernel. 

Build the driver with `make`. It should build with without warnings or errors and output a *button.ko* file.

### Task 2: Test the file operations

In the following we will be using the file node in */dev* generated by a user space service (deamon), *udev*, when the driver creates a device. For security reasons, the nodes in */dev* are root accessable only!

This is a little tiresome when learning, so we'll add a rule, such that when *udev* sees a new device with the preamble **my** in it's name, e.g. `mygpio`, it will make the [node accesable by all users](https://en.wikipedia.org/wiki/Chmod):
```bash
sudo sh -c 'echo "SUBSYSTEM==\"my*\", MODE=\"0666\"" > /etc/udev/rules.d/99-my.rules'
```

When done, load the module with `insmod`. 

Let's check if a new node has become available
```bash
ls -l /dev/my*
````
This should output something like:
```bash
crw-rw-rw- 1 root root 509, 0 Nov  7 12:43 /dev/mygpio-but0
```
Read it like this:
* 'c' - This is a character device
* 'rw-rw-rw-' - user/group/other all have read/write permissions, but no execute permission
* '1' - Number of links to this file
* 'root root' - Created and owned by root
* '509' - Major number
* '0' - Minor number
* 'Nov  7 12:43' - Date / Time
* '/dev/mygpio-but0' - Pathname of node file

So, as defined by the rule, it should be accissable by all, and it seems so. 

Let's try to read the content of the node with `cat`. Remember that `cat` *opens* the file, *reads* the content until the end, and *closes* the file:
```bash
cat /dev/mygpio-but0
```
This should output the value set inside... which function? `gpio_read()` or `gpio_write()`??
Note, that a device node file does not contain an *end-of-file* character like a text document, so `cat` will read forever. You can stop `cat` with `ctrl-c`.

Let's try to write something to the node using `echo`. Remember we can use `>` to redirect the output of echo to a file (or pipe or...). `echo` will *open* the file, *write* and *close* the file again.

```bash
echo 201 > /dev/mygpio-but0
```

*Where must we locate the result?* In kernel or user space? Which command must we use to see the result? Find out and see if we have written the value successfully!?

## Activity 3: Create a button driver

We now have the basic boiler-plate code in place to manage insertion/removal of the module and to support the *file operations* interface. 
Next, we have to implement the exciting part, connecting to actual hardware! Again, we keep things simple at this point, and use a button as example. The principles however extends to all sorts of hardware, in particular character devices that stream data in cronological order: keyboards, accelerometers, ADC, DAC, mice, motor drivers etc. (opposed to random-access devices like memory or packet-based devices like network adapters).

There exists a low-level gpio driver that we can rely on, you can find the interface in [linux/gpio/consumer.h](https://elixir.bootlin.com/linux/v6.17.7/source/include/linux/gpio/consumer.h). The functions from `linux/gpio/consumer.h` that we will use are:

Function to allocate gpios:
* [`struct gpio_desc *gpio_to_desc(unsigned gpio)`](https://elixir.bootlin.com/linux/v6.17.7/source/drivers/gpio/gpiolib.c#L165)

Functions for getting or setting gpio pin directions:
* [`int gpiod_get_direction(struct gpio_desc *desc)`](hhttps://elixir.bootlin.com/linux/v6.17.7/source/drivers/gpio/gpiolib.c#L424)
* [`int gpiod_direction_input(struct gpio_desc *desc)`](https://elixir.bootlin.com/linux/v6.17.7/source/drivers/gpio/gpiolib.c#L2809)
* [`int gpiod_direction_output(struct gpio_desc *desc, int value)`](https://elixir.bootlin.com/linux/v6.17.7/source/drivers/gpio/gpiolib.c#L2979)

Functions for getting or setting gpio pin values:
* [`int gpiod_get_value(const struct gpio_desc *desc)`](https://elixir.bootlin.com/linux/v6.17.7/source/drivers/gpio/gpiolib.c#L3480)
* [`int gpiod_set_value(struct gpio_desc *desc, int value)`](https://elixir.bootlin.com/linux/v6.17.7/source/drivers/gpio/gpiolib.c#L3858)

Data structure used to represent gpios (drivers/gpio/gpiolib.h):
* [`struct gpio_desc`](https://elixir.bootlin.com/linux/v6.17.7/source/drivers/gpio/gpiolib.h#L170)

### Task 1: Declare a button gpio descriptor

Currently we'll support a single button, so place an instance of `struct gpio_desc` in the global scope of `button.c`, next to the other static variable declarations:
```c
static struct gpio_desc *gpio_btn;
```

### Task 2: Allocate gpio and set direction

We have previously located which buttons use which GPIO numbers by looking at the [schematic for the ECE hat](https://gitlab.au.dk/platforms/rpi5/ecesyshat/-/blob/main/Documents/ECE-SYS-HAT_V2_DOC.pdf?ref_type=heads). These numbers, 12, 16, 20 are however only a subset of all the actual gpios on the chip. As we are now working in kernel space, we have access to much more than in user space, so we need to know the actual gpio number in the total gpio number span of the processer (more than 500!!). Luckily, we can translate from one to another.

Open the file `/sys/kernel/debug/gpio` with a text viewer or editor. Look for the section `gpiochip0` which is the port connected to the Raspberry Pi's expansion connection. You will see something like:

```bash
gpiochip0: GPIOs 569-622, parent: platform/1f000d0000.gpio, pinctrl-rp1:
 gpio-569 (ID_SDA              )
 gpio-570 (ID_SCL              )
 gpio-571 (GPIO2               )
```

(You can also search from the terminal: `grep GPIO2 /sys/kernel/debug/gpio`)

Look for the gpio number you desire, note the number following `gpio-`. E.g. `GPIO2` uses number `571`.

**TODO** Initialization must happen when the module is loaded, and you must therefore work in `gpio_init()` now.

Type your initialization code after the call to `device_create()`.

Call `gpio_to_desc()` to allocate the gpio line. You must check if the return value is non-zero, if it returns zero, the it has failed to allocate the gpio, as the line may be in use by something else. 

Then set the gpio direction appropriatly (in/out) by calling the right function.

### Task 3: Implement read

In `gpio_read` we already have some boiler-plate code that can take an integer value and send it to user space. What we need to provide, is input from a real device, *gpio*.

Find out where in `gpio_read` you must place a function that can retrieve the state of the button, so it can be send to user space. 

Look at the suggested functions at the beginning of this activity and choose the appropriate one to obtain the state of the gpio and insert it at the proper spot!

### Task 4: Build and test

Build with `make`, `rmmod` if nescessary, `insmod` the updated module and locate your device node with `ls /dev/my*` 

Read from the node with `cat` and see if the value changes as the button changes state.

If so, you have completed this activity :-)

## Activity 4: Add an LED device

In this activity we'll extend the driver to support an LED. It's a good idea to use Git so you can revert if things go haywire at some point.

### Task 1: Declare, allocate and set gpio direction

Just like in Activity 3, locate the gpio number for one of the leds, declare a new gpio, `gpio_led` in global scope and allocate it with `gpio_to_desc()` and set the correct direction.

### Task 2: Implement write

Again, similarly to Activity 3, in `gpio_write` you must locate where we must use the value coming from user space and call the correct gpio function to change the state of the LED.

### Task 3: Build and test

Like Activity 3 step 4, but this time you must try to switch the LED on/off by writing "0" or "1" to the node using `echo`.

### Task 4: Create a node per device

So far, we have only been using one node. Reading from it returns the state of the button, whereas writing it controls the LED. Logically, this does not very sensible. We'd like to have a node per device.

To manage this we'll use different nodes and minor numbers for the different devices.
* Button: minor = 0, node = mygpio-btn
* LED: minor = 1, node = mygpio-led

Binding minor number and node name is what happens in `device_create`:
```c
device_create(gpio_class, NULL, MKDEV(MAJOR(dev_num), MINOR(0)), NULL, "%s-%s", DRIVER_NAME, DEVICE_NAME);
```
Changing the value in `MINOR()` to 1 changes the minor number to 1. The last parameter with the string `"%s-%s", DRIVER_NAME, DEVICE_NAME` sets the node name. Using a different `DEVICE_NAME` for the led, will change the node name.

**TODO:** In `gpio_init()` add a call to `device_create()` but setting minor to 1 and setting `DEVICE_NAME` to `"led"`.

Build and test! You should now have two nodes in `/dev`.

### Task 5: Update read to use minor numbers

The read/write functions does not use the minor numbers so far. That means that if you read from the led node, you will get the state of the button, which is not the intention. Let's use the minor number to control this.

In `gpio_read()` before your call to `gpio_get_value()` add the following snippet:
```c
int minor = iminor(file->f_inode);
```

This way we can get information about which node (minor number) was the cause of `gpio_read()` being called. We can use this to switch on the minor number and read from the correct gpio:

```c
switch(minor) {
    case 0:
        value = gpio_..(button);
        break;
    case 1:
        value = gpio_get..(led);
        break;
    default:
        break;
```

Implement the correct code. Reading from the LED will reflect its state.

### Task 6: Update write to use minor numbers

Similarly, update the write funtion to use the minor number.

You can use `gpiod_get_direction()` to check if the corresponding gpio is an output.

Now you can control several devices using the same driver, nice!!

## Activity 5: Making read blocking

We have seen how an I/O bound process can block and be put into the schedulers wait queue, until unblocked, after which it becomes runnable again.

Our kernel module is doing work on behalf of the user space process, take for instance: 

```bash
cat /dev/mygpio
```

In this case `cat` *opens* and *reads* the node `/dev/mygpio` which invokes the corresponding `open()` and `read()` functions inside the driver with the matching major number. This means that the code we have written in `write()` is doing work on behalf of `cat`. The consequense is that we can control the process flow of `cat`. For instance, there is a function to make the calling process sleep (block): 
```c
// From linux/wait.h
wait_event_interruptible(wq_head, condition)	
``` 

It awaits that something happens on a wait queue, which is similar to a *condition variable*, and if so, it evaluates the predicate, `condition` and continues if the predicate is true. So, `wait_event_interruptible` is pretty much like `conditional_variable::wait`.

Notification happens using this function:
```c
// From linux/wait.h
wake_up(&wq_head)
```

Again, very similar to `conditional_variable::notify`, but note the *C* syntax, it takes the conditional, `wq_head`, as a parameter. With **C++** , `wq` would be a conditional object and notify a method that works on that object (`wq.notify()`).

Finally, to declare a wait_queue, you must use the following macro:
```c
DECLARE_WAIT_QUEUE_HEAD(wq);
```

So, to sum it up the overall structure looks like this:
```c
//... other includes..
#include <linux/wait.h>

#define DRIVER_NAME "mygpio" // Must start with "my"
#define DEVICE_NAME "but0"     // Name of specific device

static dev_t dev_num;
static struct cdev gpio_cdev;
static struct class *gpio_class;
static int flag;
static DECLARE_WAIT_QUEUE_HEAD(wq);

static ssize_t gpio_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    char kbuf[16]; // Buffer allocated in kernel
    int value;
    wait_event_interruptible(wq, flag==1); // Wait until woken up...	
    // If flag == 1 then continue...
}

// Somewhere in one of the other functions...
flag = 1;
wake_up(&wq); // Signal!!!

```

### Task 1: Make `read` blocking 

Copy your *gpio-driver.c* an *Makefile* to a new folder, before continuing.

Add a wait queue and add a call to `wait_event_interruptible` in read() as just described.

Add the wake-up part in your `write` function.

Build and insert your driver.

### Task 2: Test blocking `read` 

For this we need two terminals with each their ssh connection to the raspberry pi:
```bash
ssh au123456@au123456-rpi.local
```

Now, in *terminal 1* read from the node:
```bash
cat /dev/mygpio-but0
```
Nothing should happen, as `read()` has hit `wait_event_interruptable` which makes `cat` sleep.

In *terminal 2* write to the node:
```bash
echo 1 > /dev/mygpio-but0
```

Now, `wait_event_interruptable` should wake up, and if you set the flag before calling `wake_up`, then the reader should continue and to read the button value, copy it to user space and then fall a sleep again, because *cat* reads on and on until it meets End of File, which is never does.

Open a third terminal and add a reader to the led also. Now write again from terminal 2. What happens then? All device nodes that share major number will invoke the same code, so do we have a race condition? (Yes!) There are several options to resolve this, one is mutual exclusion. This is the interface to use Mutex' in the kernel:
```c
// From: linux/mutex.h
DEFINE_MUTEX(name);
void mutex_lock(struct mutex *lock);
void mutex_unlock(struct mutex *lock);
```
Note! Unlike `std::conditional_variable`, the `wait_queue` is not bound to a Mutex, which means that you need to unlock the Mutex yourself after the critical section. 

If you have the time, you can try adding a Mutex to resolve this. It is not the best solution though, since the lock will probably be higly contended...

## Activity 6: Using an interrupt to control read flow

As you may have realized by now, you cannot program an interrupt service routine in Linux user space. Since interrupt service routines run atomically on a CPU, a user space ISR can jepardize the whole system if it gets stuck. In kernel space, where user access is restricted, you have access to more ressources, including interrupts. Extreme care must be taken when dealing with interrupts inside the kernel, as it can stall the whole system, if for example an interrupt blocks, waiting for a ressoure. So, no `mutex_lock()`, `msleep()`, or access to ressources, that may block, e.g. allocating memory inside an interrupt handler! GPIO access and writing variables is considered safe.

To use interrupts in a driver you can use the following defined in `interrupt.h`

```c
// From: linux/interrupt.h
#define IRQF_TRIGGER_NONE	    0x00000000
#define IRQF_TRIGGER_RISING	    0x00000001
#define IRQF_TRIGGER_FALLING	0x00000002
#define IRQF_TRIGGER_HIGH	    0x00000004
#define IRQF_TRIGGER_LOW	    0x00000008

typedef irqreturn_t (*irq_handler_t)(int, void *);

static request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)

void *free_irq(unsigned int, void *);
```

And this helper function defined in `gpio/consumer.h`

```c
// From: linux/gpio/consumer.h
int gpiod_to_irq(const struct gpio_desc *desc);
```

`request_irq` is used for requesting an interrupt and `irq_handler_t` is a function pointer type and prototype for the interrupt handler function. 
The interrupt subsystem uses an internal numbering system to keep track of interrupt sources, called interrupt lines. These lines derive from the specific hardware and luckily we can use the function `gpiod_to_irq` to translate gpio numbers to interrupt line numbers.
The `IRQF_TRIGGER_` defines are used for setting which event/state on the interrupt line that causes the invocation of the interrupt handler.

Wrapping up, using an interrupt looks something like this:

```c
// ...
#include <linux/interrupt.h>

irqreturn_t my_isr(int, void *) {
    printk(KERN_ALERT "In ISR \n");
    return IRQ_HANDLED;
}

// ...

static int gpio_init(void)
{ 
    int ret = 0;
    // Boiler plate stuff

    // mygpio = gpio_to_desc...
    // gpiod_direction_input...

    ret = request_irq(gpiod_to_irq(mygpio), my_isr, IRQF_TRIGGER_FALLING,
                 "mygpio IRQ", NULL);
    return ret;
}

static void gpio_exit(void)
{
    // Reverse-order cleanup!!!
    free_irq(gpiod_to_irq(mygpio), NULL);

    // Boiler plate stuff
}
```

This code snippet requests an interrupt for the gpio (must be an input) and if a *falling* edge occurs on the gpio, then the ISR is called. The ISR prints a message, **In ISR**, to the kernel log and returns. The ISR must `return IRQ_HANDLED`, to notify the interrupt subsystem that the interrupt has been handled by this particular ISR. Several ISRs can subscribe to the same interrupt, and the interrupt subsystem will notify ALL subscribers, one-by-one, until one of the subscribers reply with *handled*. 

### Task 1: Add ISR and request interrupt

And an interrupt service routine to your code, like shown in the code snippet above. At the end of your `init()` function add a call to `request_irq()`. Use your button as input.

Build and insert your kernel module.

Use `dmesg` to view the kernel log. Pushing the button should print lines of text to the log.

Have a look inside `/proc/interrupts` with `cat`. Here you can see the interrupt line that your driver has requested and the number of events that has happened since power-on.

### Task 2: Make button read interrupt controlled

Your `read()` function is made blocking with `wait_event_interruptible()` and `wake_up()` is called inside `write()`. 

We'd like `read()` to block until there is a change on the button, so which function should we move setting the flag and calling `wake_up()` to?

Implement this, build and insert the module.

Try reading from the button node in `/dev`. This should block until you toggle the button.

You may end up getting several reads from a single button press, why do you think so?

**Done!!!** Coming to this point, you should have a basic understanding of how kernel modules, file operations, major/minor numbers, gpio access and interrupts works inside the kernel.