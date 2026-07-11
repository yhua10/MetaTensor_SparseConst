# 本次新增：独立 `optimize_f` 稀疏常量改写 pass

本文档只说明本次新增内容：把之前在 `block2data_sparse_const` 内部完成的“IR 改写思想”，进一步实现为文档意义上的独立 `optimize_f` pass。

新增核心文件：

```text
plugins/optimize_sparse_const.c
```

新增测试输出：

```text
dump_sparse_opt_pass.c
```

## 1. 新增内容概览

本次新增了真正独立的：

```c
block optimize_sparse_const(target t, block ob)
```

它被注册进 `target_c99_new()` 的 optimize 链：

```c
target_register_optimize(_base, optimize_sparse_const);
```

也就是说，现在可以走标准优化流程：

```c
optimized = optimize(t, ob);
sc = block2data_arithmetic(t, optimized);
```

这和之前不同。

之前的 IR 改写路径是在：

```text
block2data_sparse_const
```

内部一边构造临时 datadesc，一边发射 C 代码。

本次新增后，真正的流程变成：

```text
原始 IR
  -> optimize_sparse_const 做 IR 改写
  -> block2data_arithmetic 做代码生成
```

## 2. 原理说明

### 2.1 为什么需要 `optimize_f`

题目文档中提到两条路线：

```text
路线 A：写 block2data_sparse_const，直接生成优化后的 C 代码
路线 B：写 optimize_sparse_const，在 IR 层改写 datadesc，再交给 block2data_arithmetic
```

路线 A 的特点是：

```text
快，直接，适合 demo
```

路线 B 的特点是：

```text
更符合编译器优化思想
```

因为路线 B 不直接生成 C 代码，而是先改变 IR 的数据描述。

## 3. 实现递进链

本项目的实现可以理解成一条递进链：

```text
路线 A
  -> block2data 内部的 IR 改写思想
  -> 独立 optimize_f pass
```

也就是：

```text
从“直接生成优化代码”
到“在代码生成阶段临时构造稀疏子视图”
再到“真正把稀疏化作为 IR 优化 pass 独立出来”
```

### 3.1 第一阶段：路线 A，直接代码生成

第一阶段实现的是：

```text
block2data_sparse_const
```

它直接匹配：

```text
[prod]-(K & X)
```

然后在代码生成阶段读取 `K` 的值：

```text
K == 0  跳过
K == 1  生成 out = x
其他值  生成 out = K * x
```

这一阶段的特点是：

```text
直接、稳定、容易验证
适合最低 demo 和完整 dense 输出
但优化逻辑和代码生成绑在一起
IR 层看不到“数据已经被稀疏化”
```

所以它解决了“能不能利用稀疏常量少生成乘法”的问题。

### 3.2 第二阶段：block2data 内部的 IR 改写思想

第二阶段仍然在：

```text
block2data_sparse_const
```

内部，但已经不只是逐条手写：

```c
out[i] = K[i] * x[i];
```

而是先收集非零位置，临时构造：

```text
K_ir
X_ir
```

再调用：

```text
block2data_arithmetic
```

这就是之前日志中的：

```text
SPARSE IR REWRITE PATH HIT
```

这一阶段的意义是：

```text
已经出现了路线 B 的核心思想：
把原始 dense 计算压缩成只包含有效位置的稀疏子计算。
```

但它还不是完整路线 B，因为：

```text
它发生在 block2data_sparse_const 内部
它一边构造临时 datadesc，一边继续 emit C 代码
它不是独立 optimize_f pass
```

所以它可以称为：

```text
不完整路线 B / 路线 B 思想的内嵌实现
```

### 3.3 第三阶段：完整路线 B，独立 optimize_f pass

第三阶段就是本次新增：

```text
plugins/optimize_sparse_const.c
```

它实现了：

```c
block optimize_sparse_const(target t, block ob)
```

这时流程变成：

```text
原始 IR
  -> optimize_sparse_const
  -> 新 IR
  -> block2data_arithmetic
  -> 目标 C 代码
```

也就是：

```text
[prod]-(K & X)
```

被改写为：

```text
[prod]-(K_view & X_view)
```

这一阶段的关键区别是：

