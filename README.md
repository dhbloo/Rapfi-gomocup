# Rapfi

Rapfi is a Gomoku (Five in a Row) engine designed for the [Gomocup AI Tournament](http://gomocup.org/). Since Gomocup requires AI to communicate through the [Gomocup protocol](http://petr.lastovicka.sweb.cz/protocl2en.htm), in order to use it comfortably, you may need the compatible GUI called [Piskvork](https://github.com/wind23/piskvork_renju) which you can download [here](https://raw.githubusercontent.com/wind23/piskvork_renju/master/Release/piskvork_renju.zip). Partly documented in author's native language (Chinese).

As the code structure is not suitable for future development, this version of Rapfi will no longer be updated.

### Overview

+ Rapfi is written in C++, based on the alpha-beta algorithm with enhancements.

+ This version of Rapfi only supports freestyle rules.
+ It has a relative strong playing strength. It ranks **4st** in Gomocup 2018, with about 300 elo gap between the top.

| Game Type | Elo  |  +   |  -   | games |
| :-------: | :--: | :--: | :--: | :---: |
| Freestyle | 2096 |  37  |  35  | 1344  |
| Fastgame  | 2090 |  47  |  44  | 1008  |



### Modify config

Rapfi has a built-in config, however, if you want to change the config, you need to re-generate a config file. Config file contains all the information needed, such as eval value, score value, pruning margin, etc. Since Rapfi has an enormous amount of evaluation values, it's nearly impossible for manual adjustment. But we can generate the full config file using a simplified eval file, which contains much less values.

In order to generate a config file, compile "EvalGen" project, then use this command:

```
EvalGen.exe basicEval.txt config
```

`basicEval.txt` contains all the basic eval settings, with its meaning written in Chinese. Put `config` in the same dictionary with Rapfi, then Rapfi will automatically load it when started.