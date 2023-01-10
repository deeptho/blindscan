# Neumo-blindscan

User space code for DVB blind-scanning, getting spectra and getting IQ constellation samples
on tbs based DVB cards. Currently supports:

* stid135-based cards: tbs6909x and tbs6903x
* stv091x based cards: tbs5927
* si2183 based cards: tbs6504
* tas2101 based cards (incomplete): tbs5990, tbs6904

The code requires a patched kernel tree which is available
at https://github.com/deeptho/linux_media
Always use the latest versions from both repositories, or make sure that the versions match
(e.g. the same tag or the same branch). Installation instructions for installing the drivers in Ubuntu can be found in [INSTALL.md](INSTALL.md)


# Spectrum Scan

`./neumo-blindscan  -c spectrum` will scan a satellite in less 30 seconds with a resolution of 2MHz on tbs5927/
The spectrum will be saved to `/tmp/spectrumH.dat` and `/tmp/spectrumV.dat` by default, but this can be changed.

There are two methods of spectrum scan: `sweep` and `fft`. `sweep` sweeps across the frequency spectrum and measures
narrow band power, taking into account AGC settings. `fft` uses the builtin fft engine on the stid135 chip to scan
the spectrum in very high resolution (100kHz per default, but 50kHz sometimes detects more narrow band transponders).

The spectrum will be saved to `/tmp/spectrumH.dat` and `/tmp/spectrumV.dat` by default, but the name
can be changed.

## Sweep scan
`./neumo-blindscan -c spectrum  -U3 -pH -a0 --spectrum-method sweep --spectral-resolution 500`
will scan the horizontal polarization.

## FFT Spectrum Scan

`./neumo-blindscan -c spectrum  -U3 -pH -a0 --spectrum-method fft --spectral-resolution 100` will scan
the full Ku band in one polarization in about 20 seconds with a resolution of 100kHz on tbs6909x and
tbs6903x. This is using the internal high speed fft engine.

Here are some example spectra for 5.0West; horizontal and vertical.

![example spectrum 5.0W Horizontal](doc/images/5.0WH.png)

![example spectrum 5.0W Vertical](doc/images/5.0WV.png)


Here is a portion of a 100kHz spectrum captured on 5.0West, horizontal polarization created on tbs6909x
It shows several narrow band transponders

![example 100kHz resolution fft spectrum 5.0W Horizontal](doc/images/5.0W_100kHz.png)



# Blindscan
The blind scan code scans all or some of the frequency spectrum
available on a DVB-S2 tuner. The scanned portion is the range
[start-freq, end-freq]. Unless `-p` is specified, both polarizations
will be scanned. There are two ways of scanning: `exhaustive` and `spectral-peaks`.

This blind scanner can also scan transponders which have "Physical Layer Scrambling" turned
on. When it discovers such a transponder it uses its built-in codes (for the multistreams on
5.0West). Additional mod_modes/mod_codes can be added using --pls-modes. For example `--pls-modes ROOT+12 GOLD+13`
adds two codes to try.

It is also possible to specify a range of ROOT+x codes to test. This option is very slow but
will take around 90 minutes at most. Currently  this is not working on stid135.

Blindscan results are saved in `/tmp/blindscanH.dat` and  `/tmp/blindscanV.dat` by default. Currently
only the frequencies are logged. More detailed information is produced in the output printed on the
screen. For example:

````
RESULT: freq=11178.144V Symrate=29996 Stream=12    pls_mode= 0:16416 ISI list: 4 13 5 12
SIG=-41.7dB SIG= 85% CNR=17.30dB CNR= 86% SYS(21) 8PSK FEC_3_5  INV_OFF PIL_ON  ROL_AUTO
````

## Exhaustive scan
The spectrum is scanned in steps of step-freq. The default
value is fine for scanning high symbolrate transponders.
To scan low symbol rate transponders, set max-symbol-rate to a low value
and star-freq close to where they might be.

