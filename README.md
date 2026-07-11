# topic_sparse_const 当前实现说明

本文档总结当前仓库中 `topic_sparse_const` 的实现范围。它不是只完成了最小 demo，而是在最小要求之上继续扩展了若干能力，包括反向乘法、加法、`double`、高维 dense 以及 tuple sparse 常量。

## 1. 这个优化解决什么问题

普通逐元素乘法会对每个位置都生成计算：

```c
out[i] = K[i] * x[i];
```

但如果 `K` 是编译期已经知道的常量，而且里面有大量 `0`，那么很多计算其实没有意义：

```c
K[i] == 0  =>  out[i] 一定是 0
```

所以当前实现会在代码生成阶段读取 `K` 的真实数值，只给有用的位置生成代码，从而减少无意义的乘法。

## 2. 当前支持的表达式形态

当前支持：

```c
K * X
X * K
K + X
float
double
一维 dense K
二维/多维 dense K
高维 tuple sparse K
```

其中：

- `K` 是编译期已知的常量数据，来自 `region_direct`。
- `X` 是运行期输入数据。
- 输出形状跟 `X` 一致。
- 当前 `X` 需要是 dense datadesc。

## 3. 一维 dense 情况

基础情况可以理解成：

```c
out[i] = K[i] * x[i];
```

或者：

```c
out[i] = K[i] + x[i];
```

### 3.1 乘法优化规则

对于乘法：

```c
K * X
X * K
```

生成规则是：

- `K[i] == 0`：不生成这一行，输出缓冲区先用 `memset` 清零。
- `K[i] == 1`：生成 `out[i] = x[i];`
- 其他非零值：生成 `out[i] = 常量 * x[i];`

例如：

```c
K = [0, 1, 2.5, 0]
```

会生成类似：

```c
out[1] = x[1];
out[2] = 2.5f * x[2];
```

不会生成：

```c
out[0] = 0.0f * x[0];
out[3] = 0.0f * x[3];
```

### 3.2 加法优化规则

对于加法：

```c
K + X
```

生成规则是：

- `K[i] == 0`：生成 `out[i] = x[i];`
- 其他值：生成 `out[i] = 常量 + x[i];`

例如：

```c
K = [0, 3, 0, -2]
```

会生成类似：

```c
out[0] = x[0];
out[1] = 3.0f + x[1];
out[2] = x[2];
out[3] = -2.0f + x[3];
```

## 4. 高维 dense 情况

当前版本不只支持一维 `K[i]`，还支持高维 dense 常量，例如二维：

```c
out[row][col] = K[row][col] * x[row][col];
```

在底层 C 代码里，多维数据仍然会映射成一段连续内存。插件会根据 `datadesc` 中的 dimension、index 和 offset 计算真实线性位置。

二维时可以简单理解为：

```c
pos = row_offset + col_offset;
out[pos] = K[row][col] * x[pos];
```

所以高维情况下，优化规则仍然是同一套：

- 乘法里 `K == 0` 就跳过。
- 乘法里 `K == 1` 就直接复制 `x`。
- 乘法里其他非零值就生成 `常量 * x`。
- 加法里 `K == 0` 就直接复制 `x`。
- 加法里其他值就生成 `常量 + x`。

区别只是这里的位置不再只是 `i`，而是由高维坐标映射出来的 `pos`。

## 5. tuple sparse 高维情况

当前版本还支持 tuple sparse 形式的 `K`。

普通 dense 矩阵会存所有位置：

```text
K[0][0], K[0][1], K[0][2], ...
K[1][0], K[1][1], K[1][2], ...
```

tuple sparse 只存有意义的坐标和值，例如：

```text
(0, 0) -> 0.5
(1, 2) -> 1.0
(2, 3) -> -0.75
```

插件会做两件事：

1. 读取 tuple sparse `K` 中保存的坐标和值。
2. 把这些坐标映射到 dense `X` 的真实线性位置。

例如 `(1, 2)` 可能会被映射成线性位置 `6`，于是生成：

```c
out[6] = x[6];
```

或者：

```c
out[6] = 1.5f * x[6];
```

这说明当前实现已经不是简单的“一维数组跳过 0”，而是能够理解 metatensor 的高维 `datadesc` 描述，并根据它生成稀疏优化代码。

## 6. 涉及的主要文件

核心实现：

```text
plugins/block2data_sparse_const.c
```

测试生成程序：

```text
main_test/test.sparse_const_basic.c
```

数值验证程序：

```text
main_test/test.sparse_const_verify.c
```

编译脚本：

```text
Makefile_test_sparse_const
```

生成出来的示例代码包括：

```text
dump_sparse.c
dump_sparse_reverse.c
dump_sparse_sum.c
dump_sparse_double.c
dump_sparse_2d.c
dump_sparse_2d_sum.c
dump_sparse_tuple.c
dump_sparse_tuple_sum.c
```

## 7. 当前测试覆盖

当前验证覆盖了：

- 一维 `float` 乘法：`K * X`
- 一维 `float` 反向乘法：`X * K`
- 一维 `float` 加法：`K + X`
- 一维 `double` 乘法：`K * X`
- 二维 dense 乘法
- 二维 dense 加法
- tuple sparse 乘法
- tuple sparse 加法

验证方式是：把 sparse 优化生成的代码和朴素版本逐元素比较，确认结果一致。

## 8. 如何重新验证

在仓库根目录运行：

```powershell
mingw32-make -f Makefile_test_sparse_const clean
mingw32-make -f Makefile_test_sparse_const verify_sparse
.\verify_sparse.exe
```

期望输出：

```text
OK
```

生成代码时还会看到：

```text
SPARSE PATH HIT
```

这表示 `block2data_sparse_const` 插件确实被调用了，而不是走了普通 arithmetic 路径。

## 10. 仍然可以继续增强的方向

当前实现还不是完整的通用稀疏张量代数系统。后续还可以继续做：

- IR 层 sparse datadesc 改写，而不是只在 `block2data` 阶段直接生成 C 代码。
- 自动判断是否值得启用 sparse 优化，避免非零元素很多时代码膨胀。
- 和 SIMD 后端联动，对连续非零块做向量化。
- 支持 `X` 本身也是 sparse tuple。
- 支持两个 sparse 输入之间的交集、并集和更复杂广播规则。

一句话总结：

```text
当前版本已经支持一维、高维 dense、tuple sparse 的稀疏常量代码生成；
但如果要做到研究级，还可以继续做 IR 改写、启发式选择、SIMD 联动和双稀疏支持。
```
