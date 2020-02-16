# PowerGraph部署过程

## 1. 整体部署说明

  由于作者不再对github上的GraphPower项目进行维护，故安装部署将遇到很多问题。

  在部署PowerGraph前，建议将要部署的主机系统设置为不高于Ubuntu16.04或CentOS7的系统版本。经过笔者测试，在这两个系统上按照此教程部署不会遇到太大问题，而在Ubuntu18.04和CentOS8上测试均失败。笔者猜测可能的原因是*Linux kernel*版本过新造成了不兼容，可以通过手动配置的方式解决，但笔者未尝试过，不保证会不会出现新的问题。

  本文主要是Ubuntu 16.04环境下的PowerGraph部署过程，并列举了在该环境下可能出现的各种问题以及解决方案。

## 2. 部署过程

### 2.1 环境说明

- Ubuntu 16.04


- gcc 5.4

- g++ 5.4
- jdk 1.8
- build-essential

- Zlib


### 2.2 下载并编译PowerGraph

#### 2.2.1 首先更新一下源，保证获取的安装包都是最新的

```bash
sudo apt-get update
```

**注意：**在Ubuntu 16.04环境下软件源默认最新的软件版本为环境说明中所列举版本，而在Ubuntu 18.04及CentOS 8环境下软件源默认最新的软件版本gcc为7.4,jdk为11。gcc版本过高会导致在编译过程出错，使得源代码中可以在低版本gcc中编译通过的部分代码在高版本gcc中编译不通过，而jdk版本过高会导致下载的低版本hadoop包编译出错。

#### 2.2.2 为主机安装依赖

```
sudo apt-get install gcc g++ build-essential libopenmpi-dev openmpi-bin default-jdk cmake zlib1g-dev git
```


#### 2.2.3 从GitHub上下载PowerGraph

```bash
git clone https://github.com/graphlab-code/graphlab.git
```

#### 2.2.4 编译

```bash
cd graphlab
./configure
```

```bash
cd release/toolkits/graph_analytics
make -j 3
```

  至此为止的过程为PowerGraph官方推荐的单机编译的标准过程，不出意外的话应该会出现错误提示，请检查自身环境并根据错误提示以及下文可能的错误中给出的解决方案做出解决。

  如果有其他错误请在下方评论回复，笔者会尽力尝试解决。

#### 2.2.5 编译成功

  将错误解决后，经过一段时间编译，编译成功。

#### 2.2.6 集群中部署PowerGraph

  集群中部署PowerGraph可参考文章：https://www.jianshu.com/p/af02bb459ff4。

## 3. 可能的错误

### 3.1 make编译下载压缩包时，提示连接超时或者提示下载的包hash编码不匹配

  CMakeLists.txt文件内第三方包下载链接失效。

  **解决方案：**直接网上下载对应的压缩包，然后放至对应路径，若下载包的版本不一样，则修改相应CMakeLists.txt文件中包的md5值。

  本处提供一个经本人修改后的CMakeLists.txt,直接替换即可。

### 3.2 /hadoop/src/hadoop/src/c++/libhdfs/configure: Permission denied

  配置文件没有可执行权限。

  **解决方案**:

```bash
chmod a+rx /hadoop/src/hadoop/src/c++/libhdfs/configure
```

### 3.3 以上配置均完成后make时出现的其他错误

  **解决方案：**检查gcc、jdk版本，确定gcc版本<=5.4,jdk版本<=1.8。

## 4. 参考资料

1. [PowerGraph github网址](https://github.com/jegonzal/PowerGraph)
2. [前人总结的部署过程可能出现的错误](https://blog.csdn.net/nice_wen/article/details/80561441)
3. [PowerGraph集群部署](https://www.jianshu.com/p/af02bb459ff4)