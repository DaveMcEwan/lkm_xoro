
### Dave McEwan 2019-11-13

# xoro Linux Kernel Module

This is just a little example to demonstrate how to write a loadable LKM.
The module, called "xoro", provides a charater device `/dev/xoroshiro128p`
which can be read to get numbers from the "Xoroshiro128+".
Up to 8 bytes can be read at a time.

This does not implement all the best practices as recommeded by Vigna and
Blackman and should probably not be used.


## Viewing messages from the kernel module

The LKM can emit messages using the `printk` function which has a few
interfaces to userland.
View the most recent messages as they come in with `tail -f /var/log/kern.log`.
You probably want to do this before loading the LKM in a separate terminal.
Other ways of interfacing with the kernel log include using `dmesg` and
`/dev/kmsg`.


## User access to the character device

In order to access the character device as a user you need to add a rule to
udev with the included rules file.

    cp 99-xoro.rules /etc/udev/rules.d/
    udevadm control --reload


## Building, loading, and testing

Remove any existing objects and remove already loaded module.

    $ make clean
    # rmmod xoro

Then build kernel module with the provided test program, and load the module.

    $ make build
    # insmod xoro.ko

You should see messages with "XORO:" in the kernel log describing the
initialization process.
Additionally, you can check that the character device exists.

    $ ls -l /dev/xoroshiro128p
    cr--r--r-- 1 root root 239, 0 Nov 13 12:23 /dev/xoroshiro128p

Note that the `major_number` (239 in this example) will probably be different.
Then run test program which will open the character device, read some values,
and print information to STDOUT.

    $ ./test_xoro

You should see also more messages in the kernel log.


## Links

These are some of the resources used to write this module:

- <http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction>
- <http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/>
- <https://linux.die.net/lkmpg/x351.html>
- <https://www.kernel.org/doc/html/v4.15/kernel-hacking/hacking.html>
- <https://elinux.org/Debugging_by_printing>
- <http://prng.di.unimi.it/>


## TODO (maybe)

- Allow reading more than 8 bytes at a time.
- Provide multiple devices, each with a different algorithm.
  - xoroshiro128pp
  - xoroshiro128ss
  - xoroshiro256p
  - xoroshiro256pp
  - xoroshiro256ss
- Test output against known values.
- Command line arguments for seed.
- Initialize with SplitMix64.

