# `topic_sparse_const` 完整实现说明

## 1. 一句话概括

本次工作的目标是优化下面这种表达式：

```c
out = K * X
out = X * K
out = K + X
```

其中：

```text
K 是编译期已知的常量张量
X 是运行期输入张量
```

因为 `K` 在生成代码之前就能看见，所以如果 `K[i] == 0`，很多位置根本不需要真的生成计算。

最终实现效果是：

```text
1. 路线 A：block2data_sparse_const 直接生成优化后的 C 代码
2. 路线 B：optimize_sparse_const 先改写 IR，再由 block2data 接管生成代码
3. 路线 B 已升级为完整 dense 输出，不再只是 packed sparse demo
4. 支持 prod / sum、float / double、一维 / 多维 dense、广播规则
5. 普通 block2data() 会自动先跑 optimize，不需要手动调用 optimize()
```

## 2. 最基础概念

### 2.1 什么是 `K`

`K` 是常量，例如：

```text
K = [0.5, 0, 0, 1.2, 0, 0, 1.0, 0]
```

它的 `datadesc` 里：

```c
K->region == region_direct
```

意思是：`K` 的真实数据在元编程阶段就能直接读到。

所以优化 pass 可以直接读：

```c
K->ref.p_direct
```

然后知道哪些位置是 0，哪些位置不是 0。

### 2.2 什么是 `X`

`X` 是运行期输入，例如：

```c
void compute(float *input_x, float *out)
```

它的 `datadesc` 里：

```c
X->region == region_main
```

意思是：真正的数据要等目标程序运行时才有。

所以我们不能在元编程阶段读取 `X[i]`，只能生成访问 `X[i]` 的代码。

### 2.3 什么是 `datadesc`

`datadesc` 可以理解成“数据说明书”。

它不存普通运行期数据，而是描述：

```text
数据在哪：region_direct / region_main
数据叫什么：ref.p_direct / ref.p_refname
元素类型：float / double
元素大小：4 / 8
有几个维度：n_dim
每个维度有哪些下标，以及这些下标对应内存里的哪些 offset
```

例如一个二维张量：

```text
X[row, col]
```

在 `datadesc` 里不是简单写成二维数组，而是记录：

```text
row 维有哪些 index
col 维有哪些 index
每个 index 对应的内存 offset 是多少
```

这样项目就可以描述不连续、切片、稀疏、高维等更灵活的数据布局。

### 2.4 什么是 IR

IR 可以理解成“还没变成 C 代码之前的计算图”。

本题里最重要的 IR 长这样：

```text
[prod]-(K & X)
```

它的意思是：

```text
把 K 和 X 并联起来
然后对它们做乘法归约
```

也就是：

```c
out[i] = K[i] * X[i];
```

加法对应：

```text
[sum]-(K & X)
```

也就是：

```c
out[i] = K[i] + X[i];
```

## 3. 实现递进链

整个实现不是一步跳到最终版本，而是一条递进链：

```text
路线 A
  -> 不完整路线 B
  -> 完整路线 B
  -> 增强后的完整路线 B
```

### 3.1 第一阶段：路线 A

路线 A 的文件是：

```text
plugins/block2data_sparse_const.c
```

它直接匹配原始 IR：

```text
[prod]-(K & X)
[sum]-(K & X)
```

然后直接生成 C 代码。

对于乘法：

```text
K[i] == 0：不生成这一行，输出靠 memset 得到 0
K[i] == 1：生成 out[i] = x[i]
其他值：生成 out[i] = K[i] * x[i]
```

对于加法：

```text
K[i] == 0：输出等于 x[i]
K[i] != 0：生成 out[i] = K[i] + x[i]
```

路线 A 的优点：

```text
直接
稳定
容易验证
能输出完整 dense 结果
```

路线 A 的不足：

```text
优化逻辑和代码生成绑在一起
IR 本身没有被真正改写
不够像一个标准编译器优化 pass
```

### 3.2 第二阶段：不完整路线 B

后来在 `block2data_sparse_const` 内部加入了“IR 改写思想”。

它会先收集非零位置，构造临时的：

```text
K_ir
X_ir
```

然后复用：

```text
block2data_arithmetic
```

日志里可以看到：

```text
SPARSE IR REWRITE PATH HIT
```

这一阶段已经有路线 B 的思想：

```text
把 dense 计算压缩成只包含有效位置的 sparse 子计算
```

但它仍然不是真正的路线 B，因为：

```text
它还发生在 block2data_sparse_const 内部
它仍然一边改写，一边 emit C 代码
它不是独立 optimize_f pass
```

