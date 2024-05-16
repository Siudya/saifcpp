# SAIF(Switching Activity Interchange Format) Parser Library

## requirements

Clang 17 or GCC 9   
Ubuntu 20.04

```bash
sudo apt install xmake
xmake install fmt boost jsoncpp argparse
```

## compilation & test
```bash
xmake
xmake run test <path_to_saif_file>
xmake run test <path_to_saif_file> -o <path_to_json_file>
```