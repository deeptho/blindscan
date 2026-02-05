The following info is incomplete

# Compiling on Fedora
Install prerequisites:
```sudo dnf install libconfig-devel mold```



# Compiling on Ubuntu
Info contributed by user Llew on satellites.co.uk


First update ubuntu:

```sudo apt update```

After that completes, type :

```sudo apt upgrade```

Wait to finish, now to add some utilities (you'll need root privileges,
so still in terminal, type sudo -s then your password). Then -

```sudo apt-get install patchutils libproc-processtable-perl mold```

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

Now install the blindscan drivers:

```mkdir blindscan_kernel
cd blindscan_kernel
git clone --depth=1 deeptho/linux_media -b deepthought ./media

read the README.md provided for more instructions and follow the instructions
```

(Notice we replace tbsdtv with deeptho in the second cloning line!)

Now reboot - in Terminal:

```sudo reboot```
