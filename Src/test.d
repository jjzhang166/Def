#--
 Def 语言测试执行文件
--#









#--



# 处理器定义
def mmm{a b}
	mydeal{a}
	1
	2
	3
;




mydeal{a}



cc : -123.99

# 函数定义
defun printl(a ,-123 b c:cc)
	a
;


# 字典
dt : [
	false value
	'key' name
]


print obj.dt.use{ none ,-2 true }


# 列表
a : (-999  1  ,-2  a-b  a:1+3)




# 字典
dt : [
	'key' value
]


# 块，不执行
{
	a : a+1
}




# 函数定义
defun myfunc()
	a : a+1
	b : a+c
;



--#


1/f() + 1*m.d.c{a fun() ,-999} + 100






















#--

v : 1         # 变量赋值
v:: 1         # 向上搜索赋值
a+b-c*d/e     # 加减乘除
a=b           # 相等比较
a>b
a<b
a>=b
a<=b
a~=b          # 不相等比较
a&b           #（与）
a|b           #（或）
~a=b          #（取反）

(a+b)*c       # 优先级
(a b c)       # 元组
[a b c]       # 数组
{'a'a  'b'b}  # 字典

array[i]      # 容器访问
getVar(a b)   # 函数调用
dealit{a a+b} # 处理器调用

# 关键字

def
defun
class

if
while

return
break
continue




# 定义处理器
def repeat{a b}
	...
;

# 定义函数
defun function(a b)
	...
;

# if 分支
if a
	...
elif b
	...
else
	...
;

# while 循环
while num
	...
;


--#