## Spectral peaks scan
First the spectrum is analyzed to find candidate transponders and then a blindscan is performed
on those transponders. If the transponder locks, the output is saved.

On stv091x, the code searches for falling or rising edges of transponders. This is less reliable
than the stid135 algorithm (see below). After a blindscan succeeds, the spectrum part for that transponder
is skipped to avoid double detections and to save time. The main search parameters are

* max-symbol-rate: determines how far to scan from an edge in the spectrum. Set higher for high symbol rates. High values
are usually recommended.


On stid135, the code searches for central frequencies of transponders after acquiring the complete spectrum.
The spectrum is of high resolution and the peak detector is more sophisticated. As a result most spectral peaks are
found, even very weak or very narrow band ones. The code scans all candidate peaks and skips the spectrum part
for any found transponder. The main search parameters are:
* search-range: determines how far to search from the center of frequency peaks. A typical vale is 10000
* spectral-resolution: determines how finely to scan the spectrum. Use 50 for very low symbolrates. Otherwise use 100.

The stid135 algorithm's peak finding is more reliable. On the other hand, the actual blindscan is slower for
and less reliable for low symbol rates compared to stv091x. Also scanning false peaks is especially slow and
this makes scanning slower than it could be.
With the current code, scanning a full satellite (H and V; 10700Ghz-12750Ghz) takes about 6 minutes.

Below is an example for scanning 5.0W with default parameters.

```
time ./neumo-blindscan -c blindscan  -a0 -U2 --blindscan-method spectral-peaks   -F 512  --spectral-resolution 100
```

The most influential parameters are `spectral-resolution` (100kHz is fine, 50kHz is sometimes better), and
`search-range` (default: 10Mhz, higher is sometimes better).  `spectral-resolution` determines the accuracy
of the spectrum. Candidate peaks may be lost if the value is too high. `search-range` determines how far from the
candidate peak the blindscan can search for a lock. To find high symbolrate transponders (30Ms/s and more), a
larger value will find more transponders if the peak-finder does not find the center of the transponder, but
rather a value closer to its edges.


Here is the example:
![example 100kHz resolution fft scan 5.0W Horizontal](doc/images/tbs6909x_5.0wH_100kHz.png)

The green pluses indicate the found transponders. The orange ones are the candidate peaks. Several
narrow band transponders are found properly (11456H, 2400 kS/s and 11480H, 3124kS/s), but three transponders with very low symbolrate are missed (11458H, 542 kS/s and 11465H, 668kS). 11465H, 668kS can be found sometimes with a spectral
resolution setting o5 50kHz and can be tuned to reliably. The other one does not tune, even on stv091x on Linux,
even though it can be done on Windows. So the drivers need some more improvement.


# Obtaining constellation samples

`./neumo-tune -a 8 -c iq -f 11138000 -pV  -n 8000` will connect to adapter 8, tune to 11138V  and otain
8000 IQ samples. The samples will be saved in `/tmp/iqV.dat`
Currently this requires an active DVB signal. This will not work for inactive transponders or continuous stream transponders.

# Blindscan tuning a single transponder

`./neumo-tune -a 8 -c tune -f 11138000 -pV  -n 8000` will connect to adapter 8, tune to 11138V  and then wait forever.
Currently this requires an active DVB signal. This will not work for inactive transponders or continuous stream transponders.


# Usage

