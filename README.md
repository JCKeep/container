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

- [x] attach to container bash: `make exec`
- [ ] add net_namespace
- [ ] implememt config file like Dockerfile

**. . .**
