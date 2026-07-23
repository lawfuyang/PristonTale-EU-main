# Aging & Sheltom Requirements Reference

> Auto-generated from `Server/server/itemserver.cpp` (`iaSheltomAgingList`) and `GameDB.dbo.AgeList`
> Generated: 2026-07-22 00:21:20

## Sheltom Items

| Code | Name | ReqLvl |
| --- | --- | --- |
| os101 | Lucidy | 5 |
| os102 | Sereneo | 12 |
| os103 | Fadeo | 20 |
| os104 | Sparky | 30 |
| os105 | Raident | 40 |
| os106 | Transparo | 45 |
| os107 | Murky | 50 |
| os108 | Devine | 55 |
| os109 | Celesto | 60 |
| os110 | Mirage | 70 |
| os111 | Inferna | 80 |
| os112 | Enigma | 90 |
| os113 | Bellum | 100 |
| os114 | Oredo | 102 |
| os121 | Fury Sheltom | 120 |

## Sheltom Requirements by Aging Level

Source: `iaSheltomAgingList[20][12]` in `Server/server/itemserver.cpp`.
Each value is the sheltom tier suffix (3=os103 Fadeo, 4=os104 Sparky, etc). Each occurrence = 1 sheltom.

| Age Lv | Sheltoms Required | Breakdown |
| --- | --- | --- |
| 1 | 5 | 2x Fadeo (os103), 2x Sparky (os104), 1x Raident (os105) |
| 2 | 6 | 2x Fadeo (os103), 2x Sparky (os104), 2x Raident (os105) |
| 3 | 7 | 2x Fadeo (os103), 2x Sparky (os104), 2x Raident (os105), 1x Transparo (os106) |
| 4 | 8 | 2x Fadeo (os103), 2x Sparky (os104), 2x Raident (os105), 2x Transparo (os106) |
| 5 | 9 | 2x Fadeo (os103), 2x Sparky (os104), 2x Raident (os105), 2x Transparo (os106), 1x Murky (os107) |
| 6 | 10 | 2x Fadeo (os103), 2x Sparky (os104), 2x Raident (os105), 2x Transparo (os106), 2x Murky (os107) |
| 7 | 11 | 2x Fadeo (os103), 2x Sparky (os104), 2x Raident (os105), 2x Transparo (os106), 2x Murky (os107), 1x Devine (os108) |
| 8 | 12 | 2x Fadeo (os103), 2x Sparky (os104), 2x Raident (os105), 2x Transparo (os106), 2x Murky (os107), 2x Devine (os108) |
| 9 | 11 | 2x Sparky (os104), 2x Raident (os105), 2x Transparo (os106), 2x Murky (os107), 2x Devine (os108), 1x Celesto (os109) |
| 10 | 12 | 2x Sparky (os104), 2x Raident (os105), 2x Transparo (os106), 2x Murky (os107), 2x Devine (os108), 2x Celesto (os109) |
| 11 | 11 | 2x Raident (os105), 2x Transparo (os106), 2x Murky (os107), 2x Devine (os108), 2x Celesto (os109), 1x Mirage (os110) |
| 12 | 12 | 2x Raident (os105), 2x Transparo (os106), 2x Murky (os107), 2x Devine (os108), 2x Celesto (os109), 2x Mirage (os110) |
| 13 | 11 | 2x Transparo (os106), 2x Murky (os107), 2x Devine (os108), 2x Celesto (os109), 2x Mirage (os110), 1x Inferna (os111) |
| 14 | 12 | 2x Transparo (os106), 2x Murky (os107), 2x Devine (os108), 2x Celesto (os109), 2x Mirage (os110), 2x Inferna (os111) |
| 15 | 11 | 2x Murky (os107), 2x Devine (os108), 2x Celesto (os109), 2x Mirage (os110), 2x Inferna (os111), 1x Enigma (os112) |
| 16 | 12 | 2x Murky (os107), 2x Devine (os108), 2x Celesto (os109), 2x Mirage (os110), 2x Inferna (os111), 2x Enigma (os112) |
| 17 | 11 | 2x Devine (os108), 2x Celesto (os109), 2x Mirage (os110), 2x Inferna (os111), 2x Enigma (os112), 1x Bellum (os113) |
| 18 | 12 | 2x Devine (os108), 2x Celesto (os109), 2x Mirage (os110), 2x Inferna (os111), 2x Enigma (os112), 2x Bellum (os113) |
| 19 | 11 | 2x Celesto (os109), 2x Mirage (os110), 2x Inferna (os111), 2x Enigma (os112), 2x Bellum (os113), 1x Oredo (os114) |
| 20 | 12 | 2x Celesto (os109), 2x Mirage (os110), 2x Inferna (os111), 2x Enigma (os112), 2x Bellum (os113), 2x Oredo (os114) |

## Notes

- **Sheltom requirements** are per aging attempt and depend on the current aging level, not the item level.
- Aging levels 1-5 are **safe** (no downgrade or break).
- Aging level 20 is the maximum.
- The `ALWAYS_AGING_SUCCESS` server.ini cheat skips probability checks.
- Sheltoms are consumed regardless of success or failure.
- Source: `iaSheltomAgingList` in `Server/server/itemserver.cpp`, copied verbatim.

