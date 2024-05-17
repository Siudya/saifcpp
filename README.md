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
xmake                                                         //compile 
xmake run saif2json path_to_saif_file                         //testing parsing speed
xmake run saif2json path_to_saif_file -o -                    //output json to stdout
xmake run saif2json path_to_saif_file -o path_to_json_file    //output json to file
```