```text
optimize_sparse_const 只做 IR -> IR
它不 emit 代码
它返回新的 block
后续由 block2data_arithmetic 接管
```

这就符合题目文档中路线 B 的定义。

### 3.4 三个阶段对比

| 阶段 | 实现位置 | 是否独立 pass | 是否 emit 代码 | 核心作用 |
|---|---|---|---|---|
| 路线 A | `block2data_sparse_const` | 否 | 是 | 直接生成跳过零位置的代码 |
| 内嵌 IR 改写 | `block2data_sparse_const` 内部 | 否 | 是 | 临时构造 `K_ir/X_ir`，复用 arithmetic 思想 |
| 完整路线 B | `optimize_sparse_const` | 是 | 否 | 真正 IR -> IR，交给 arithmetic 接管 |

这条递进链说明：

```text
最开始解决“能优化”；
中间解决“能抽象成稀疏子视图”；
最后解决“能作为独立编译优化 pass 存在”。
```

### 3.5 当前最终状态

现在项目里两条路线都存在：

```text
路线 A：block2data_sparse_const
路线 B：optimize_sparse_const + block2data_arithmetic
```

二者分工不同：

```text
路线 A 负责完整 dense 输出，适合最终用户可直接调用。
路线 B 负责 packed sparse 子计算，适合展示 IR 层优化和 arithmetic 复用。
```

这不是冲突，而是两种层次的优化：

```text
路线 A 偏代码生成优化；
路线 B 偏 IR 结构优化。
```

## 4. optimize pass 具体原理

### 4.1 原始 IR 长什么样

优化前的 IR 可以理解为：

```text
[prod]-(K & X)
```

其中：

```text
K.region == region_direct
X.region == region_main
```

`K` 是编译期常量，所以优化 pass 可以直接读取：

```c
K->ref.p_direct
```

从而知道哪些位置是 0，哪些位置非 0。

### 4.2 optimize pass 做了什么

假设原始数据是：

```text
K = [0.5, 0, 0, 1.2, 0, 0, 1.0, 0, 0, -0.3, 0, 0, 0, 0, 0.7, 0]
X = [x0, x1, x2, x3, ..., x15]
```

非零位置是：

```text
0, 3, 6, 9, 14
```

`optimize_sparse_const` 会把原来的 dense datadesc 改写成两个新的 1D sparse view：

```text
K_view = [0.5, 1.2, 1.0, -0.3, 0.7]
X_view = [x0,  x3,  x6,  x9,   x14]
```

也就是把原来的完整数组：

```text
16 个位置
```

压缩成只包含非零乘法需要的：

```text
5 个位置
```

然后返回新的 IR：

```text
[prod]-(K_view & X_view)
```

注意：这个函数本身不发射 C 代码，只返回新的 block。

### 4.3 为什么还要改 `block2data_arithmetic`

真正路线 B 的目标是：

```text
改写后让 block2data_arithmetic 接管
```

但原来的 `block2data_arithmetic` 默认处理运行期指针数据，不会把 `region_direct` 的常量内容发射到目标 C 代码里。

所以本次也增强了：

```text
plugins/block2data_arithmetic.c
```

新增能力：

```text
当输入 datadesc 是 region_direct 时，
block2data_arithmetic 会把它展开成目标 C 代码里的 const 数组。
```

例如生成：

```c
const float K_view[5] = {
    0.5f, 1.2f, 1.0f, -0.3f, 0.7f
};
```

这样 arithmetic 就可以像处理普通输入一样处理它：

```c
tmp[i] = K_view[i] * X_view[i];
```

## 5. 功能描述

本次新增的 `optimize_sparse_const` 支持：

```text
表达式：[prod]-(K & X)
反向：[prod]-(X & K)
K：region_direct
X：region_main
类型：float / double
布局：dense datadesc
维度：一维或多维 dense，只要 K 和 X 维度名与 indices 能匹配
```

当前 pass 的输出语义是：

```text
输出为 packed sparse 结果
```

也就是说，它输出的是非零位置对应的紧凑结果：

```text
[K[0]*X[0], K[3]*X[3], K[6]*X[6], K[9]*X[9], K[14]*X[14]]
```

而不是完整 dense 输出：

