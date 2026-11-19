# Comparacion: arranque auxiliar y arranque desde disco

Auditor aegis-audit v1.1, sin cambios, con el parametro de IP experimental `192.168.56.111`.

| Control | ISO auxiliar 19:37 | GRUB en disco 20:20 |
| --- | --- | --- |
| CAT-IDN-01 | PASS | PASS |
| CAT-IDN-02 | PASS | PASS |
| CAT-PER-01 | PASS | PASS |
| CAT-PER-02 | PASS | PASS |
| CAT-SSH-01 | PASS | PASS |
| CAT-RED-01 | PASS | PASS |
| PROP-AEGIS-01 | PASS | PASS |
| PROP-AEGIS-02 | PASS | PASS |

Ambas ejecuciones: PASS=8, FAIL=0, WARN=0. El informe posterior se conserva byte por byte, SHA-256 `d8735b7e970ebeb0770c0e15b5f18962a8089e189e31a4a3d102fe91e8902645`.
