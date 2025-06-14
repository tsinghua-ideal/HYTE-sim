# HYTE-sim
This is the source code repository of the ISCA'25 paper *HYTE: Flexible Tiling for Sparse Accelerators via Hybrid Static-Dynamic Approaches*.

## Build

```bash
$ g++ -O3 -march=native  src/config.cpp src/data.cpp src/estimation.cpp src/parameters.cpp src/util.cpp src/statistics.cpp src/cache.cpp src/dynamic.cpp src/simulator.cpp src/main.cpp -o hyte
```

## Workload
The scheduler and simulator accept sparse matrices from MatrixMarket (.mtx). The folder containing these matrices is under `data`.

## Run
The following command simulates multiplication of `matrix1` and `matrix2` with the configuration specified in `config/config.json`:
```bash
$ ./hyte matrix1 matrix2 config/config.json
```
Here is a sample json configuration:
```json
{
    "dataflow": 2,
    "format": 0,
    "transpose": 0,
    "minBlock": 2,
    "cachesize": 4,
    "memorybandwidth": 68,
    "PEcnt": 32,
    "srambank": 32,
    "iscache": 0,
    "cacheScheme": 0,
    "baselinetest": 0,
    "outputDir": "./output/"
}
```

## Code description
The code contains both baseline test and HYTE test. You can change the execution mode through the `baselinetest` parameter in the config.json, in which 0 represents HYTE and 1 represents the three baselines (run in turn). 

The static process and dynamic process of HYTE run together in `./hyte`. During the HYTE execution in `main.cpp`, firstly `getParameterSample()` from `estimation.cpp` is called to get the requisite parameters of the specific matrix as described in Section 5.1 in the paper. Then, we search for the best tiling scheme as described in Section 5.2 in the paper. The cost model described in Section 5.3 is called through the function `runTile()` from `simulator.cpp`, with the first parameter `isest = 1`, representing the analytical cost model. After obtaining the optimized static tiling scheme, `runTile()` from `simulator.cpp` is called with the first parameter `isest = 0`, representing the simulated performance. During the simulation, `update_T()` in `dynamic.cpp` is called for dynamic tuning as described in Section 6.4 in the paper.

## Reference

If you use this tool in your research, please kindly cite the following paper.

Xintong Li, Zhiyao Li, and Mingyu Gao. *HYTE: Flexible Tiling for Sparse Accelerators via Hybrid Static-Dynamic Approaches*. In Proceedings of the 52nd Annual International Symposium on Computer Architecture (ISCA), 2025.
