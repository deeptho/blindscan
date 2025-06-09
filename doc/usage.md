# Neumo-blindscan

## Usage: Command line arguments

### Usage neumo-blindscan

```
Usage: neumo-blindscan [OPTIONS] [L]

Positionals:
  L ENUM:value in {C->4,universal->1,wideband->2,wideband-uk->3} OR {4,1,2,3}=1
                              LNB Type

Options:
  -h,--help                   Print this help message and exit
  -c,--command ENUM:value in {blindscan->0,iq->2,spectrum->1} OR {0,2,1}=1
                              Command to execute
  --blindscan-method ENUM:value in {fft->2,sweep->1} OR {2,1}=2
                              Blindscan method
  --delsys ENUM:value in {ATSC->11,ATSCMH->12,AUTO->22,CMMB->14,DAB->15,DCII->21,DSS->4,DTMB->13,DVBC->1,DVBC2->19,DVBH->7,DVBS->5,DVBS2->6,DVBS2X->20,DVBT->3,DVBT2->16,ISDBC->10,ISDBS->9,ISDBT->8,TURBO->17} OR {11,12,22,14,15,21,4,13,1,19,7,5,6,20,3,16,10,9,8,17}=6
                              Delivery system
  --lnb-type ENUM:value in {C->4,universal->1,wideband->2,wideband-uk->3} OR {4,1,2,3}=1
                              LNB Type
  --spectrum-method ENUM:value in {fft->1,sweep->0} OR {1,0}=0
                              Spectrum method
  -a,--adapter INT=0          Adapter number
  -r,--rf-in INT=-1           RF input
  --frontend INT=0            Frontend number
  -s,--start-freq INT=-1      Start of frequency range to scan (kHz)
  -e,--end-freq INT=-1        End of frequency range to scan (kHz)
  -S,--step-freq INT=6000     Frequency step (kHz)
  -R,--spectral-resolution INT=2000
                              Spectral resolution (kHz)
  -F,--fft-size INT=256       FFT size
  -M,--max-symbol-rate INT=45000
                              Maximal symbolrate (kHz)
  --search-range INT=10000    Search range (kHz)
  -p,--pol INT:value in {BOTH->3,H->1,V->2} OR {3,1,2}=3
                              Polarization to scan
  --pls-modes TEXT=[] ...     PLS modes (ROOT, GOLD, COMBO) and code to scan, separated by +
  --start-pls-code INT=-1     Start of PLS code range to start (mode=ROOT!)
  --end-pls-code INT=-1       End of PLS code range to start (mode=ROOT!)
  -d,--diseqc TEXT=UC         DiSEqC command string (C: send committed command; U: send uncommitted command
  -U,--uncommitted INT=-1     Uncommitted switch number (lowest is 0)
  -C,--committed INT=-1       Committed switch number (lowest is 0)

```

DiSEqC switching signals are sent based on the DiSEqC command string. This
string controls the order in which DiSEqC commands are sent, e.g., "UC" means
send uncommitted command first, then committed command. "UCU" means that the uncommitted
command will be sent twice. The switch ports are specified by the --uncommitted
and --committed arguments


