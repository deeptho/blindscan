# Neumo-blindscan

##  DAB streams using neumo-blindscan and eti-tools ##

The following provides concrete examples on how to listen to specific DA streams,
using neumo-blindscan and eti-tools. In all cases, two commands are needed:

* The first command tunes to the relevant mux: `neumo-tune`. As an alternative,
  you can also simply use neumoDVB. You will then need to find out the adapter
  number that neumoDVB uses. Typically this can be found with

  ``ls -al /proc/`pidof -s neumodvb`/fd|grep adapter``

  which will show some output like

  ``
  lrwx------ 1 xxx xxx 64 Jun  9 00:43 45 -> /dev/dvb/adapter8/frontend0
  lrwx------ 1 xxx xxx 64 Jun  9 00:43 57 -> /dev/dvb/adapter8/demux0
  lrwx------ 1 xxx xxx 64 Jun  9 00:43 79 -> /dev/dvb/adapter8/demux0
  ``
* The second command is `neumo-dmx`. It will output the stream and this output will be typically sent
  to other commands for further processing.  In many cases, dvbsnoop can be used as an alternative:

  ``dvbsnoop -adapter 8 -s ts -tsraw -b``

Note that in the examples, you will need to change  the parameters for DiSEqC in the `neumo-tune` command.
For example `-dU -U2` means "send uncommitted commands only ", with the uncommitted command
selecting port 2.

Also you will need to select a specific adapter (-a) and rf input (-r) for tuning and demuxing (the same in both cases), i.e., an
adapter that can connect to the cable that can reach the LNB.

You may also need small changes depending on where bbframe-tools and eti-tools commands are installed on
your computer. Sometimes the commands may not work as the eti-tools are a bit buggy. Then retry the demux
command a  few times. Also check that neumo-tune's output shows that the mux has indeed been locked.

### NRK 10716V@0.8W

* Tune:

    ``neumo-tune -ctune -A blind -a 8 -r 0 -dU -U2 -s 171 -f 10716000 -pV  -S 5400``

* demux:

    ``neumo-dmx  -a 8  --fe-stream --pid=270  | ~/bbframe-tools/pts2bbf 270 | ~/eti-tools/bbfedi2eti -dst-ip 239.199.2.1 -dst-port 1234 | ~/dablin/build/src/dablin_gtk``


You can also use other values of  `-dst-ip`, i.e., 239.199.2.X, where X is in the range 1...8.

### ERT 12241H@39.0E

* Tune:

   ``neumo-tune -ctune -A blind -a 8 -r 0 -dU -U14  -f 12241336 -pH  -S 13382``

* demux:
  ``neumo-dmx  -a 8 --pid=1010  | ~/tsniv2ni/tsniv2ni 1010 | ~/dablin/build/src/dablin_gtk 2>/dev/null``


### DAB Italia 11727V@9.0E

* Tune:

    ``neumo-tune -ctune -A blind -a 8 -r 0 -dU -U5  -f 11727132 -pV  -S 30000``

* demux:

  ``neumo-dmx  -a 8 --pid=7031  | ~/tsniv2ni/tsniv2ni 7031 | ~/dablin/build/src/dablin_gtk 2>/dev/null``

  For the second mux:

  ``neumo-dmx  -a 8 --pid=7131  | ~/tsniv2ni/tsniv2ni 7131 | ~/dablin/build/src/dablin_gtk 2>/dev/null``



### BBC 11426H@28.2E

* Tune:

    ``neumo-tune -ctune -A blind -a 8 -r 0 -dCUCU -C3 -U9  -f 11426500 -pH  -S 27647``

* demux:

  ``neumo-dmx  -a 8 --pid=1061  | ~/eti-tools/ts2na --pid 1061 | ~/eti-tools/na2ni | ~/dablin/build/src/dablin_gtk 2>/dev/null``

You can also choose other values for the pid value in the range 1061...1065


### SWR 12568V@7.0E

* Tune:

  ``neumo-tune -ctune -A blind -a 8 -r 0 -dCUCU -C2 -U10  -f 12568443 -pV  -S 17017``

* demux:

  ``neumo-dmx  -a 8 --pid=101  | ~/eti-tools/fedi2eti 101 239.132.1.50 5004 | ~/dablin/build/src/dablin_gtk``

You can also use other values for the ip. See https://github.com/piratfm/eti-tools for a long list.



### WDR 11603V@19.0E

* Tune:

    ``neumo-tune -ctune -A blind -a 8 -r 0 -dCUCU -C0 -U10  -f 11603750 -pV  -S 2200``

* demux:

  ``neumo-dmx  -a 8 --pid=3000   | ~/eti-tools/fedi2eti 3000 228.10.1.5 10010 | ~/dablin/build/src/dablin_gtk``

  For the second mux:

  ``neumo-dmx  -a 8 --pid=3000   | ~/eti-tools/fedi2eti 3000 228.10.2.5 10010 | ~/dablin/build/src/dablin_gtk``


### Bundesmux 12168V@23.5E

* Tune:

    ``neumo-tune -ctune -A blind -a 8 -r 0 -dCUCU -C1 -U9  -f 12168000 -pV  -S 27500``

* demux:

   ``neumo-dmx  -a 8 --pid=4121 | ~/eti-tools/fedi2eti 4121 239.128.43.43 50043 | ~/dablin/build/src/dablin_gtk``


  For the second mux:
  
    ``neumo-dmx  -a 8 --pid=4122 | ~/eti-tools/fedi2eti 4122 239.128.72.10 50010 | ~/dablin/build/src/dablin_gtk``

### Metropolitain 11461H@5.0W

* Tune:

  ``neumo-tune -ctune -A blind -a 8 -r 0 -dU -U0  -f 11461000 -pH  -S 5780``

* demux:

  ``neumo-dmx -a 8 --pid=301 | ~/eti-tools/fedi2eti  301 239.0.1.11 5001 | ~/dablin/build/src/dablin_gtk``


  For the second mux:

  ``neumo-dmx -a 8 --pid=301 | ~/eti-tools/fedi2eti  301 239.0.1.12 5002 | ~/dablin/build/src/dablin_gtk``



### Salisbury 11221V@10.0E

* Tune:

    ``neumo-tune -ctune -A blind -a 8 -r 1 -dU -U3  -f 11220830 -pV  -S 30000``

* demux:

      ``neumo-dmx -a 8 --pid=701 | ~/eti-tools/fedi2eti  701 239.232.1.201 2048 | ~/dablin/build/src/dablin_gtk``
