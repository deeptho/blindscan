

# Installing on Ubuntu
Info contributed by user Llew on satellites.co.uk


First update ubuntu:

```sudo apt update```

After that completes, type :

```sudo apt upgrade```

Wait to finish, now to add some utilities (you'll need root privileges,
so still in terminal, type sudo -s then your password). Then -

```sudo apt-get install patchutils libproc-processtable-perl```

Then (maybe not necessary, but no big dea to add it):

```sudo apt-get install build-essential```


Check for cmake version. In Terminal, type

```cmake --version```

(should be greater than 3.12.0). If not installed, still in Terminal, type :

```sudo apt-get install cmake```

You may also need the 'make' build tool, so in Terminal type:

```sudo apt-get install make```

Same with gcc: check

```gcc --version```

(I think that's already installed in Ubuntu etc).

Next, in terminal

```sudo apt-get install git```

Now go to TBS driver installation - LinuxTVWiki


Scroll down to' Buidling TBS'Forked Driver' and follow these instructions in your Terminal -

```mkdir tbsdriver
cd tbsdriver
git clone tbsdtv/media_build
git clone --depth=1 deeptho/linux_media -b deepthought ./media
cd media_build
make dir DIR=../media
make allyesconfig
make -j4
sudo make install
```

(Notice we replace tbsdtv with deeptho in the second cloning line!)

Now reboot - in Terminal:

```sudo reboot```
