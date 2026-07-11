# topic_sparse_const 实现与汇报说明

本文档用于说明本仓库中 `topic_sparse_const` 的实现思路、两条实现路径的区别、当前新增能力以及验证结果。

一句话概括：

```text
本实现利用 metatensor 中 region_direct 常量在代码生成期可见的特点，
提前读取稀疏常量 K 的数值，只为真正有意义的位置生成计算代码。
```

当前实现已经超过题目最低 demo 范围，支持一维、高维 dense、tuple sparse、`X` sparse、双 sparse tuple、自动选路、IR datadesc 改写以及连续块 SIMD-ready 代码生成。

## 1. 课题目标

原始普通代码会对每个位置都生成逐元素计算：

```c
out[i] = K[i] * x[i];
```

但本课题中的 `K` 是编译期可见的常量数组。如果 `K` 中很多位置为 `0`，那么这些位置的乘法没有必要生成：

```c
K[i] == 0  =>  out[i] 一定是 0
```

所以优化目标是：

```text
在代码生成阶段读取 K 的真实值，
跳过 K == 0 的位置，
简化 K == 1 的位置，
只生成真正需要的计算。
```

题目最低 demo 只要求：

```text
float
一维
K * X
K 是 dense 数组但内部有很多 0
跳过 K == 0 的乘法
```

当前版本已经扩展为：

```text
K * X
X * K
K + X
float
double
一维 dense
二维/多维 dense
K tuple sparse
X tuple sparse
K 和 X 同时 tuple sparse
自动选择直线代码 / IR 改写 / 连续块 SIMD-ready 路径
```

## 2. metatensor 原理基础

要理解这个优化，必须先理解三个概念：`region_direct`、`datadesc` 和 `block`。

### 2.1 region_direct

在 metatensor 里，数据不只是一个 C 指针，而是用 `datadesc` 描述。

其中 `region` 表示数据位于哪里：

```text
region_direct  编译期直接可见的数据
region_main    运行期主内存中的数据
```

本课题成立的关键就在这里：

```text
K.region == region_direct
```

这意味着插件在生成 C 代码时，能直接通过：

```c
K->ref.p_direct
```

读到 `K` 数组的真实内容。

而 `X` 是运行期输入：

```text
X.region == region_main
```

所以插件生成代码时不知道 `X[i]` 的值，只知道它的地址表达式。

因此优化逻辑是：

```text
K 的值：编译期已知，可以用来决定生成什么代码
X 的值：运行期才知道，只能生成访问 X 的代码
```

### 2.2 datadesc 的寻址原理

`datadesc` 不只是描述“一维数组”。它描述的是一个张量在内存中的布局。

一个元素的真实位置由这些信息共同决定：

```text
真实地址 = ref + offset0 + 所有维度 offset 之和 * elem_size
```

对一维 dense 来说，很简单：

```text
位置 i -> offset = dims[0]->offsets[i]
```

对二维 dense 来说：

```text
位置 (row, col) -> offset = row_offset + col_offset
```

例如：

```text
row = 1, col = 2
row_offset = 4
col_offset = 2
线性 offset = 6
```

所以生成代码时会变成：

```c
out[6] = K[6] * x[6];
```

这就是为什么高维情况下依然可以生成普通 C 数组访问。

### 2.3 tuple sparse 的含义

普通 dense 二维矩阵会存所有坐标：

```text
(0,0), (0,1), (0,2), ...
(1,0), (1,1), (1,2), ...
```

tuple sparse 不存完整矩阵，只存有意义的坐标：

```text
(0,0) -> 0.5
(1,2) -> 1.0
(2,3) -> -0.75
```

在 `datadesc` 里，这种形式表现为：

```text
dims[0]->n_tuple > 1
```

例如 `n_tuple == 2` 表示一个 entry 同时描述两个维度：

```text
entry 0: (row=0, col=0)
entry 1: (row=1, col=2)
entry 2: (row=2, col=3)
```

当前实现会根据 tuple 中保存的坐标，把它映射到 `X` 或输出的真实 offset。

## 3. 输入 IR 形态

当前插件主要匹配如下 IR 形态：

```text
con_ser(reduce, con_par_int(K_set, X_set))
```

也就是课题文档里的：

```text
[prod]-(K & X)
[sum]-(K & X)
```

其中：

```text
K_set 中有一个 datadesc，且 K.region == region_direct
X_set 中有一个 datadesc，且 X.region != region_direct
```

插件也支持反向乘法：

```text
[prod]-(X & K)
```

识别出哪一边是 `K` 后，后面的优化逻辑是一样的。

## 4. 两条实现路径

