# Ns3-cc-simulation 
在ns3.33上实现了linux-reno，newreno，vegas，westwood，bic，cubic，dctcp，bbr，copa等拥塞控制算法
## Build
```shell
cd ns3.33  
mkdir traces
./waf configure  
./waf build  
```
## Run
```shell
./waf --run "scratch/tcp-test --cc=reno --folder=reno"
```
## Results
在trace文件夹下可看成了对应的trace数据

```shell
cd plot
bash cwnd-plot.sh
```

绘制cwnd图，其他类似

## 参考

主要参考来源于https://github.com/SoonyangZhang/ns3-congestion-variants

和https://github.com/SoonyangZhang/ns3-tcp-bbr