所以它只能算：

```text
不完整路线 B
```

### 3.3 第三阶段：完整路线 B

完整路线 B 的核心文件是：

```text
plugins/optimize_sparse_const.c
```

它实现了真正独立的：

```c
block optimize_sparse_const(target t, block ob)
```

这一步只做：

```text
IR -> IR
```

不直接生成 C 代码。

优化前：

```text
[prod]-(K & X)
```

优化后：

```text
[prod]-(K_view & X_view)
```

其中：

```text
K_view 只保存 K 的非零值
X_view 只指向 X 中需要参与计算的位置
```

例如：

```text
K = [0.5, 0, 0, 1.2, 0, 0, 1.0, 0]
X = [x0, x1, x2, x3, x4, x5, x6, x7]
```

会被改写成：

```text
K_view = [0.5, 1.2, 1.0]
X_view = [x0,  x3,  x6]
```

这样 arithmetic 只需要计算 3 个位置，而不是 8 个位置。

### 3.4 第四阶段：增强后的完整路线 B

之前完整路线 B 只能输出 packed sparse 结果：

```text
[K[0]*X[0], K[3]*X[3], K[6]*X[6]]
```

这证明了 IR 优化是成功的，但它不是最终用户最想要的完整输出。

本次继续升级后，路线 B 现在会输出完整 dense 结果：

```text
[out0, out1, out2, out3, out4, out5, out6, out7]
```

做法是：

```text
1. optimize_sparse_const 仍然只做 IR 改写
2. 改写后的 IR 携带 sparse 子计算和 scatter 元数据
3. block2data_sparse_const_optimized 调用 block2data_arithmetic 计算 packed 结果
4. 再把 packed 结果 scatter 回 dense 输出
```

乘法的完整流程：

```text
原始 dense 计算
  -> 收集 K 的非零位置
  -> 构造 K_view / X_view
  -> arithmetic 只计算非零位置
  -> dense 输出先 memset 为 0
  -> 把非零结果 scatter 回原位置
```

加法的完整流程：

```text
原始 dense 计算
  -> 收集 K 的非零位置
  -> 构造 K_view / X_view
  -> dense 输出先 memcpy X
  -> arithmetic 只计算 K 非零位置上的 K + X
  -> 把这些位置 scatter 回输出
```

这就是为什么乘法和加法的初始化不同：

```text
乘法：K == 0 时结果就是 0，所以先 memset
加法：K == 0 时结果就是 X，所以先 memcpy X
```

## 4. 当前两条路线的关系

现在项目里两条路线都存在：

```text
路线 A：block2data_sparse_const
路线 B：optimize_sparse_const + block2data_sparse_const_optimized + block2data_arithmetic
```

它们不是互相冲突，而是分工不同。

| 路线 | 核心文件 | 主要作用 | 特点 |
|---|---|---|---|
| 路线 A | `plugins/block2data_sparse_const.c` | 直接生成优化 C 代码 | 代码生成层优化，覆盖 tuple sparse 等路径 |
| 路线 B | `plugins/optimize_sparse_const.c` | 先改写 IR，再生成代码 | 编译器 pass 风格，支持 dense scatter、sum、broadcast |

现在普通用户调用：

```c
sc = block2data(t, ob);
```

时，会自动先执行：

```c
ob = optimize(t, ob);
```

所以不需要手动写：

```c
optimized = optimize(t, ob);
```

如果 `optimize_sparse_const` 能处理，就走完整路线 B。

如果不能处理，例如 tuple sparse 的复杂情况，就回落到路线 A。

## 5. 本次新增能力

### 5.1 完整 dense 输出

之前路线 B 输出 packed sparse 结果。

现在路线 B 输出完整 dense 结果。

例如：

```text
K = [0.5, 0, 0, 1.2]
X = [x0, x1, x2, x3]
```

乘法输出：

```text
[0.5*x0, 0, 0, 1.2*x3]
```

而不是：

```text
[0.5*x0, 1.2*x3]
```

这一步靠 scatter 完成。

### 5.2 支持加法 `K + X`

现在独立路线 B 支持：

```text
[sum]-(K & X)
```

核心规则是：

```text
K == 0：不需要重新计算，输出保持 X
K != 0：只在这个位置计算 K + X
```

所以生成代码会先：

```c
memcpy(out, input_x, nbytes);
```

然后只改写 K 非零的位置。

### 5.3 支持广播规则

广播可以理解成“小的 K 自动铺开到大的 X 上”。

例如：