```text
[out0, out1, ..., out15]
```

这是路线 B 的关键特征：它先把 datadesc 缩成稀疏子集，让 arithmetic 接管稀疏子集的计算。

完整 dense 输出仍由之前的 `block2data_sparse_const` 负责。

## 6. 与之前实现的对比

### 6.1 之前：block2data 内部改写

之前已经有：

```text
SPARSE IR REWRITE PATH HIT
```

但那条路径仍然发生在：

```text
block2data_sparse_const
```

内部。

它的特点是：

```text
一边构造临时 datadesc
一边发射 C 代码
最后还负责 scatter 回 dense 输出
```

所以它虽然体现了 IR 改写思想，但不是独立的 `optimize_f` pass。

### 6.2 现在：真正 optimize pass

现在新增的是：

```text
SPARSE OPTIMIZE PASS HIT
```

它的特点是：

```text
只做 IR -> IR
不调用 emit_push
不生成 C 代码
返回一个新的 block
由 block2data_arithmetic 继续生成代码
```

对比表：

| 项目 | 之前的内部 IR 改写 | 本次新增 optimize_f pass |
|---|---|---|
| 所在位置 | `block2data_sparse_const` 内部 | `plugins/optimize_sparse_const.c` |
| 是否独立 pass | 否 | 是 |
| 是否 emit 代码 | 是 | 否 |
| 输出 | dense 完整输出 | packed sparse 输出 |
| 后续代码生成 | sparse_const 自己控制 | `block2data_arithmetic` 接管 |
| 更符合文档路线 B | 部分符合 | 符合 |

## 7. 测试方案

新增测试在：

```text
main_test/test.sparse_const_basic.c
main_test/test.sparse_const_verify.c
```

测试流程：

```c
ob = make_sparse_block(K, X, block_builtin_reduce_prod, 0);
t = target_c99_new();
optimized = optimize(t, ob);
sc = block2data_arithmetic(t, optimized);
```

这里故意调用：

```c
block2data_arithmetic(t, optimized)
```

而不是：

```c
block2data(t, optimized)
```

目的是证明：

```text
优化后的 IR 确实可以被原来的 arithmetic 路径接管。
```

新增生成文件：

```text
dump_sparse_opt_pass.c
```

它会生成类似结构：

```c
const float K_view[5] = {
    0.5f, 1.2f, 1.0f, -0.3f, 0.7f
};

for (...) {
    tmp[i] = K_view[i] * input_x[offset_table[i]];
}
```

验证时，将 packed 输出与朴素结果比较：

```text
generated[j] == K[index[j]] * X[index[j]]
```

## 8. 测试结果

运行：

```powershell
mingw32-make -f Makefile_test_sparse_const clean
mingw32-make -f Makefile_test_sparse_const verify_sparse
.\verify_sparse.exe
```

结果：

```text
OK
```

日志中可以看到：

```text
SPARSE OPTIMIZE PASS HIT
```

说明：

```text
optimize_sparse_const 被调用；
原始 IR 被改写；
改写后的 IR 成功交给 block2data_arithmetic；
生成代码编译通过；
数值验证通过。
```

## 9. 当前边界

本次新增 pass 聚焦“文档意义上的路线 B”，因此边界是：

```text
只处理 prod，不处理 sum
输出 packed sparse 结果，不负责 scatter 回 dense 输出
主要处理 dense K / dense X 的 datadesc 改写
完整 dense 输出仍由 block2data_sparse_const 路径负责
```

这个边界是有意设计的：

```text
optimize_f 只做 IR 改写；
block2data_f 负责最终代码生成和具体输出形态。
```

## 10. 本次新增成果总结

本次新增完成了：

```text
1. 新增 plugins/optimize_sparse_const.c
2. 在 target_c99_new 中注册 optimize_sparse_const
3. 增强 block2data_arithmetic，使其能消费 region_direct 常量 datadesc
4. 新增 dump_sparse_opt_pass.c 生成路径
5. 新增 packed sparse 数值验证
6. verify_sparse.exe 输出 OK
```

最终效果：

```text
之前：只有 block2data 级别的稀疏常量优化。
现在：同时具备真正 optimize_f 级别的 IR 改写优化。
```
