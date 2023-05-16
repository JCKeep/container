1. 调用 `clone` 或 `unshare` 创建一个新的匿名 netns，或使用 `ip netns add` 创建。
2. 如何使用 clone 或 unshare 创建，则需要使用 `ls -l /proc/1282/ns/net` 和 `ls -l /var/run/netns/` 找出其对应的 netns 名字，如何找不到，则需要在 `/var/run/netns/` 目录下创建一个软链接，不然系统将无法通过名称正常访问新建的 `netns`。
   ```
   ln -s /proc/[pid]/ns/net /var/run/netns/[netns_name]
   ``` 
3. 创建一个 veth-pair
   ```
   ip link add veth0 type veth peer name veth1
   ```
4. 将其中一个移到新的 netns 当中
   ```
   ip link set veth1 netns [netns_name]
   ```
5. 为两个 veth 分配 ip 地址
   ```
   ip addr add 192.168.0.2/24 dev veth0
   ip netns exec [netns_name] ip addr add 192.168.0.1/24 dev veth1
   ```
6. 启动 veth1 和 veth0
   ```
   ip link set dev veth0 up
   ip netns exec [netns_name] ip link set dev veth1 up
   ```