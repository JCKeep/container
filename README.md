# CONTAINER

一个基于 Linux Namespace & Cgroup 的简易容器

## 如何使用

1. 编译

    ```
    make
    ```

    ![image-20230501003721877](./assets/image-20230501003721877.png)

2. 运行容器

    ```
    make run
    ```

    ![image-20230501003752741](./assets/image-20230501003752741.png)

    当输出`container starting`是代表容器已经开始在后台运行了

3. 进入容器

    ```
    make exec
    ```

    ![image-20230501003957266](./assets/image-20230501003957266.png)

这样我们就得到了一个运行在一个新的容器中的bash，这个容器与主机资源相互隔离，可以在`include/c_cgroup.h`中对容器资源进行限制，默认为 `cpu: 10%, memory: 64M, cpuset: 0-1, stack: 32K`，未来将增加配置文件支持，像Dockerfile及docker-compose.yml一样对容器进行定制化配置。


## TO IMPLEMENT

- [x] 实现 make exec 进入容器 bash
- [ ] 重构 namespace 模块
- [ ] 实现一个容器最小文件系统
- [ ] 加入 user_namespace 保户主机文件安全 (目前我的WSL未开启userns，暂时跳过)
- [ ] 增加 net_namespace 支持 (Linux net 子系统过于复杂，将在7月份开始学习)
- [ ] 增加配置文件支持
- [ ] 支持多个容器，实现管理多个容器的用户命令
- [ ] 使用 OverlayFS 文件系统支持容器镜像
- [ ] namespace 内核源码实现解析
- [ ] cgroup 内核源码实现解析
- [ ] 重构 cgroup 模块 (目前规模较小，无需重构)

**. . .**

## 最小文件系统

ME: **能否自己定制一个最小文件系统**

chatGPT: 

> 是的，可以自己定制一个最小文件系统来用于 Linux 容器。
>
> 通常情况下，最小文件系统只需要包含操作系统所需的最小文件和程序即可。一般会选择一个已有的最小文件系统作为基础，然后再从中删除不需要的文件和程序，以减小文件系统的大小。常见的最小文件系统包括 BusyBox 和 Alpine Linux。
>
> 如果需要自己定制最小文件系统，可以采用 BusyBox 来构建。BusyBox 是一个开源的工具集，其中包含了许多用于构建最小文件系统的工具，例如 init、sh、ls、cat、cp、mkdir 等。可以使用 BusyBox 来构建最小文件系统，并自定义所需的文件和程序。

ME: **怎么用BusyBox构建一个最小文件系统**

charGPT:

> 可以使用 BusyBox 工具来构建一个最小文件系统。BusyBox 是一个单一的可执行文件，包含了许多常用的 UNIX 工具，如 ls、cp、mv、rm、sed、awk、sh 等。
>
> 以下是构建最小文件系统的步骤：
>
> 1. 下载 BusyBox 工具：
>
> ```shell
> $ wget https://busybox.net/downloads/busybox-1.33.0.tar.bz2
> ```
>
> 2. 解压 BusyBox：
>
> ```shell
> $ tar xjf busybox-1.33.0.tar.bz2
> ```
>
> 3. 进入 BusyBox 目录，进行配置：
>
> ```shell
> $ cd busybox-1.33.0/
> $ make defconfig
> ```
>
> 运行 `make defconfig` 命令将使用默认配置文件进行配置。也可以运行 `make menuconfig` 命令自定义配置文件。
>
> 4. 编译 BusyBox：
>
> ```shell
> $ make
> ```
>
> 编译完成后，会生成一个 `busybox` 可执行文件。
>
> 5. 创建最小文件系统目录：
>
> ```shell
> $ mkdir rootfs
> $ cd rootfs
> $ mkdir -p bin etc proc sys dev
> ```
>
> 在 `rootfs` 目录下创建 `bin`、`etc`、`proc`、`sys` 和 `dev` 目录。
>
> 6. 复制 BusyBox 可执行文件到 `bin` 目录：
>
> ```shell
> $ cp ../busybox bin/
> ```
>
> 复制完成后，`bin` 目录下会有一个名为 `busybox` 的可执行文件。
>
> 7. 创建设备节点：
>
> ```shell
> $ cd dev
> $ sudo mknod console c 5 1
> ```
>
> 创建一个名为 `console` 的设备节点。
>
> 8. 创建 `etc` 目录下的文件：
>
> ```shell
> $ cd ../etc
> $ touch passwd
> $ echo "root:x:0:0:root:/root:/bin/sh" > passwd
> ```
>
> 创建一个名为 `passwd` 的文件，并写入 `root` 用户的信息。
>
> 9. 在 `proc` 和 `sys` 目录下创建挂载点：
>
> ```shell
> $ cd ../
> $ sudo mount -t proc none proc/
> $ sudo mount -t sysfs none sys/
> ```
>
> 挂载 `proc` 和 `sys` 文件系统到 `rootfs/proc` 和 `rootfs/sys` 目录下。
>
> 现在，最小文件系统就已经创建完成了。可以使用 `chroot` 命令切换到 `rootfs` 目录下，然后执行 `./bin/busybox sh` 命令启动最小文件系统。
>
> 在BusyBox创建的最小文件系统中，只会包含最基本的系统工具和必要的文件，没有链接库和其他依赖。因此，如果你需要运行的应用程序依赖于库，你需要手动将它们添加到文件系统中。一种方法是将依赖项静态链接到应用程序中，另一种方法是将依赖项动态链接并将库一同打包到文件系统中。
>
> 是的，一般情况下最小文件系统不会包含bash，而是包含一个更加轻量的shell，比如Alpine Linux默认的shell是ash。因此，启动容器后进入最小文件系统，使用的命令行解释器一般是sh，而不是bash。