````
Usage: ./neumo-blindscan [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -a,--adapter INT=0          Adapter number
  --frontend INT=0            Frontend number
  -s,--start-freq INT=10700000
                              Start of frequency range to scan (kHz)
  -e,--end-freq INT=12750000  End of frequency range to scan (kHz)
  -S,--step-freq INT=6000     Frequency step (kHz)
  -M,--max-symbol-rate INT=45000
                              Maximal symbolrate (kHz)
  -R,--search-range INT=10000 search range (kHz)
  -p,--pol INT:value in {BOTH->3,H->1,V->2} OR {3,1,2}=3
                              Polarization to scan
  --pls-modes TEXT=[] ...     PLS modes (ROOT, GOLD, COMBO) and code to scan, separated by +
  --start-pls-code INT=-1     Start of PLS code range to start (mode=ROOT!)
  --end-pls-code INT=-1       End of PLS code range to start (mode=ROOT!)
  -d,--diseqc TEXT=UC         DiSEqC command string (C: send committed command; U: send uncommitted command
  -U,--uncommitted INT=0      uncommitted switch number (lowest is 0)
  -C,--committed INT=0        committed switch number (lowest is 0)
````

DiSEqC switching signals are sent based on the DiSEqC command string. This
string controls the order in which DiSEqC commands are sent, e.g., "UC" means
send uncommitted command first, then committed command. "UCU" means that the uncommitted
command will be sent twice. The switch ports are specified by the --uncommitted
and --committed arguments



```
Usage: ./neumo-tune [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -c,--command ENUM:value in {iq->1,tune->0} OR {1,0}=0
                              Command to execute
  -A,--algo ENUM:value in {blind->0,cold->2,warm->1} OR {0,2,1}=0
                              Algorithm for tuning
  -a,--adapter INT=0          Adapter number
  -r,--rf-in INT=-1           RF input
  --frontend INT=0            Frontend number
  -S,--symbol-rate INT=-1     Symbolrate (kHz)
  -m,--modulation ENUM:value in {APSK_1024->30,APSK_128->22,APSK_128L->28,APSK_16->10,APSK_16L->25,APSK_256->23,APSK_256L->29,APSK_32->11,APSK_32L->26,APSK_64->21,APSK_64L->27,APSK_8L->24,C_OQPSK->17,C_QPSK->14,DQPSK->12,I_QPSK->15,PSK_8->9,QAM_1024->19,QAM_128->4,QAM_16->1,QAM_256->5,QAM_32->2,QAM_4096->20,QAM_4_NR->13,QAM_512->18,QAM_64->3,QAM_AUTO->6,QPSK->0,Q_QPSK->16,VSB_16->8,VSB_8->7} OR {30,22,28,10,25,23,29,11,26,21,27,24,17,14,12,15,9,19,4,1,5,2,20,13,18,3,6,0,16,8,7}=9
                              modulation
  --delsys ENUM:value in {ATSC->11,ATSCMH->12,AUTO->22,CMMB->14,DAB->15,DCII->21,DSS->4,DTMB->13,DVBC->1,DVBC2->19,DVBH->7,DVBS->5,DVBS2->6,DVBS2X->20,DVBT->3,DVBT2->16,ISDBC->10,ISDBS->9,ISDBT->8,TURBO->17} OR {11,12,22,14,15,21,4,13,1,19,7,5,6,20,3,16,10,9,8,17}=6
                              Delivery system
  -R,--search-range INT=10000 Search range (kHz)
  -p,--pol INT:value in {BOTH->3,H->1,V->2} OR {3,1,2}=3
                              Polarization to scan
  -n,--num-samples INT=1024   Number of IQ samples to fetch
  -f,--frequency INT=-1       Frequency to tune for getting IQ samples
  -s,--stream-id INT=-1       stream_id to select
  --pls-code TEXT=[] ...      PLS mode (ROOT, GOLD, COMBO) and code (number) to scan, separated by +
  --start-pls-code INT=-1     Start of PLS code range to start (mode=ROOT!)
  --end-pls-code INT=-1       End of PLS code range to start (mode=ROOT!)
  -T,--pls-search-timeout INT=25
                              Search range timeout
  -d,--diseqc TEXT=UC         DiSEqC command string (C: send committed command; U: send uncommitted command
  -U,--uncommitted INT=-1     Uncommitted switch number (lowest is 0)
  -C,--committed INT=-1       Committed switch number (lowest is 0)


```



## Compiling
The code is compiled with clang++. Make sure it is installed as some versions of g++ have some
problems compiling this

````
cd <whereever you have checkout>
mkdir build
cd build
cmake ..
make
````

The executable will be located in `build/src/blindscan` and can be copied to `/usr/bin/`