题目文档里给了两条路线：

```text
路线 A：block2data 直接代码生成
路线 B：IR / datadesc 改写后复用 arithmetic
```

本项目是按“先路线 A，后路线 B”的顺序实现的。

### 4.1 路线 A：直接代码生成路径

路线 A 的核心思想是：

```text
插件自己读取 K，
插件自己判断哪些位置有用，
插件自己直接发射 C 代码。
```

例如原始逻辑是：

```c
for (i = 0; i < N; i++) {
    out[i] = K[i] * x[i];
}
```

如果：

```text
K = [0, 1, 2.5, 0]
```

路线 A 会直接生成：

```c
out[1] = x[1];
out[2] = 2.5f * x[2];
```

不会生成：

```c
out[0] = 0.0f * x[0];
out[3] = 0.0f * x[3];
```

乘法规则：

```text
K == 0  不生成该位置的乘法，输出提前 memset 为 0
K == 1  生成 out = x
其他值  生成 out = K * x
```

加法规则：

```text
K == 0  生成 out = x
其他值  生成 out = K + x
```

路线 A 的优点：

```text
简单直接，容易验证
对小规模稀疏数组非常高效
生成代码很直观，适合 demo 展示
可以精确处理 K==0、K==1、K+X 中 K==0 的特例
```

路线 A 的缺点：

```text
如果非零位置很多，会生成很多行代码
优化逻辑和具体算子绑定较紧
IR 层面看不到“数据被改写成稀疏子集”的痕迹
不利于后续和更多通用优化组合
```

### 4.2 路线 B：IR datadesc 改写路径

路线 B 的核心思想是：

```text
不要直接为每个位置手写 C 语句，
而是把原来的 K 和 X 改写成只包含有效位置的 datadesc 子视图，
再交给原来的 block2data_arithmetic 处理。
```

通俗地说：

```text
路线 A：我自己算，我自己写代码。
路线 B：我先把数据描述改好，再让系统原来的 arithmetic 插件来算。
```

例如原来有：

```text
K: [0, 1, 2.5, 0, -3]
X: [x0, x1, x2, x3, x4]
```

有效位置是：

```text
1, 2, 4
```

路线 B 会构造新的逻辑视图：

```text
K_ir = [1, 2.5, -3]
X_ir = [x1, x2, x4]
```

然后交给已有 arithmetic 路径生成：

```text
tmp[0] = K_ir[0] * X_ir[0]
tmp[1] = K_ir[1] * X_ir[1]
tmp[2] = K_ir[2] * X_ir[2]
```

最后再 scatter 回完整输出：

```text
out[1] = tmp[0]
out[2] = tmp[1]
out[4] = tmp[2]
```

当前实现中的路线 B 是在 `block2data_sparse_const` 内部完成的：

```text
构造 K_ir / X_ir datadesc
调用 block2data_arithmetic
再把临时结果 scatter 回 out
```

也就是说，它已经实现了“datadesc 改写 + 复用 arithmetic”的核心思想，但还不是一个独立注册到全局优化链表里的 `optimize_f` pass。

路线 B 的优点：

```text
更接近编译器 IR 优化思想
可以复用已有 block2data_arithmetic
当非零位置很多时，避免生成超长直线代码
更适合后续和其它优化串联
能更清楚体现 datadesc indices / offsets 子集改写
```

路线 B 的缺点：

```text
实现复杂，需要正确构造新的 dimdesc / datadesc
需要处理临时结果再 scatter 回完整输出
小规模 case 可能不如路线 A 直接
当前版本还不是全局 optimize_f pass
```

### 4.3 两条路线对比

| 对比项 | 路线 A：直接代码生成 | 路线 B：IR datadesc 改写 |
|---|---|---|
| 核心思想 | 插件直接发 C 代码 | 先改写 datadesc，再复用 arithmetic |
| 适合场景 | 非零位置少，小 demo，直观展示 | 非零位置较多，需要控制代码长度 |
| 代码形态 | 多条 `out[i]=...` | offset 表 + 循环 + 临时结果 |
| 优点 | 简单、快、容易看懂 | 更通用、更像真正编译优化 |
| 缺点 | 非零多时代码膨胀 | 构造 datadesc 更复杂 |
| 当前作用 | 基础优化主路径 | 高级优化路径 |

当前自动选路策略：

```text
非零位置少       -> 路线 A 直线代码
非零位置很多     -> 路线 B IR datadesc 改写
非零位置连续成块 -> 连续块 / SIMD-ready 路径
```

## 5. 新增实现的核心结构

为了支持高维、tuple sparse、IR 改写和自动选路，当前插件先把不同形态统一抽象成一组 `sparse_item`。

