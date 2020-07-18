# blindscan
User space code for DVB blind scanning on tbs based DVB cards.
Currently supports stid135-based cards (tbs6909x and tbs6903x) and
stv091x based card (tbs 5927).

The code requires a patched kernel tree which is (will be) available
at https://github.com/deeptho/linux_media

The blind scan code scans all or some of the frequency spectrum
available on a DVB-S2 tuner. The scanned portion is the range
[start-freq, end-freq]. Unless -p is specified, both polarisations
will be scanned. The spectrum is canned in steps of step-freq. The default
value is fine for scanning high symbolrate transponders.

To scan low symbol rate transponders, set max-symbol-rate to a low value
and star-freq close to where they might be.

Diseqc switching signals are sent based on the diseqc command string. This
string controls the order in which diseqc commands are sent, e.g., "UC" means
send uncommitted command first, then committed command. "UCU" means that the uncommitted
command will be sent twice. The switch ports are specified by the --uncommitted
and --committed arguments

This blind scanner can also scan transponders which have "Physical Layer Scrambling" turned
on. When it discivers such a transponder it uses its built-in codes (for the multistreams on
5.0West). Additional mod_modes/mod_codes can be added using --pls-modes. For example
--pls-modes ROOT+12 GOLD+13
adds two codes to try.

It is also possible to specify a range of ROOT+x codes to test. This option is very slow but
will take around 90 minutes at most. Currently  this is not working on stid135.

Usage: src/blindscan [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -a,--adapter INT=0          Adapter number
  --frontend INT=0            Frontend number
  -s,--start-freq INT=10700000
                              Start of frequenc range to scan (kHz)
  -e,--end-freq INT=12750000  End of frequency range to scan (kHz)
  -S,--step-freq INT=6000     Frequency step (kHz)
  -M,--max-symbol-rate INT=45000
                              Maximal symbolrate (kHz)
  -R,--search-range INT=10000 search range (kHz)
  -p,--pol INT:value in {BOTH->3,H->1,V->2} OR {3,1,2}=3
                              Polarisation to scan
  --pls-modes TEXT=[] ...     PLS modes (ROOT, GOLD, COMBO) and code to scan, separated by +
  --start-pls-code INT=-1     Start of PLS code range to start (mode=ROOT!)
  --end-pls-code INT=-1       End of PLS code range to start (mode=ROOT!)
  -d,--diseqc TEXT=UC         diseqc command string (C: send committed command; U: send uncommitted command
  -U,--uncommitted INT=0      uncommitted switch number (lowest is 0)
  -C,--committed INT=0        committed switch number (lowest is 0)

