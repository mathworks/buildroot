# Catalog Files

Catalog files are used to tell Buildroot what to build. They specify what components
to build and how to combine them together. An example file is show below:

```xml
<board name="zed" platform="zynq">
    <default>
        <app name="default">
            <fsbl file="axilite_fsbl.elf"/>
            <bit file="axilite.bit"/>
        </app>
    </default>
    <image name="zynq7000ec">
        <app name="axilite">
            <dts file="axilite.dts"/>
        </app>
        <app name="axistream">
            <dts file="axistream.dts"/>
        </app>
    </image>
</board>
```

Catalog files are comprised of _images_ and _apps_. An _image_ is a full system
image, made up of one or more _apps_. Each _image_ will generate its own .zip or .img.gz file.
An _app_ is a combination of bootloader, devicetree and FPGA bitstream, and will
affect the files contained within the _image_.

# XML Format

## Board Node
The board node is the top level node in the XML file. There must be exactly one instance of this node.

The board node has the following attributes:

| __Attribute__ | __Description__ |
| --- | --- |
| name | The board name (e.g. zed, zc706, etc) | 
| platform | The platform name (e.g. zynq, socfpga) |

__name__: The name can specify a board supported in tree (in which case the various
source files supplied in tree will be available for use) or specify a completely new board.
If a new board is specified, the _boardInfo_ directory must be supplied within the default node.

__platform__: The platform must be one of the platforms supported in tree:
* zynq
* socfpga

The board node has the following sub-nodes: 

| __Node__      | __Attributes__ | __Description__ |
| ---           | ---   | --- |
| default       | N/A   | The default settings for the catalog file. Must be exactly one instance of this node | 
| image         | name  | A description of the image to build, must be one or more instances of this node|

## Default Node
The default node supplies settings that apply to the entire catalog file (all images / apps).

The board node has the following sub-nodes: 

| __Node__      | __Attributes__ | __Description__ |
| ---           | ---   | --- |
| app           | name  | The default settings for all apps within the catalog; will be used unless overriden | 
| board_dir     | dir   | The board directory to use. Will use the in-tree directory if not specified. __Must__ be specified for boards not supported in-tree|
| sdcard        | dir   | The source directory to copy over contents at the root of the SD card. Will use the in-tree directory if not specified |
| dtsi          | dir   | A DTS include directory-- will be added to the path for compiling the DTB file. Multiple nodes can be specified |
| br2_config    | file  | A buildroot config file to append to generated buildroot config. Can be used to specify new values or override existing ones |
| kernel_config | file  | The kernel configuration to use instead of the default in-tree defconfig |

#### Platform Notes: socfpga

For the socfpga platform, the _app_ node must specify a _handoff_ directory
(see the app section below). This is used in the catalog-wide build process and cannot
be overriden by image-specific applications.

## Image Node(s)
Each image represents a distinct output from the tool, made up of the common settings
(e.g. those specified by the board/platform/default node) and the application-specific settings.
The name of the image file will be based on the name attribute of the image node.

The image node has the following sub-nodes: 

| __Node__      | __Attributes__ | __Description__ |
| ---           | ---   | --- |
| app           | N/A   | The an app to include in the image. The first listed app will be considered the default app, used to boot the image | 
| sdcard        | dir   | The source directory to copy over contents at the root of the SD card. Will be combined with the contents specified in the _default_ node (or the in-tree contents). |
| dtsi          | dir   | A DTS include directory-- will be added to the path for compiling the DTB file. Will be combined with the _dtsi_ directories specified in the _default_ node. Multiple nodes can be specified |

### App Node(s)
The app node specifies the combination of bootloader, devicetree, and FPGA bitstream to build.
If values are not specified, the values from the default app node are used.

The app node has the following sub-nodes: 

| __Node__      | __Attributes__ | __Description__ |
| ---           | ---   | --- |
| dts           | file  | The DTS file to build for the application. |
| fsbl          | file  | (zynq only) The First Stage Bootloader to build for the application. |
| handoff       | dir   | (socfpga only) The Quartus handoff files to use in generating the U-Boot SPL. |
| bit           | file  | The bitstream to build for the application. |

When locating the specified files, the following paths are searched (in order):
* Relative to the catalog file, in the per-file subdirectories
    * dts: ./dts directory
    * bit: ./boot directory
    * fsbl: ./boot directory
* Relative to the catalog file
* Relative to the board directory (in-tree or otherwise), in the per-file subdirectories as above

#### Platform Notes: zynq
If an _app_ node specifies a FSBL or bitstream, a BOOT.BIN will be generated for
the app. Otherwise one will not be generated. A BOOT.BIN will always be genreated for
the first (default) app.
