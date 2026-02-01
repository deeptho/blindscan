

# Neumo-blindscan

User space code for DVB blind-scanning, getting spectra and getting IQ constellation samples
on tbs based DVB cards. Currently supports:

* stid135-based cards: tbs6909x, tbs6903x, tbs6916
* stv091x based cards: tbs5927
* si2183 based cards: tbs6504
* tas2101 based cards (incomplete): tbs5990, tbs6904
* m88rs6060 based cards: tbs6904se

The code requires a patched kernel tree which is available
at https://github.com/deeptho/linux_media
Always use the latest versions from both repositories, or make sure that the versions match
(e.g. the same tag or the same branch). Installation instructions for installing the drivers in Ubuntu can be found in [INSTALL.md](INSTALL.md)

#Important

This code requires specific versions of neumo drivers to be loaded
* Versions up to and including release-1.3 use older neumo drivers, which can be found in
**  https://github.com/deeptho/neumo_media_build
** https://github.com/deeptho/linux_media
* Versions above release-1.3 use newer neumo drivers, which can be found in




Specifically some features require that the kernel modules support at least neumodvb api 1.6.

## [Changes](doc/changes.md) ##

## [Usage: command-line arguments](doc/usage.md) ##

## [Tuning, spectrum scan and blindscan](doc/tuning_scanning.md) ##

## [DAB streams using neumo-blindscan and eti-tools](doc/dab.md) ##


## Compiling
The code is compiled with clang++. Make sure it is installed as some versions of g++ have some
problems compiling this

```
cd <whereever you have checkout>
mkdir build
cd build
cmake ..
make
```

The executable will be located in `build/src/blindscan` and can be copied to `/usr/bin/`
