# Overlay FS

OverlayFS 是一种联合文件系统，可以将多个不同的文件系统叠加在一起成为一个只读的文件系统，同时保留每个文件系统自己的可写部分。它使用了 Linux 内核提供的 OverlayFS 文件系统驱动程序，这个驱动程序使得多个文件系统可以透明地叠加在一起，表现为一个统一的文件系统视图。OverlayFS 在 Linux 容器技术中被广泛使用，因为它提供了轻量级、高效的容器文件系统隔离机制。

OverlayFS 由一个只读的“下层文件系统”（lowerdir）和一个只读的“上层文件系统”（upperdir）组成，另外还有一个“工作目录”（workdir）用于 OverlayFS 的内部使用。下层文件系统包含一个只读的基础文件系统，通常是一个发行版的根文件系统，它是只读的，不能被修改。上层文件系统包含应用程序的可写部分，它可以在容器运行时修改。工作目录是一个临时目录，用于 OverlayFS 内部存储工作文件和元数据。

OverlayFS 使用联合挂载（union mount）技术，将下层文件系统和上层文件系统合并为一个文件系统，同时保留了两者各自的只读和可写部分。在容器内部，所有对文件系统的读写操作都会首先被 OverlayFS 拦截并重定向到上层文件系统中的对应文件，只有当上层文件系统中没有相应的文件时，才会读取下层文件系统中的对应文件。

OverlayFS 的优点包括：

1. 轻量级：OverlayFS 仅需要少量的内存和处理器资源，因此非常适合在容器等资源受限环境中使用。
2. 高效：OverlayFS 不需要在容器启动时复制整个文件系统，而是仅需要复制上层文件系统中的变更部分，因此启动速度很快。
3. 灵活性：OverlayFS 可以支持多个上层文件系统，因此可以轻松地实现镜像叠加、多版本部署等场景。
4. 容错性：OverlayFS 支持联合挂载，因此即使上层文件系统出现问题，下层文件系统仍然可以正常工作，从而保障容器的稳定性和可靠性。

但是，OverlayFS 也有一些缺点：

1. 安全性：由于 OverlayFS 可以访问所有的下层文件系统，因此容器内部可能会访问到宿主机的文件系统，造成安全隐患。
2. 性能：OverlayFS 需要进行额外的文件系统重定向操作，因此可能会对性能造成一定的影响


## CHATGPT

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