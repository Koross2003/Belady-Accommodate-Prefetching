# 适应预取的Belady近似算法

***

## 项目构成

```
├─ChampSim                  //ChampSim模拟器
│  ├─example                
│  │  ├─harmony
│  │  ├─hawkeye
│  │  └─homony
│  ├─inc
│  ├─lib
│  ├─single_core_result
│  └─trace
├─harmony               //Harmony算法
├─hawkeye               //Hawkeye算法
├─homony                //Homony算法
└─Report                //实验报告
    └─pig
```

`hawkeye` 文件夹参考仓库[Belady-Cache-Replacement](https://github.com/hyerania/Belady-Cache-Replacement)和一个fork于ChampSim源仓库的仓库中中提供的[Hwakeye](https://github.com/brajskular/ChampSim/tree/master/replacement/hawkeye)(他可能fork于原仓库的某个分支，但我没找到)。实现了Jain&Lin在[Back to the Future: Leveraging Belady’s Algorithm for Improved Cache Replacement](https://www.cs.utexas.edu/~lin/papers/isca16.pdf)中提出的算法。

`harmony` 复现了Jain&Lin在[Rethinking Belady’s Algorithm to Accommodate Prefetching](https://www.cs.utexas.edu/~akanksha/isca18.pdf)中提出的算法。

`homony` 提供了一个新的解决方案。

## 模拟器

使用2nd Cache Replacement提供的ChampSim([下载链接](https://www.dropbox.com/s/o6ct9p7ekkxaoz4/ChampSim_CRC2_ver2.0.tar.gz?dl=1))

在ChampSim文件夹下使用以下指令
- 运行hawkeye：`make hawkeye`
- 运行harmony：`make harmony`
- 运行homony：`make homony`

运行结果保存于 `ChampSim/single_core_result` 文件夹下