```text
K[col]      shape = [4]
X[row,col]  shape = [3,4]
```

`K` 没有 `row` 维，但它可以沿着 `row` 方向复用。

如果：

```text
K[col] = [0, 2, -1, 0]
```

那么对每一行都使用同一个 `K[col]`：

```text
row 0: [0*x00, 2*x01, -1*x02, 0*x03]
row 1: [0*x10, 2*x11, -1*x12, 0*x13]
row 2: [0*x20, 2*x21, -1*x22, 0*x23]
```

实现时不是把 `K` 真的复制三遍，而是在 IR 改写阶段生成映射：

```text
K_view = [2, -1, 2, -1, 2, -1]
X_view = [x01, x02, x11, x12, x21, x22]
scatter = [1, 2, 5, 6, 9, 10]
```

也就是说：

```text
只计算 K 非零的位置
再把结果放回完整 dense 输出的对应位置
```

### 5.4 自动接入 `block2data`

之前测试需要手动写：

```c
optimized = optimize(t, ob);
sc = block2data_arithmetic(t, optimized);
```

现在 `src/target_base.c` 中的 `block2data()` 会自动先跑：

```c
ob = optimize(t, ob);
```

然后再交给 block2data 插件链。

因此外部使用方式更自然：

```c
sc = block2data(t, ob);
```

### 5.5 arithmetic 支持 `region_direct`

路线 B 会构造：

```text
K_view
```

而 `K_view` 仍然是编译期常量：

```c
K_view->region == region_direct
```

所以 `block2data_arithmetic` 需要能把它展开成目标 C 代码里的 const 数组。

现在它可以生成：

```c
const float K_view[5] = {
    0.5f, 1.2f, 1.0f, -0.3f, 0.7f
};
```

然后像普通数组一样参与 arithmetic 计算。

### 5.6 生成代码质量优化

之前 arithmetic 会生成类似：

```c
out[i] = 1 * a[i] * b[i];
out[i] = 0 + a[i] + b[i];
```

现在去掉了多余的身份元，变成：

```c
out[i] = a[i] * b[i];
out[i] = a[i] + b[i];
```

完整路线 B 的 scatter 也有简单优化：

```text
如果 sparse 输出位置正好是从 0 开始的连续输出，就直接 memcpy
否则才生成 offset table 做 scatter
```

## 6. 核心文件变化

### 6.1 `plugins/optimize_sparse_const.c`

这是本次最核心的文件。

它现在包含两部分：

```text
optimize_sparse_const
block2data_sparse_const_optimized
```

`optimize_sparse_const` 负责：

```text
识别 K * X / X * K / K + X
读取 region_direct 的 K
根据 K 的非零位置构造 K_view
根据 X 的真实 offset 构造 X_view
处理 dense 多维和广播
生成 scatter offset 元数据
返回新的 block
```

`block2data_sparse_const_optimized` 负责：

```text
识别 optimize_sparse_const 返回的新 block
为最终 dense 输出分配内存
prod 时 memset 为 0
sum 时 memcpy 原始 X
调用 block2data_arithmetic 计算 packed sparse 子结果
把 packed 结果 scatter 回 dense 输出
```

### 6.2 `src/target_base.c`

`block2data()` 现在会自动先跑优化：

```c
ob = optimize(t, ob);
```

这让路线 B 进入正常编译流程。

### 6.3 `targets/target_c99.c`

新增注册：

```c
target_register_block2data(_base, block2data_sparse_const_optimized);
target_register_optimize(_base, optimize_sparse_const);
```

插件优先级上，完整路线 B 的 codegen 插件会优先识别优化后的 block。

### 6.4 `plugins/block2data_arithmetic.c`

增强了两点：

```text
1. 可以消费 region_direct 常量 datadesc
2. 去掉 sum/prod 生成代码里的 0+ 和 1*
```

### 6.5 `main_test/test.sparse_const_basic.c`

新增生成：

```text
dump_sparse_broadcast.c
dump_sparse_broadcast_sum.c
dump_sparse_opt_pass.c
```

其中 `dump_sparse_opt_pass.c` 现在验证的是完整 dense 输出路线 B。

### 6.6 `main_test/test.sparse_const_verify.c`

新增验证：

```text
广播乘法
广播加法
完整 dense optimize pass 输出
```

## 7. 支持能力总表