###  Usage neumo-dmx
Stream dvb data from a demux to standard output
Usage: DVB demux program [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -a,--adapter INT=0          Adapter number
  -d,--demux INT=0            Demux number
  --fe-stream                 directly address the frontend
  --stid-pid INT=-1           pid in which stid bbframes are embedded
  --stid-isi INT=-1           stid isi
  --t2mi-pid INT=-1           pid in+ which t2mi stream is embedded
  --t2mi-plp INT=-2           t2mi isi to extract
  --pid INT=[] ...            pid (omit for full transport stream)

In most cases, the only arguments you need are `--adapter` and `--pid` to select a specific
adapter and request it to output TS packets for a specific pid.  If `--pid` is not specificed,'
then the full transport stream is output to standard output.

Multi-streams are a special case. For all cards, a specific stream-id (ISI) can be specified using
neumo-tune. neumo-dmx will then simply use the transport stream for that ISI. For stid135 cards, neumo-tune
can be requested to output bbframes (using the option `--bb_frames`). In that case neumo-dmx can be requested
to output data from **any stream**  by specifying the desired ISI using the `--stid-isi` argument.

Another special case are T2MI streams, which are embedded in a specific PID of a transport stream. That transport
stream can itself also be part of a multi-stream. The `--t2mi-pid` argument selects the pid in which the T2MI stream
is embedded.

A third special case is that of non-transport streams (GCS or GSE data). The drivers output such streams
by embedding them in a transport stream with a specific pid, which is currently hard coded as 270 (but this could
change). The demux part of the driver is currently not aware of the GCS/GSE nature of the stream and treats it as a
regular transport stream, and then see corrupt packets. Moreover, by default it also
the GCS/GSE packets as a regular transport stream and starts parsing it, resulting in corrupt data.  To prevent this,
use the  `--fe-stream` argument.

For example, when tuned to a GSE stream, the following commands

``neumo-dmx  -a 8  --fe-stream``

or

``neumo-dmx  -a 8  --fe-stream --pid=270``

will produce a single-pid transport stream, containing only packets with pid 270. The packets contain the
bbframes. Such a stream can then be demuxed in user space, e.g., the following command extracts the bbframe
data:

``neumo-dmx  -a 8  --fe-stream --pid=270 | ~/bbframe-tools/pts2bbf 270``

Note that the `--pid=270` part is redundant as the stream can only ever contain pid 270 if the driver is indeed
tuned to a GSE stream. Also note that a future version of the drivers will allow directly outputting GSE/GCS data,
without parsing it internally. In that case, the pts2bbf part will no longer be needed.



###  Usage neumo-tune

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
  -p,--pol INT:value in {H->1,V->2} OR {1,2}=0
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
  -b,--bb_frames              Ask to outputput bbframes encapsulated in mpeg packets
  -U,--uncommitted INT=-1     Uncommitted switch number (lowest is 0)
  -C,--committed INT=-1       Committed switch number (lowest is 0)

```

### Usage stid135-blindscan

```
Usage: stid135-blindscan [OPTIONS]


Positionals:
  L ENUM:value in {C->4,universal->1,wideband->2,wideband-uk->3} OR {4,1,2,3}=1
                              LNB Type

Options:
  -h,--help                   Print this help message and exit
  -c,--command ENUM:value in {blindscan->0,iq->2,spectrum->1} OR {0,2,1}=1
                              Command to execute
  --blindscan-method ENUM:value in {fft->2,sweep->1} OR {2,1}=2
                              Blindscan method
  --delsys ENUM:value in {ATSC->11,ATSCMH->12,AUTO->22,CMMB->14,DAB->15,DCII->21,DSS->4,DTMB->13,DVBC->1,DVBC2->19,DVBH->7,DVBS->5,DVBS2->6,DVBS2X->20,DVBT->3,DVBT2->16,ISDBC->10,ISDBS->9,ISDBT->8,TURBO->17} OR {11,12,22,14,15,21,4,13,1,19,7,5,6,20,3,16,10,9,8,17}=6
                              Delivery system
  --lnb-type ENUM:value in {C->4,universal->1,wideband->2,wideband-uk->3} OR {4,1,2,3}=1
                              LNB Type
  --spectrum-method ENUM:value in {fft->1,sweep->0} OR {1,0}=1
                              Spectrum method
  -a,--adapter INT=[] ...     Adapter number
  -r,--rf-in INT=-1           RF input
  --frontend INT=0            Frontend number
  -s,--start-freq INT=-1      Start of frequency range to scan (kHz)
  -e,--end-freq INT=-1        End of frequency range to scan (kHz)
  -S,--step-freq INT=6000     Frequency step (kHz)
  --spectral-resolution INT=0 Spectral resolution (kHz)
  -F,--fft-size INT=512       FFT size
  -R,--search-range INT=10000 Search range (kHz)
  -p,--pol INT:value in {BOTH->3,H->1,V->2} OR {3,1,2}=3
                              Polarisation to scan
  --pls-modes TEXT=[] ...     PLS modes (ROOT, GOLD, COMBO) and code to scan, separated by +
  --start-pls-code INT=-1     Start of PLS code range to start (mode=ROOT!)
  --end-pls-code INT=-1       End of PLS code range to start (mode=ROOT!)
  -d,--diseqc TEXT=UC         DiSEqC command string (C: send committed command; U: send uncommitted command
  -U,--uncommitted INT=-1     Uncommitted switch number (lowest is 0)
  -C,--committed INT=-1       Committed switch number (lowest is 0)

```
