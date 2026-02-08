# Neumo-blindscan

##  DVB-NIP IBC data carroussel 12227V @13.0E ##

This files in this carroussel can be received using neumo-dmx and a **recent** version of tsuck

* First tune to the relevant mux, e.g., using the positoner dialog of neumoDVB: 12226V, symbol rate 27500.
  You can also use `neumo-tune`. For example, the folloing requests to use the LNB connected to the 3rd input (`rf_in==2`)
  on a DVB-card and the adapter (`-a 8`) that is connected to it. The last line shows that the tuner is locked
  ```
    neumo-tune -a 8 --rf-in=2 -U 8 --frequency=12227000 --symbol-rate=27500 --pol=V

    adapter=8
    rf_in=2
    frontend=0
    freq=12227000
    pol=2
    pls_codes[5]={ 4202496, 2048, 98139136, 134216704, 80015360, }
    diseqc=UC: U=8 C=-1
    Name of card: TurboSight TBS 6916 (Hexa DVB-S/S2/S2X)
    Name of adapter: A8 TBS 6916
    Name of frontend: TurboSight TBS 6916 (Hexa DVB-S/S2/S2X)
    ==========================
   DEBUG main - Tuning to DVBS1/2 12227V
   DEBUG main - select rf_in=2
   DEBUG main - Succesfully set MASTER rf_input=2
   BLIND TUNE search-range=10000
	FE_GET_EVENT: stat=543, timedout=0 locked=1
	FE_READ_STATUS: stat=543, signal=1 carrier=1 viterbi=1 sync=1 timedout=0 locked=1

    ```

* Leave the tuning command running, but in another terminal, select the proper PID in the stream and pass
  it to tsdduck to list its content. Bellow we will save the service with service id 1772. This service id can be found
  in the service list in neumoDVB, as the `service_id`  for the service named DVB-NIP IBC. We use the same adapter (`-a 8`)
  that we tuned.  The stream produced by neumo-dmx is passed to `tsresync` which will find the start of the stream, and then
  to the `nip` plugin which will output a summary of the stream; the stream itself is dropped (not used any further).

```
    neumo-dmx  -a 8   |  tsresync | tsp -P nip  --service=17702    --log-fdt -O drop
    * nip: FDT instance: 52929, source: 10.31.30.11, destination: 224.0.23.14:3937, TSI: 0, 1 files, expires: 2026/02/09 16:02:02.000
      TOI: 52929, name: urn:dvb:metadata:cs:NativeIPMulticastTransportObjectTypeCS:2023:bootstrap, 3,185 bytes, type: application/xml+dvb-mabr-session-configuration
    * nip: FDT instance: 841550, source: 10.31.30.11, destination: 225.80.232.178:50074, TSI: 50074, 1 files, expires: 2026/02/09 16:01:58.000
    TOI: 53394, name: eutelsat/NIP/CG/xml/nip_cg_1.xml, 1,642,770 bytes, type: application/xml+dvb-nip-cg
    * nip: FDT instance: 52849, source: 10.31.30.11, destination: 225.80.232.176:50001, TSI: 9, 11 files, expires: 2026/02/09 15:22:26.000
    TOI: 52839, name: http://dvb.gw/eutelsat/corp/73_arryadia_k2tgcj0_480p/playlist.m3u8, 136 bytes, type: application/vnd.apple.mpegurl
    TOI: 52840, name: http://dvb.gw/eutelsat/bpk-tv/orp01-ten999-ch007/hls-sb/index.m3u8, 629 bytes, type: application/vnd.apple.mpegurl
    TOI: 52841, name: http://dvb.gw/eutelsat/ts_corp/73_arrabia_hthcj4p/playlist.m3u8, 444 bytes, type: application/vnd.apple.mpegurl
    TOI: 52842, name: http://dvb.gw/eutelsat/bpk-tv/orp01-ten999-ch008/hls-sb/index.m3u8, 629 bytes, type: application/vnd.apple.mpegurl
    TOI: 52843, name: urn:dvb:metadata:cs:MulticastTransportObjectTypeCS:2021:gateway-configuration, 19,942 bytes, type: application/xml+dvb-mabr-session-configuration
    TOI: 52844, name: http://dvb.gw/eutelsat/bpk-tv/orp01-ten999-ch009/dash-sb/index.mpd, 2,644 bytes, type: application/dash+xml
    TOI: 52845, name: http://dvb.gw/eutelsat/ts_corp/73_almaghribia_83tz85q/playlist.m3u8, 456 bytes, type: application/vnd.apple.mpegurl
    TOI: 52846, name: http://dvb.gw/eutelsat/bpk-tv/orp01-ten999-ch009/dash-sb/dash/orp01-ten999-ch009-audio_133600_eng=131600.dash, 637 bytes, type: audio/mp4
    TOI: 52847, name: http://dvb.gw/eutelsat/bpk-tv/orp01-ten999-ch009/dash-sb/dash/orp01-ten999-ch009-video=2600000.dash, 732 bytes, type: video/mp4
    TOI: 52848, name: http://dvb.gw/eutelsat/corp/73_aloula_w1dqfwm/playlist.m3u8, 139 bytes, type: application/vnd.apple.mpegurl
    TOI: 52849, name: http://dvb.gw/eutelsat/ts_corp/73_tamazight_tccybxt/playlist.m3u8, 450 bytes, type: application/vnd.apple.mpegurl
...
```
* The actual data can be extracted and saved to /tmp/xxx as follows: we first create an empty directory, then ask to
  save all data (`--save-dvb-gw /tmp/xxx`) while also showing some progress information (`--log-fdt`; can be omited)

```
    mkdir /tmp/xxx;
    neumo-dmx  -a 8   |  tsresync | tsp -P nip  --service=17702 --log-fdt --save-dvb-gw /tmp/xxx -O drop
    * nip: FDT instance: 52929, source: 10.31.30.11, destination: 224.0.23.14:3937, TSI: 0, 1 files, expires: 2026/02/09 16:07:58.000
    TOI: 52929, name: urn:dvb:metadata:cs:NativeIPMulticastTransportObjectTypeCS:2023:bootstrap, 3,185 bytes, type: application/xml+dvb-mabr-session-configuration
    * nip: FDT instance: 52849, source: 10.31.30.11, destination: 225.80.232.176:50001, TSI: 9, 11 files, expires: 2026/02/09 15:22:26.000
...
```

  Many files will be created. For example, some of the files are transport streams and can be played with mpv

  ![dvb-nip ibc screenshot](images/tamazight.jpg)
