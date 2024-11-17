# esp_avb_simple_talker
Simple Talker for ESP32

Summary of the AVB talker (T), listener (L), controller (C) and bridge (B) interaction:
[Sequence of frame transmissions by each device type]

GPTP (sync with network clock):
TL: GPTP Announce
TL: GPTP Sync
TL: GPTP Follow Up
TL: GPTP Peer Delay Request
B: GPTP Peer Delay Response
B: GPTP Peer Delay Resopnse Follow Up

ATDECC (remote control):
TL: ATDECC ADP Entity Available (resend every ~10sec)
C: ATDECC AECP Read Entity Command
TL: ATDECC AECP Read Entity Response
C: ATDECC ECMP Connect RX Command (4.5sec timeout)
L: ATDECC ECMP Connect RX Response
L: ATDECC ECMP Connect TX Command (2sec timeout)
T: ATDECC ECMP Connect TX Response

MSRP (reserve bandwidth):
T: MVRP VLAN Identifier (resend every ~2sec)
L: MSRP Domain (resend every ~9sec)
T: MSRP Talker Advertise Leave (resend every ~3sec)
B: MSRP Talker Advertise Indicate
L: MVRP VLAN Identifier (resend every ~2sec)
L: MSRP Listener Ready (resend every ~3sec)

AVTP (media streaming):
T: AVTP AAF Stream
