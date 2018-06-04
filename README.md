# Rapfi

Rapfi is a Gomoku (Five in a Row) engine designed for the [Gomocup AI Tournament](http://gomocup.org/). Since Gomocup requires AI to communicate through the [Gomocup protocol](http://petr.lastovicka.sweb.cz/protocl2en.htm), in order to use it comfortably, you may need the compatible GUI called [Piskvork](https://github.com/wind23/piskvork_renju) which you can download [here](https://raw.githubusercontent.com/wind23/piskvork_renju/master/Release/piskvork_renju.zip). Partly documented in author's native language (Chinese).

As the code structure is not suitable for future development, this version of Rapfi will no longer be updated.

###Overview

+ Rapfi is written in C++, based on the alpha-beta algorithm with some enhancements.

+ This version of Rapfi only supports freestyle rules.
+ It has a relative strong playing strength. It ranks **4st** in Gomocup 2018, with about 300 elo gap between the top.

| Game Type | Elo  |  +   |  -   | games |
| :-------: | :--: | :--: | :--: | :---: |
| Freestyle | 2096 |  37  |  35  | 1344  |
| Fastgame  | 2090 |  47  |  44  | 1008  |



