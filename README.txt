MSSE Embedded Systems
=====================

Setup (Linux/Ubuntu)
--------------------

Install Dependencies:

    $ sudo apt-get install gcc-avr avr-libc avrdude
    $ wget http://www.pololu.com/file/download/libpololu-avr-121115.zip
    $ unzip libpololu-avr-121115.zip
    $ cd libpolulu*
    $ make
    $ sudo make install

Run a simple app (assume plugged into USB):

    $ cd simple
    $ make
    $ make program

If you experience issues where it looks like you can see the device
but the write is failing for some reason, make the following change to
/etc/avrdude.conf:

    --- /etc/avrdude.conf.bak	2013-03-02 00:21:36.522309695 -0600
    +++ /etc/avrdude.conf		2013-03-02 00:21:55.426309718 -0600
    @@ -4754,31 +4754,31 @@
     #------------------------------------------------------------
     # ATmega1284P
     #------------------------------------------------------------

     # similar to ATmega164p

     part
         id               = "m1284p";
         desc             = "ATMEGA1284P";
         has_jtag         = yes;
         stk500_devcode   = 0x82; # no STK500v1 support, use the ATmega16
         one
         avr910_devcode   = 0x74;
         signature        = 0x1e 0x97 0x05;
         pagel            = 0xd7;
         bs2              = 0xa0;
    -    chip_erase_delay = 9000;
    +    chip_erase_delay = 55000;
         pgm_enable       = "1 0 1 0  1 1 0 0    0 1 0 1  0 0 1 1",
                            "x x x x  x x x x    x x x x  x x x x";

         chip_erase       = "1 0 1 0  1 1 0 0    1 0 0 x  x x x x",
                            "x x x x  x x x x    x x x x  x x x x";

         timeout	= 200;
         stabdelay	= 100;

