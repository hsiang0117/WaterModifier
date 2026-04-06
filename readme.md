# 水域修改工具 使用说明

<div align="center">
  <video src="readme/video.mp4"
         width="800"
         controls
         loop
         muted>
  </video>
</div>

## 一、安装环境

<font color = red>如果显卡的显存容量小于8g请跳过这一步，直接到第二步启动工具。</font>

### 1、确认显卡驱动版本

按下win+r键，输入cmd打开命令行，输入nvidia-smi查看显卡驱动信息。

![显卡驱动信息](readme/cuda_version.png)

如果支持的cuda版本小于12.4请升级显卡驱动。

### 2、安装cuda

安装提供的cuda toolkit

<div style="text-align:center">
  <img src="readme/cuda_toolkit.png" alt="cuda安装包"/>
</div>

### 3、确认安装成功

按下win+r键，输入cmd打开命令行，输入nvcc -V查看安装是否成功。

![cuda安装结果](readme/cuda_result.png)

<div STYLE="page-break-after: always;"></div>

## 二、启动工具

进入WaterModifier\Binaries\Win64，双击SegmentAnything-Socket-Backend.exe启动。可以为该程序创建一个快捷方式放到你想要的地方。

![目录](readme/directories.png)

![里层目录](readme/inner_directories.png)

启动过程需要加载AI模型，因此启动过程较慢，属于正常现象。

<div STYLE="page-break-after: always;"></div>

## 三、工具使用

### 1、基础界面

![基础界面](readme/ui_introduction.png)

<div STYLE="page-break-after: always;"></div>

### 2、加载瓦片地图

第一步需要加载瓦片地图，以瑶湖机场数据集为例，tilemapresource.xml这个文件所在的目录是瓦片地图的根目录，将这个目录输入地图文件根目录框中，点击确认即可加载地图集。<font color = red>注意文件目录下必须要有meta.json和tilemapresource.xml两个文件，文件名必须一致。如果没有meta.json可以自己写一个，需要其中的maxzoom字段，形式如 {"maxzoom":18} 即可</font>

![地图根目录](readme/map_root.png)

![地图根目录加载完毕](readme/map_root_loaded.png)

加载完成后便可使用地图，可以通过滚轮或右上角选择框调整地图等级，通过右上角第二个选择框调整视野范围，使用鼠标左键按住拖动地图。左上角会实时显示当前中心点所处的经纬度。

<div STYLE="page-break-after: always;"></div>

### 3、加载地形数据

第二步需要加载对应的地形数据，同样以瑶湖机场为例，将layer.json所在的目录输入地形数据根目录框中，点击确认加载地形数据集。<font color = red>注意文件目录下必须要有layer.json文件，文件名必须一致。</font>

![地形根目录](readme/terrain_root.png)

![地形根目录加载完毕](readme/terrain_root_loaded.png)

<div STYLE="page-break-after: always;"></div>

### 4、查看水域功能

加载完正确的地形数据后，左上角查看水域按钮会启用，点击查看水域开启查看水域功能，当前的水域会显示为蓝色。

![查看水域](readme/water_view.png)

<div STYLE="page-break-after: always;"></div>

### 5、分割水域功能

点击锁定后，当前视角会被锁定，同时启用分割功能。

#### ①自动分割

右下角模式调整为自动，此时启动鼠标选点功能。左键点击为前景点，即需要的水面部分；右键点击为背景点，即不需要的其他部分。在想要的水面上点击左键选几个点，在非水面区域点击右键选几个点，再点击分割就会自动分割出水域。

![自动分割](readme/water_segment_auto.png)

若觉得分割效果不理想，可以继续添加选点再次分割，直到得到想要的效果。

<div STYLE="page-break-after: always;"></div>

#### ②手动分割

右下角模式调整为手动，此时左键点击选点手动画多边形。逻辑参考photoshop钢笔工具，会根据选点标记出一块多边形区域。选好标点后点击分割。

![手动分割](readme/water_segment_manual.png)

右上角提供了按钮可以隐藏ui方便观察分割结果。

<div STYLE="page-break-after: always;"></div>

#### ③修改逻辑

分割完成后可以点击修改按钮进行修改。左边两个勾选框决定了修改方法。覆盖决定了修改是否会完全覆盖原有的数据。例如，勾选上覆盖后进行修改，则分割出的水域会被修改为有水，而分割出不是水域的区域会被修改为无水，如数据中原来某区域有水，而分割结果中该区域被标记为无水，则最终会修改为无水。若不勾选覆盖，则相当于向数据中添加水，仅分割出的水域区域会被加入到数据中，其他区域的水域信息保持不变。

同步勾选框决定了是否会将修改结果同步到高lod等级，若勾选上修改，则修改结果会一路同步到最高的lod等级。例如，从14级修改，勾选同步，而数据集最高lod等级是18级，则14-18级下该区域都会被同步修改。若不勾选同步修改则只会修改当前所处等级的数据。因为同步修改所涉及的数据量是指数级增长的，因此不建议从过低的等级上同步修改。

修改前水域

![修改前水域](readme/before_modify.png)

分割结果

![分割结果](readme/segment_result.png)

注意，在修改时会对分割结果进行形态学处理，分割结果存在一些噪点可以不用在意，会被自动修补。但同时因为形态学处理会导致分割结果的边缘与最终修改到数据的边缘有一些区别，总体上是对分割结果的边缘进行了一些平滑操作。

<div STYLE="page-break-after: always;"></div>

根据需要选择是否勾选覆盖和同步，之后点击修改即可开始修改。

![修改中](readme/modifying.png)

<div STYLE="page-break-after: always;"></div>

修改后水面。

![修改后](readme/after_modify.png)