每个有效位置记录四个信息：

```text
k_offset    K 中的元素 offset
x_offset    X 中的元素 offset
out_offset  输出中的元素 offset
k_value     K 在该位置的真实值
```

这个统一结构很重要。

因为不管输入是一维、二维还是 tuple sparse，最终都可以变成：

```text
在 out_offset 位置，使用 k_value 和 x_offset 位置的 X 生成结果
```

后续三种代码生成路径都基于这组 `sparse_item`：

```text
sparse_item 列表
    -> 直线代码
    -> IR datadesc 改写
    -> 连续块 / SIMD-ready
```

## 6. 高维 dense 的实现原理

高维 dense 的关键是：不能再假设位置就是 `i`。

二维情况下，一个坐标 `(row, col)` 的真实 offset 是：

```text
offset = row_offset + col_offset
```

当前实现会根据维度名匹配 `K` 和 `X`：

```text
K 的 row 维度 对应 X 的 row 维度
K 的 col 维度 对应 X 的 col 维度
```

然后枚举 `X` 的所有坐标，计算：

```text
k_offset
x_offset
out_offset
```

例如：

```text
(row=1, col=2)
```

可能得到：

```text
k_offset = 6
x_offset = 6
out_offset = 6
```

如果 `K[1][2] == 0`，乘法路径就跳过。

如果 `K[1][2] == 1`，就生成：

```c
out[6] = x[6];
```

如果 `K[1][2] == -1.5`，就生成：

```c
out[6] = -1.5f * x[6];
```

作用：

```text
把原本只能处理一维数组的稀疏常量优化，扩展到 metatensor 的高维 datadesc。
```

## 7. tuple sparse 的实现原理

tuple sparse 的关键是：一个 entry 不是一个普通下标，而是一组坐标。

例如：

```text
entry 0: (row=0, col=0) -> 0.5
entry 1: (row=1, col=2) -> 1.0
entry 2: (row=2, col=3) -> -0.75
```

当前实现会：

```text
读取 tuple 中的维度名
读取 tuple 中的坐标值
在目标 dense 或 sparse X 中寻找对应坐标
计算输出 offset
生成 sparse_item
```

### 7.1 K tuple sparse, X dense

`K` 只存非零坐标，`X` 是完整 dense 矩阵。

这时输出仍然和 `X` 一样是 dense。乘法时：

```text
K 中没出现的位置，语义上就是 0
输出提前 memset 为 0
只对 K 中出现的坐标生成计算
```

### 7.2 K dense, X tuple sparse

`K` 是完整矩阵，`X` 只存部分坐标。

这时输出保持 `X` 的 sparse 形态。插件只枚举 `X` 中存在的坐标，然后到 dense `K` 里查对应位置的值。

作用：

```text
支持运行期输入 X 本身就是稀疏结构，而不是只支持 dense X。
```

### 7.3 K tuple sparse, X tuple sparse

这是双 sparse tuple 情况。

插件会以 `X` 的坐标为输出坐标，去 `K` 的 tuple 坐标中查找是否存在匹配项：

```text
X 有该坐标，K 也有该坐标 -> 使用 K 的值计算
X 有该坐标，K 没有该坐标 -> K 视为 0
```

乘法时：

```text
K 缺失 -> out = 0
```

加法时：

```text
K 缺失 -> out = X
```

作用：

```text
把优化从“常量数组里有 0”提升到“两个 sparse tuple datadesc 之间按坐标匹配”。
```

## 8. 连续块和 SIMD-ready 路径

稀疏优化后，非零位置可能是零散的：

```text
0, 3, 5, 11
```

这种情况下 SIMD 不好做，因为 SIMD 喜欢连续内存。

但如果非零位置刚好连续：

```text
0, 1, 2, 3, ..., 19
```

就可以把它变成块循环。

当前实现会检测连续块，如果连续块足够长，就生成：

```c
#if defined(__AVX2__)
_mm256_loadu_ps(...)
_mm256_mul_ps(...)
_mm256_storeu_ps(...)
#endif
```

同时保留标量循环：

```c
for (; i < len; i++) {
    out[start + i] = K_block[i] * x[start + i];
}
```

这样做的好处是：

```text
默认编译环境不支持 AVX2 也能通过
如果以后开启 -mavx2，就能走 SIMD intrinsic
连续非零块不会生成大量重复赋值语句
```

运行测试时如果命中这条路径，会看到：

```text
SPARSE SIMD BLOCK PATH HIT
```

## 9. 当前支持规则

### 9.1 表达式形态

```text
K * X
X * K
K + X
```

### 9.2 数据类型

