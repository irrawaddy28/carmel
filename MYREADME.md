How to install and run carmel?

Step 1: Make sure you have Boost library. If not, install.
> sudo apt-get install libboost-all-dev

Check if Boost installation was successful. On ifp-05, the default installation
directory was "/usr/lib/x86_64-linux-gnu" and the static and dynamically linked
library files are saved as libboost_*.a and libboost_*.so respectively. There were
36 such files. Hence, roughly the boost library files look like these:
>  ls -l /usr/lib/x86_64-linux-gnu/libboost_*
-rw-r--r-- 1 root root    2190 Jun 20  2014 /usr/lib/x86_64-linux-gnu/libboost_atomic.a
lrwxrwxrwx 1 root root      25 Jun 20  2014 /usr/lib/x86_64-linux-gnu/libboost_atomic.so -> libboost_atomic.so.1.54.0
-rw-r--r-- 1 root root    5864 Jun 20  2014 /usr/lib/x86_64-linux-gnu/libboost_atomic.so.1.54.0
-rw-r--r-- 1 root root  114234 Jun 20  2014 /usr/lib/x86_64-linux-gnu/libboost_chrono.a
lrwxrwxrwx 1 root root      25 Jun 20  2014 /usr/lib/x86_64-linux-gnu/libboost_chrono.so -> libboost_chrono.so.1.54.0
.
.
.


Step 2: Clone carmel
> cd ~
> git clone https://github.com/irrawaddy28/carmel  (forked from the original https://github.com/graehl/carmel.git)

This creates a directory called ~/carmel. Let's call this directory D.
> D=~/carmel

Step 3: Run make from $D/carmel
> cd $D/carmel
> ls -l
-rwxr-xr-x 1    544 Apr 11 21:47 carmel
drwxr-xr-x 2   4096 Apr 11 21:47 carmel-tutorial
-rwxr-xr-x 1  12217 Apr 11 21:47 ChangeLog
-rwxr-xr-x 1     62 Apr 11 21:47 debug.sh
drwxr-xr-x 3   4096 Apr 12 00:40 deps
drwxr-xr-x 3   4096 Apr 11 21:47 doc
-rw-r--r-- 1  37252 Apr 11 21:47 Doxyfile
-rw-r--r-- 1   9019 Apr 11 21:47 LICENSE
-rwxr-xr-x 1    885 Apr 11 21:47 make-dictionary.pl
-rwxr-xr-x 1   1968 Apr 11 21:47 Makefile
-rwxr-xr-x 1     51 Apr 11 21:47 NOTES
-rw-r--r-- 1  21749 Apr 11 21:47 README
drwxr-xr-x 7   4096 Apr 11 21:47 sample
drwxr-xr-x 3   4096 Apr 11 21:47 src
drwxr-xr-x 2   4096 Apr 11 21:47 test
-rw-r--r-- 1    934 Apr 11 21:47 ToDo
-rwxr-xr-x 1   7502 Apr 11 21:47 training-code.txt

> make -j 8 install BOOST_SUFFIX=  INSTALL_PREFIX=$D   (INSTALL_PREFIX is where the user wants to save the binaries)

After installation is successful, you should see three directories created:
a) $D/carmel/obj:   contains *.o files
b) $D/carmel/bin:   contains executable files "carmel" and "carmel.debug"
c) $D/bin: contains executable files "carmel" and "carmel.debug"
Note: The binaries in b) and c) are identical

Step 4: Test if carmel works by calling it w/o any argument
> $D/bin/carmel

usage: carmel [switches] [file1 file2 ... filen]

composes a sequence of weighted finite state transducers and writes the
result to the standard output.

-l (default)	left associative composition ((file1*file2) * file3 ... )
-r		right associative composition (file1 * (file2*file3) ... )
-s		the standard input is prepended to the sequence of files (for
.
.
.