| 能力 | 路线 A | 增强后路线 B |
|---|---:|---:|
| `K * X` | 支持 | 支持 |
| `X * K` | 支持 | 支持 |
| `K + X` | 支持 | 支持 |
| `float` | 支持 | 支持 |
| `double` | 支持 | 支持 |
| 一维 dense | 支持 | 支持 |
| 多维 dense | 支持 | 支持 |
| 完整 dense 输出 | 支持 | 支持 |
| packed sparse 子计算 | 内部使用 | 支持 |
| 独立 `optimize_f` pass | 不支持 | 支持 |
| 自动接入 `block2data()` | 不需要 | 支持 |
| 广播规则 | 不作为主路径 | 支持 |
| tuple sparse | 支持 | 当前回落路线 A |
| SIMD-friendly 连续块 | 支持 | 当前主要走 arithmetic + scatter |

## 8. 生成代码长什么样

以乘法为例，路线 B 现在会生成类似结构：

```c
float *out_dense = malloc(64);
memset(out_dense, 0, 64);

float *tmp_sparse = malloc(20);

const float K_view[5] = {
    0.5f, 1.2f, 1.0f, -0.3f, 0.7f
};

for (...) {
    tmp_sparse[i] = K_view[i] * X_view[i];
}

const int32_t scatter[5] = {
    0, 3, 6, 9, 14
};

for (...) {
    out_dense[scatter[i]] = tmp_sparse[i];
}
```

对加法，区别是开头不是 `memset`，而是：

```c
memcpy(out_dense, input_x, nbytes);
```

因为：

```text
K == 0 时，K + X == X
```

## 9. 测试方案

测试使用：

```powershell
mingw32-make -f Makefile_test_sparse_const clean
mingw32-make -f Makefile_test_sparse_const verify_sparse
.\verify_sparse.exe
```

测试覆盖：

```text
1. 一维 float 乘法
2. 一维 float 反向乘法 X * K
3. 一维 float 加法
4. K 全零乘法
5. K 全零加法
6. 一维 double 乘法
7. 二维 dense 乘法
8. 二维 dense 加法
9. 广播乘法 K[col] * X[row,col]
10. 广播加法 K[col] + X[row,col]
11. tuple sparse K + dense X
12. tuple sparse K + tuple sparse X
13. 大数组 IR rewrite 路径
14. 连续块 SIMD-friendly 路径
15. 独立 optimize pass + dense scatter 路线 B
```

最终验证结果：

```text
OK
```

编译过程中仍会出现一些项目原有 warning，例如旧代码里的 switch 枚举覆盖、未使用变量、函数指针类型警告等；这些不是本次新增 sparse_const 路径导致的错误，数值验证已经通过。

## 10. 和之前版本的对比

| 阶段 | 输出形态 | 是否独立 pass | 是否自动接入 | 支持 sum | 支持 broadcast |
|---|---|---:|---:|---:|---:|
| 最低 demo | dense | 否 | 否 | 部分 | 否 |
| 路线 A 完整版 | dense | 否 | 是 | 是 | 否 |
| 不完整路线 B | dense | 否 | 否 | 是 | 否 |
| 之前完整路线 B | packed sparse | 是 | 否 | 否 | 否 |
| 当前增强路线 B | dense | 是 | 是 | 是 | 是 |

现在可以这样总结：

```text
路线 A 解决了“能不能直接生成优化代码”。
不完整路线 B 解决了“能不能在代码生成阶段复用 sparse 子计算思想”。
之前完整路线 B 解决了“能不能把 IR 改写独立成 optimize_f pass”。
当前增强路线 B 解决了“能不能让独立 optimize_f pass 真正输出完整 dense 结果，并支持 sum / broadcast / 自动流程”。
```

## 11. 当前边界

当前增强路线 B 重点覆盖：

```text
region_direct dense K
region_main dense X
prod / sum
float / double
一维 / 多维 dense
广播
完整 dense 输出
```

tuple sparse 目前仍由路线 A 负责。

这个设计是有意保留的：

```text
路线 B 作为标准 optimize pass，先把最典型的 dense + broadcast 场景做完整；
路线 A 继续覆盖 tuple sparse 和更直接的代码生成优化路径。
```

## 12. 最终成果

本次继续优化后，`topic_sparse_const` 已经从“最低 demo”推进到一个完整可汇报版本：

```text
1. 有清晰的路线 A
2. 有清晰的不完整路线 B 过渡阶段
3. 有真正独立的 optimize_f pass
4. 有增强后的完整路线 B dense 输出
5. 支持乘法、反向乘法、加法
6. 支持 float / double
7. 支持高维 dense 和广播规则
8. 普通 block2data() 自动接入 optimize
9. arithmetic 能消费 region_direct 常量
10. 完整测试通过，verify_sparse.exe 输出 OK
```