```text
float
double
```

### 9.3 数据布局

```text
K dense,        X dense
K tuple sparse, X dense
K dense,        X tuple sparse
K tuple sparse, X tuple sparse
```

### 9.4 乘法规则

```text
K == 0  跳过，输出靠 memset 得到 0
K == 1  生成 out = x
其他值  生成 out = K * x
```

### 9.5 加法规则

```text
K == 0  生成 out = x，或先 memcpy X 后跳过该位置
其他值  生成 out = K + x
```

## 10. 工作成果

核心实现文件：

```text
plugins/block2data_sparse_const.c
```

主要新增/完善内容：

```text
统一 sparse_item 枚举结构
一维 dense 支持
二维/多维 dense 支持
tuple sparse K 支持
tuple sparse X 支持
K/X 双 tuple sparse 支持
float / double 支持
K * X / X * K / K + X 支持
K == 0、K == 1、K + X 中 K == 0 的特化
自动选路
IR datadesc 改写路径
连续非零块 SIMD-ready 路径
```

测试生成文件：

```text
main_test/test.sparse_const_basic.c
```

数值验证文件：

```text
main_test/test.sparse_const_verify.c
```

编译脚本：

```text
Makefile_test_sparse_const
```

当前会生成这些 dump 文件：

```text
dump_sparse.c                  一维 float K * X
dump_sparse_reverse.c          一维 float X * K
dump_sparse_sum.c              一维 float K + X
dump_sparse_double.c           一维 double K * X
dump_sparse_2d.c               二维 dense K * X
dump_sparse_2d_sum.c           二维 dense K + X
dump_sparse_tuple.c            K tuple sparse, X dense, prod
dump_sparse_tuple_sum.c        K tuple sparse, X dense, sum
dump_sparse_ir.c               IR datadesc 改写路径
dump_sparse_block.c            连续块 / SIMD-ready 路径
dump_sparse_x_tuple.c          K dense, X tuple sparse
dump_sparse_dual_tuple.c       K tuple sparse, X tuple sparse, prod
dump_sparse_dual_tuple_sum.c   K tuple sparse, X tuple sparse, sum
```

## 11. 验证方式

在仓库根目录运行：

```powershell
mingw32-make -f Makefile_test_sparse_const clean
mingw32-make -f Makefile_test_sparse_const verify_sparse
.\verify_sparse.exe
```

期望结果：

```text
OK
```

生成代码时还会看到：

```text
SPARSE PATH HIT
SPARSE IR REWRITE PATH HIT
SPARSE SIMD BLOCK PATH HIT
```

含义：

```text
SPARSE PATH HIT             sparse_const 插件被调用
SPARSE IR REWRITE PATH HIT  IR datadesc 改写路径被调用
SPARSE SIMD BLOCK PATH HIT  连续块 / SIMD-ready 路径被调用
```

当前验证结果已经通过：

```text
verify_sparse.exe 输出 OK
```

说明所有生成代码与对应朴素版本逐元素比较一致。

## 12. 汇报时可以这样讲

可以按下面顺序汇报：

```text
1. 题目目标：利用编译期已知稀疏常量 K，减少无意义乘法。
2. metatensor 基础：region_direct 让插件能在生成代码时读到 K。
3. datadesc 寻址：高维坐标通过 offsets 映射成线性位置。
4. 路线 A：直接代码生成，适合小规模 demo。
5. 路线 B：IR datadesc 改写，适合非零较多、代码量较大的情况。
6. 对比两条路线：A 简单直接，B 更通用更接近编译器优化。
7. 新增扩展：高维 dense、tuple sparse、X sparse、双 sparse。
8. SIMD-ready：连续非零块生成可向量化代码。
9. 测试结果：所有 dump 编译通过，verify_sparse 输出 OK。
```

## 13. 当前边界

当前实现已经覆盖了 `topic_sparse_const` 的高阶扩展，但它仍然不是完整的通用稀疏张量代数系统。

当前边界：

```text
IR 改写路径目前在 block2data_sparse_const 内部完成，还不是独立 optimize_f pass
当前支持一个编译期常量 K 加一个运行期输入 X
当前没有实现任意广播规则
当前没有实现多个运行期 sparse 输入之间的完整交集/并集代数
SIMD 使用条件编译保护，默认验证不要求机器必须支持 AVX2
```

最终总结：

```text
本实现从路线 A 的直接代码生成出发，完成了题目最低 demo；
随后加入路线 B 的 IR datadesc 改写，提升了通用性和代码规模控制能力；
最后扩展到高维、tuple sparse、X sparse、双 sparse 和连续块 SIMD-ready 路径。
```