## overlayfs

ME: **如何使用overlayfs挂载一个最小文件系统**

charGPT:

> 要使用OverlayFS挂载一个最小文件系统，首先需要创建两个目录，分别用于挂载lowerdir和upperdir：
>
> ```shell
> mkdir /tmp/lower
> mkdir /tmp/upper
> ```
>
> 接下来，可以将一个预先构建好的最小文件系统作为lowerdir，这里以busybox的最小文件系统为例：
>
> ```shell
> wget https://www.busybox.net/downloads/binaries/1.31.1-defconfig-multiarch/busybox-x86_64
> chmod +x busybox-x86_64
> ./busybox-x86_64 --list > files.txt
> mkdir /tmp/lower/rootfs
> cat files.txt | xargs -I {} sh -c 'cp -av /{} /tmp/lower/rootfs/{}'
> ```
>
> 然后，可以使用mount命令挂载OverlayFS：
>
> ```shell
> mount -t overlay overlay -o lowerdir=/tmp/lower/rootfs,upperdir=/tmp/upper,workdir=/tmp/work /mnt
> ```
>
> 其中，lowerdir指定了lower directory，即预先构建好的最小文件系统；upperdir指定了upper directory，即存储OverlayFS的写入数据；workdir指定了work directory，即OverlayFS内部使用的工作目录。
>
> 挂载完成后，可以进入/mnt目录查看最小文件系统的内容。
>
> ```shell
> cd /mnt
> ls
> ```
>
> 这样就可以使用OverlayFS挂载一个最小文件系统了。

ME: **/tmp/lower/rootfs可以是其他文件系统吗**

chatGPT:

> 是的，`/tmp/lower/rootfs` 可以是其他文件系统，只要它们可以被挂载到该位置。`lowerdir` 可以指定一个或多个只读的目录作为下层目录，这些目录可以是各种类型的文件系统，包括 ext4、xfs 等等。这些下层目录中的内容将被保留在 overlay 文件系统中，而上层目录中的内容将会隐藏下层目录中的相同路径的内容。

ME: **upperdir用来做什么的**

chatGPT:

> `upperdir` 是 OverlayFS 中的一个参数，表示 OverlayFS 在写入文件时的一个工作目录。OverlayFS 在读取文件时会首先检查 `upperdir` 目录中是否存在该文件，如果存在则直接使用该文件，如果不存在则会继续在 `lowerdir` 目录中查找。
>
> 在 OverlayFS 中，`upperdir` 是可写的，所有的写操作会在 `upperdir` 中进行，而 `lowerdir` 是只读的，不能在 `lowerdir` 中进行写操作。这种设计使得 OverlayFS 支持写时复制（copy-on-write）的机制，即当对一个文件进行修改时，OverlayFS 会将这个文件复制到 `upperdir` 目录中，并在该文件的副本上进行修改，从而避免了对 `lowerdir` 目录中的文件进行直接修改。这种机制可以提高文件系统的性能，也可以保证文件系统的稳定性，避免对原有文件的破坏。

ME: **那我如何使用多层目录**

chatGPT: 

> 使用多层目录的方式与单层目录类似，只需要在挂载时指定多个 `lowerdir` 参数即可。例如：
>
> ```shell
> mount -t overlay overlay -o lowerdir=/tmp/lower1:/tmp/lower2:/tmp/lower3,upperdir=/tmp/upper,workdir=/tmp/work /mnt/overlay
> ```
>
> 这个命令将 `/tmp/lower1`、`/tmp/lower2` 和 `/tmp/lower3` 这三个目录作为下层目录，`/tmp/upper` 目录作为上层目录，`/tmp/work` 目录作为工作目录，然后将它们作为一个整体挂载到 `/mnt/overlay` 目录。

ME: **多层目录优先级如何**

chatGPT:

> 在 OverlayFS 中，多个层的文件系统是按照挂载的先后顺序来确定优先级的。越后挂载的文件系统的优先级越高。在读取某个文件时，会从上层文件系统向下查找，直到找到该文件或查找完所有的层。在写入文件时，会将修改写入到最高优先级的上层文件系统中。如果上层文件系统没有该文件，则会创建新的文件。如果上层文件系统存在该文件，则会在上层文件系统中进行修改，而下层的文件系统中的该文件并不会被修改。因此，在 OverlayFS 中，多层目录的优先级可以通过挂载顺序来控制。

