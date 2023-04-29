# DEMO_CONTAINER

一个基于 Linux Namespace & Cgroup 的简易容器

## 如何使用

1. 编译

    ```
    make
    ```

    ![img](assets/1.png)

2. 运行

    ```
    make run
    ```

    ![img](assets/2.png)

3. 进入容器

    ```
    make exec
    ```

    ![img](assets/3.png)

这样我们就得到了一个运行在一个新的容器中的bash，这个容器与主机资源相互隔离，可以在`include/c_cgroup.h`中对容器资源进行限制，默认为 `cpu: 30%, memory: 128M, cpuset: 0-1, stack: 2M`，
未来将增加配置文件支持，像Dockerfile及docker-compose.yml一样对容器进行定制化配置。


## TO IMPLEMENT

- [x] attach to container bash: `make exec`
- [ ] implememt config file like Dockerfile
- [ ] add net_namespace

    **. . .**
